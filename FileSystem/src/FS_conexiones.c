/*
 * FS_conexiones.c
 *
 *  Created on: 8/9/2017
 *      Author: utnso
 */

#include "FS_conexiones.h"

#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/string.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/serializacion.h>
#include <funcionesCompartidas/serializacion_yama_master.h>
#include "FS_interfaz_nodos.h"
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "estructurasfs.h"
#include "FS_administracion.h"

extern yamafs_config *configuracion;
extern t_list *nodos;
extern t_log *logi;
fd_set master;
fd_set read_fds;
int fdmax;
int yamasock;

message *create_Message(header *head, void *data);

size_t dtamanio_bloque_archivo(bloqueArchivo *ba) {

    size_t retorno = 0;

    retorno += strlen(ba->nodo0) + 1;
    retorno += strlen(ba->nodo1) + 1;
    retorno += sizeof(bloqueArchivo);

    return retorno;
}

size_t dtamanio_lista_t_nodo(t_list *nodis) {

    int i;
    size_t tfinal = 0;
    t_nodo *nodi;

    for (i = 0; i < nodis->elements_count; i++) {

        nodi = list_get(nodis, i);
        tfinal += strlen(nodi->nodo) + 2 + strlen(nodi->ip) + sizeof(t_nodo);
    }


    return tfinal;
}

char *dserializar_bloque_archivo(bloqueArchivo *inf, size_t *len) {

    size_t desplazamiento = 0;
    size_t leng = dtamanio_bloque_archivo(inf);
    size_t aux;
    char *buff = malloc(leng + sizeof(int));

    memcpy(buff + desplazamiento, &leng, sizeof(int));
    desplazamiento += sizeof(int);

    aux = strlen(inf->nodo0) + 1;
    memcpy(buff + desplazamiento, &aux, sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(buff + desplazamiento, inf->nodo0, aux);
    desplazamiento += aux;

    aux = strlen(inf->nodo1) + 1;
    memcpy(buff + desplazamiento, &aux, sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(buff + desplazamiento, inf->nodo1, aux);
    desplazamiento += aux;

    memcpy(buff + desplazamiento, &inf->bloquenodo0, sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(buff + desplazamiento, &inf->bloquenodo1, sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(buff + desplazamiento, &inf->bytesEnBloque, sizeof(int));
    desplazamiento += sizeof(int);

    *len = desplazamiento;

    return buff;
}

bloqueArchivo *ddeserializar_bloque_archivo(char *serba) {

    bloqueArchivo *nuevo = malloc(sizeof(bloqueArchivo));
    size_t aux;
    size_t desplazamiento = 0;

    memcpy(&aux, serba + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);
    nuevo->nodo0 = malloc(aux + 1);
    nuevo->nodo0[aux] = '\0';
    memcpy(nuevo->nodo0, serba + desplazamiento, aux);
    desplazamiento += aux;

    memcpy(&aux, serba + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);
    nuevo->nodo1 = malloc(aux + 1);
    nuevo->nodo0[aux] = '\0';
    memcpy(nuevo->nodo1, serba + desplazamiento, aux);
    desplazamiento += aux;

    memcpy(&nuevo->bloquenodo0, serba + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(&nuevo->bloquenodo1, serba + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(&nuevo->bytesEnBloque, serba + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);

    return nuevo;
}

char *dserializar_list_bloque_archivo(t_list *info_nodos_arc, size_t *leng) {

    size_t lengtotal = 0;
    size_t desplazamiento = 0;
    int cantnodos = list_size(info_nodos_arc);
    int i;
    bloqueArchivo *uno;
    for (i = 0; i < cantnodos; i++) {
        uno = list_get(info_nodos_arc, i);
        lengtotal += dtamanio_bloque_archivo(uno);
    }
    char *buffer = malloc(lengtotal + sizeof(int) + info_nodos_arc->elements_count * sizeof(int));

    memcpy(buffer + desplazamiento, &cantnodos, sizeof(int));
    desplazamiento += sizeof(int);

    char *baux;
    size_t laux = 0;
    for (i = 0; i < cantnodos; i++) {
        uno = list_get(info_nodos_arc, i);
        baux = dserializar_bloque_archivo(uno, &laux);
        memcpy(buffer + desplazamiento, baux, laux);
        desplazamiento += laux;
        free(baux);
    }

    *leng = desplazamiento;

    return buffer;
}

t_list *ddeserializar_lista_bloque_archivo(char *serializacion) {

    t_list *final = list_create();
    size_t desplazamiento = 0;

    int cantnodos, i;

    memcpy(&cantnodos, serializacion + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);

    size_t aux;
    for (i = 0; i < cantnodos; i++) {

        memcpy(&aux, serializacion + desplazamiento, sizeof(int));
        desplazamiento += sizeof(int);
        char *baux = malloc(aux);
        memcpy(baux, serializacion + desplazamiento, aux);
        desplazamiento += aux;
        bloqueArchivo *nuevito = deserializar_bloque_archivo(baux);
        list_add(final, nuevito);
        free(baux);
    }

    return final;
}

char *dserializar_lista_nodos(t_list *nodis, size_t *leng) {

    size_t tfinal = dtamanio_lista_t_nodo(nodis);
    size_t aux = 0;
    size_t desplazamiento = 0;
    int i = nodis->elements_count;
    char *buffer = malloc(tfinal + (size_t) sizeof(int) * (i + 1));

    t_nodo *nodi;
    char *subbuffer;

    memcpy(buffer + desplazamiento, &i, sizeof(int));
    desplazamiento += sizeof(int);

    for (i = 0; i < nodis->elements_count; i++) {

        nodi = list_get(nodis, i);
        subbuffer = serializar_nodo(nodi, &aux);

        memcpy(buffer + desplazamiento, &aux, sizeof(int));
        desplazamiento += sizeof(int);

        memcpy(buffer + desplazamiento, subbuffer, aux);
        desplazamiento += aux;

        free(subbuffer);
    }

    *leng = desplazamiento;
    return buffer;

}

t_list *ddeserializar_lista_nodos(char *buffer) {

    int cant_nodos;
    size_t desplazamiento = 0;
    size_t aux = 0;
    int aux2;

    t_list *nueva = list_create();

    memcpy(&cant_nodos, buffer + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);

    char *subbuffer;

    for (aux2 = 0; aux2 < cant_nodos; aux2++) {

        memcpy(&aux, buffer + desplazamiento, sizeof(int));
        desplazamiento += sizeof(int);

        subbuffer = malloc(aux);

        memcpy(subbuffer, buffer + desplazamiento, aux);
        desplazamiento += aux;

        t_nodo *nodi = deserializar_nodo(subbuffer, &aux);

        list_add(nueva, nodi);

        free(subbuffer);
    }

    return nueva;

}

void manejo_conexiones() {


    log_info(logi, "Iniciando administrador de conexiones");

    char *puerto = string_itoa(configuracion->puerto);
    int control = 0;
    if ((fdmax = configuracion->serverfs = makeListenSock(puerto, logi, &control)) < 0) {
        perror("Error en algo de sockets %s\n");
        free(puerto);
        pthread_exit((void *) -1);
    }
    free(puerto);
    //Seteo en 0 el master y temporal
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    //Cargo el socket server
    FD_SET(configuracion->serverfs, &master);

    //Bucle principal
    while (1) {
        read_fds = master;

        int selectResult = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
        log_info(logi, "Actividad detectada en administrador de conexiones");

        if (selectResult == -1) {
            log_info(logi, "Error en el administrador de conexiones");
            break;

        } else {
            //Recorro los descriptores para ver quien llamo
            int i;
            for (i = 0; i <= fdmax; i++) {
                if (FD_ISSET(i, &read_fds)) {
                    //Se detecta alguien nuevo llamando?
                    if (i == configuracion->serverfs) {
                        //Gestiono la conexion entrante
                        int nuevo_socket = aceptar_conexion(configuracion->serverfs, logi, &control);
                        //Controlo que no haya pasado nada raro y acepto al nuevo

                        int exitoso = realizar_handshake(nuevo_socket);
                        if (exitoso == 1) {
                            //Cargo la nueva conexion a la lista y actualizo el maximo
                            FD_SET(nuevo_socket, &master);

                            if (nuevo_socket > fdmax) {
                                fdmax = nuevo_socket;
                            }
                        } else {
                            close(nuevo_socket);
                        }
                    } else {
                        int estado = direccionar(i);
                        if (estado == -1) {
                            FD_CLR(i, &master);
                            close(i);
                            continue;
                        }

                    }
                }
            }
        }
    }
}

int direccionar(int socket_rec) {
    printf("socket number[%d]", socket_rec);
    int status;
    header *header_mensaje = malloc(sizeof(header));
    char *mensaje = getMessage(socket_rec, header_mensaje, &status);

    if (status == -1) {
        //	perror("Error recibiendo");
    } else if (status == 0) {
        log_info(logi, "Se desconecto socket");
        disconnectedNodo(socket_rec);
        return -1;
    } else {
        if (header_mensaje->letra == 'Y') {
            atender_mensaje_YAMA(header_mensaje->codigo, mensaje);
        } else if (header_mensaje->letra == 'N') {
            atender_mensaje_NODO(header_mensaje->codigo, mensaje);
        } else if (header_mensaje->letra == 'W') {
            atender_mensaje_NODO(header_mensaje->codigo, mensaje);
        } else {
            // no entiendo emisor/mensaje
        }
    }

    //free(mensaje);
    return status;
}

int realizar_handshake(int nuevo_socket) {

    int retornar, estado;
    header *identificacion = malloc(sizeof(header));
    header *respuesta = malloc(sizeof(header));
    memset(respuesta, 0, sizeof(header));
    message *mensajeEnviar;
    char *buff = getMessage(nuevo_socket, identificacion, &estado);

    if (identificacion->letra == 'Y') {

        if (configuracion->estado_estable) {

            log_info(logi, "Se conecto YAMA");

            size_t leng = 0;
            respuesta->codigo = 2;
            respuesta->letra = 'F';
            char *buff = dserializar_lista_nodos(nodos, &leng);
            respuesta->sizeData = leng;

            mensajeEnviar = createMessage(respuesta, buff);

            send(nuevo_socket, mensajeEnviar->buffer, mensajeEnviar->sizeBuffer, 0);
            yamasock = nuevo_socket;
            retornar = 1;
            //todo
            //free(mensajeEnviar->buffer);
            //free(mensajeEnviar);

        } else {

            log_info(logi, "Estado No estable - rechazar YAMA");
            respuesta->codigo = 0;
            respuesta->letra = 'F';
            respuesta->sizeData = 0;
            mensajeEnviar = createMessage(respuesta, "");

            send(nuevo_socket, mensajeEnviar->buffer, mensajeEnviar->sizeBuffer, 0);
            retornar = 0;

            free(mensajeEnviar->buffer);
            free(mensajeEnviar);
        }

    } else if (identificacion->letra == 'W') {

        if (configuracion->estado_estable) {

            log_info(logi, "Se conecto un WORKER");

            respuesta->codigo = 0;
            respuesta->letra = 'F';
            respuesta->sizeData = 0;
            mensajeEnviar = createMessage(respuesta, "");

            send(nuevo_socket, mensajeEnviar->buffer, mensajeEnviar->sizeBuffer, 0);
            retornar = 1;
            free(mensajeEnviar->buffer);
            free(mensajeEnviar);

        } else {

            log_info(logi, "Estado No estable - rechazar WORKER");
            respuesta->codigo = 1;
            respuesta->letra = 'F';
            respuesta->sizeData = 0;
            mensajeEnviar = createMessage(respuesta, "");

            send(nuevo_socket, mensajeEnviar->buffer, mensajeEnviar->sizeBuffer, 0);
            retornar = 0;

            free(mensajeEnviar->buffer);
            free(mensajeEnviar);
        }

    } else if (identificacion->letra == 'D') {

        //checkStateNodos();
        log_info(logi, "Se conectó DATA_NODE");

        size_t leng;
        t_nodo *nodo_conectado = deserializar_nodo(buff, &leng);
        respuesta->codigo = 0;
        respuesta->letra = 'F';
        respuesta->sizeData = sizeof(int);
        int resuesta = 0;
        NODO *nuevo_nodo = malloc(sizeof(NODO));

        nuevo_nodo->soket = nuevo_socket;
        nuevo_nodo->puerto = nodo_conectado->puerto;
        nuevo_nodo->ip = strdup(nodo_conectado->ip);
        nuevo_nodo->nombre = strdup(nodo_conectado->nodo);

        char *buffer = malloc(sizeof(int));
        if (configuracion->inicio_limpio == 1) {

            nuevo_nodo->espacio_total = nodo_conectado->sizeDatabin;
            nuevo_nodo->espacio_libre = nodo_conectado->sizeDatabin;
            nuevo_nodo->estado = disponible;

            configuracion->espacio_total += nuevo_nodo->espacio_total;
            configuracion->espacio_libre += nuevo_nodo->espacio_total;

            list_add(nodos, nuevo_nodo);
            resuesta = 1;
            retornar = 1;
        } else {
            if (exitProcess(nuevo_nodo)) {
                if (checkStateFileSystem() == disponible) {
                    configuracion->estado_estable = 1;
                } else {
                    configuracion->estado_estable = 0;
                }
                puts("----------------------");
                printf("numero estable --> %d",configuracion->estado_estable);
                puts("----------------------");
                checkStateNodos();
                resuesta = 1;
                retornar = 1;
            }
        }
        memcpy(buffer, &resuesta, sizeof(int));
        mensajeEnviar = createMessage(respuesta, buffer);

        enviar_message(nuevo_socket, mensajeEnviar, logi, &resuesta);

        free(nodo_conectado->ip);
        free(nodo_conectado->nodo);
        free(nodo_conectado);
        free(mensajeEnviar->buffer);
        free(mensajeEnviar);
        free(buff);

    }

    free(identificacion);
    free(respuesta);
    log_info(logi, "HANDSHAKE");
    return retornar;
}

void atender_mensaje_YAMA(int codigo, void *mensaje) {

    //printf("mensaje:%s", (char *) mensaje);
    switch (codigo) {

        case 0:
            break;
        case 1:
            break;
        case 2:
            break;
        case 5: {
            t_list *listi = list_create();
            bloqueArchivo b;
            b.nodo0 = strdup("nodo_1");
            b.nodo1 = strdup("nodo_2");
            b.bloquenodo0 = 0;
            b.bloquenodo1 = 0;
            b.bytesEnBloque = 1048576;
/*
		bloqueArchivo bc;
		bc.nodo0 = strdup("nodo_1");
		bc.nodo1 = strdup("nodo_2");
		bc.bloquenodo0 = 4;
		bc.bloquenodo1 = 5;
		bc.bytesEnBloque= 1048576;
*/
            list_add(listi, &b);
            //	list_add(listi,&bc);
            size_t j;
            char *hola = dserializar_list_bloque_archivo(listi, &j);
            header h;
            h.codigo = 3;
            h.letra = 'F';
            h.sizeData = j;

            message *n = create_Message(&h, hola);
            int c = 0;
            enviar_message(yamasock, n, logi, &c);
            break;
        }

    }
}

void atender_mensaje_NODO(int codigo, void *mensaje) {

    printf("mensaje:%s", (char *) mensaje);
    switch (codigo) {

        case 1: // ESCRIBIR ARCHIVO
            break;
        case 2: // LEER ARCHIVO
            break;

    }
}

message *create_Message(header *head, void *data) {
    message *ElMensaje = malloc(sizeof(message));
    ElMensaje->sizeBuffer = (sizeof(header) + head->sizeData);
    ElMensaje->buffer = malloc(ElMensaje->sizeBuffer);
    memcpy(ElMensaje->buffer, head, sizeof(header));
    memcpy((ElMensaje->buffer + sizeof(header)), data, head->sizeData);
    return ElMensaje;
}
