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
#include <funcionesCompartidas/generales.h>
#include "estructurasfs.h"
#include "FS_administracion.h"
#define MAXEXCLUDE 30

extern yamafs_config *configuracion;
extern t_list *nodos;
extern t_log *logi;
extern t_list *archivos;
extern pthread_mutex_t mutex_socket;
fd_set master;
fd_set read_fds;

int socketExclude[MAXEXCLUDE];
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

bool socketExcluido(int socket) {
    int i;
    for (i = 0; i < MAXEXCLUDE; i++) {
        if (socketExclude[i] == socket) {
            return true;
        }
    }
    return false;
}

void InicializarArrayEcluido(){
    int i;
    for (i = 0; i < MAXEXCLUDE; i++) {
        socketExclude[i] = -1;
    }
}

void manejo_conexiones() {
    InicializarArrayEcluido();

    log_info(logi, "Iniciando administrador de conexiones");

    char *puerto = string_itoa(configuracion->puerto);
    int control = 0;
    if ((fdmax = configuracion->serverfs = makeListenSock(puerto, logi, &control)) < 0) {
       // perror("Error en algo de sockets %s\n");
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
                if (!socketExcluido(i) && FD_ISSET(i, &read_fds)) {
                    //Se detecta alguien nuevo llamando?
                    if (i == configuracion->serverfs) {
                       // puts("nueva conexcion");
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
    int status;
    header header_mensaje;
    //pthread_mutex_lock(&mutex_socket);
    char *mensaje = getMessageIntr(socket_rec, &header_mensaje, &status);
    //pthread_mutex_unlock(&mutex_socket);

    if (status == -1) {
        //	perror("Error recibiendo");
    } else if (status == 0) {
       // log_info(logi, "Se desconecto socket");
        disconnectedNodo(socket_rec);
        return -1;
    } else {
        if (header_mensaje.letra == 'Y') {
            atender_mensaje_YAMA(header_mensaje.codigo, mensaje);
        } else if (header_mensaje.letra == 'N') {
            atender_mensaje_NODO(header_mensaje.codigo, mensaje);
        } else if (header_mensaje.letra == 'W') {
            atender_mensaje_WORKER(header_mensaje.codigo, mensaje, socket_rec);
        } else {
            // no entiendo emisor/mensaje
        }
    }

    if (mensaje) free(mensaje);

    return status;
}

int realizar_handshake(int nuevo_socket) {

    int retornar, control;
    header *identificacion = malloc(sizeof(header));
    header *respuesta = malloc(sizeof(header));
    memset(respuesta, 0, sizeof(header));
    memset(identificacion, 0, sizeof(header));
    message *mensajeEnviar = NULL;
    void *bufferRequest = getMessage(nuevo_socket, identificacion, &control);
    void *bufferResponse = NULL;

    switch (identificacion->letra) {
        case 'Y': {
            // Si estable, aceptar YAMA
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
                free(buff);

            } else {

                log_info(logi, "Estado No estable - rechazar YAMA");
                respuesta->codigo = 0;
                respuesta->letra = 'F';
                respuesta->sizeData = 0;
                mensajeEnviar = createMessage(respuesta, "");

                send(nuevo_socket, mensajeEnviar->buffer, mensajeEnviar->sizeBuffer, 0);
                retornar = 0;
            }

            break;
        }
        case 'W': {
            if (configuracion->estado_estable) {

                log_info(logi, "Se conecto un WORKER");

                respuesta->codigo = 0;
                respuesta->letra = 'F';
                respuesta->sizeData = 1;
                mensajeEnviar = createMessage(respuesta, "");

                send(nuevo_socket, mensajeEnviar->buffer, mensajeEnviar->sizeBuffer, 0);
                retornar = 1;

            } else {

                log_info(logi, "Estado No estable - rechazar WORKER");
                respuesta->codigo = 1;
                respuesta->letra = 'F';
                respuesta->sizeData = 0;
                mensajeEnviar = createMessage(respuesta, "");
                retornar = 0;

                send(nuevo_socket, mensajeEnviar->buffer, mensajeEnviar->sizeBuffer, 0);
            }
            break;
        }
        case 'D': {

            log_info(logi, "Se conectó DATA_NODE");
            size_t leng;
            int acceptNodo = 1;
            retornar = 1;
            estado stateFile;

            t_nodo *nodo_conectado = deserializar_nodo(bufferRequest, &leng);

            bufferResponse = malloc(sizeof(int));

            respuesta->codigo = 0;
            respuesta->letra = 'F';
            respuesta->sizeData = sizeof(int);

            NODO *nuevo_nodo = malloc(sizeof(NODO));
            nuevo_nodo->soket = nuevo_socket;
            nuevo_nodo->puerto = nodo_conectado->puerto;
            nuevo_nodo->ip = strdup(nodo_conectado->ip);
            nuevo_nodo->nombre = strdup(nodo_conectado->nodo);

            if (configuracion->inicio_limpio) {

                nuevo_nodo->espacio_total = nodo_conectado->sizeDatabin;
                nuevo_nodo->espacio_libre = nodo_conectado->sizeDatabin;
                nuevo_nodo->estado = disponible;

                if (configuracion->estado_estable) {
                    if (!searchNodoInList(nuevo_nodo)) {
                        // En un inicio limpio con el file system ya estable, si no es un nodo previamente conectado, lo rechazo

                        free(nuevo_nodo->ip);
                        free(nuevo_nodo->nombre);
                        free(nuevo_nodo);

                        acceptNodo = 0;
                        retornar = 0;
                    }
                } else {

                	if (!searchNodoInList(nuevo_nodo)){
                		configuracion->espacio_total += nuevo_nodo->espacio_total;
                		configuracion->espacio_libre += nuevo_nodo->espacio_total;
                		list_add(nodos, nuevo_nodo);
                	}

                }


            } else {

                if (searchNodoInList(nuevo_nodo)) {

                    stateFile = checkStateFileSystem();

                    free(nuevo_nodo->ip);
                    free(nuevo_nodo->nombre);
                    free(nuevo_nodo);

                    if (!configuracion->estado_estable) {
                        configuracion->estado_estable = stateFile;
                    }

                } else {

                    free(nuevo_nodo->ip);
                    free(nuevo_nodo->nombre);
                    free(nuevo_nodo);

                    retornar = 0;
                    acceptNodo = 0;
                }
            }

            memcpy(bufferResponse, &acceptNodo, sizeof(int));
            mensajeEnviar = createMessage(respuesta, bufferResponse);
            enviar_message(nuevo_socket, mensajeEnviar, logi, &control);

            free(nodo_conectado->ip);
            free(nodo_conectado->nodo);
            free(nodo_conectado);

            break;
        }
        default: {
            retornar = 0;
            break;
        }
    }

    free(mensajeEnviar->buffer);
    free(mensajeEnviar);

    free(bufferRequest);
    free(bufferResponse);
    free(respuesta);
    free(identificacion);

    log_info(logi, "HANDSHAKE");

    return retornar;
}

void atender_mensaje_YAMA(int codigo, void *mensaje) {

    switch (codigo) {

        case 0:
            break;
        case 1:
            break;
        case 2:
            break;
        case 5: {

            int ctrl = 0;
            message *mensajje;
            header h;
            h.codigo = 3;
            h.letra = 'F';

            t_archivo *ElArchivo = get_metadata_archivo(mensaje);
            if (ElArchivo == NULL) {
                log_info(logi, "Yama pidio información de un archivo inexistente: %s", mensaje);

                h.sizeData = 0;
                h.codigo = 4;

                mensajje= create_Message(&h, "");
                enviar_message(yamasock, mensajje, logi, &ctrl);

            } else {

                log_info(logi, "Yama pidio información de un archivo: %s", mensaje);
                size_t j;

                char *bloques_serializados = dserializar_list_bloque_archivo(ElArchivo->bloques, &j);
                h.sizeData = j;

                mensajje = create_Message(&h, bloques_serializados);
                enviar_messageIntr(yamasock, mensajje, logi, &ctrl);

                free(bloques_serializados);

            }

            free(mensajje->buffer);
            free(mensajje);
            break;
        }

    }
}

void atender_mensaje_NODO(int codigo, void *mensaje) {

    switch (codigo) {

        case 1: // ESCRIBIR ARCHIVO
            break;
        case 2: // LEER ARCHIVO
            break;

    }
}

void atender_mensaje_WORKER(int codigo, void *mensaje, int socketWorker) {

    switch (codigo) {
        case 9: {
            t_file *fileReduccionGlobal = deserializar_File(mensaje);
            int resultSaveFile = 1;
            header headResponse;
            headResponse.codigo = 14;
            headResponse.sizeData = sizeof(int);
            headResponse.letra = 'F';

            char **dirnomb = sacar_archivo(fileReduccionGlobal->fname);
            int padre = existe_ruta_directorios(dirnomb[0]);
            if (padre == -9) {
                resultSaveFile = 0;
            } else if (existe_archivo(dirnomb[1], padre)) {
                resultSaveFile = 0;
            } else if (!hay_lugar_para_archivo(fileReduccionGlobal->fsize)) {
                resultSaveFile = 0;
            } else {
                FILE *ar = fopen("/tmp/almacenado_final.txt", "w");
                fwrite(fileReduccionGlobal->data, (size_t) fileReduccionGlobal->fsize, 1, ar);
                fclose(ar);

                t_list *ba = escribir_desde_archivo("/tmp/almacenado_final.txt", 'T', fileReduccionGlobal->fsize);


                t_archivo *arch = malloc(sizeof(t_archivo));
                arch->tipo = strdup("t");
                arch->bloques = ba;
                arch->cantbloques = list_size(ba);
                arch->estado = disponible;
                arch->index_padre = padre;
                arch->nombre = strdup(dirnomb[1]);
                arch->tamanio = fileReduccionGlobal->fsize;

                list_add(archivos, arch);
                crear_metadata_archivo(arch);

                unlink("/tmp/almacenado_final.txt");
                log_info(logi, "Archivo Final Almacenado");

            }
            free(fileReduccionGlobal->fname);
            free(fileReduccionGlobal->data);
            free(fileReduccionGlobal);


            message *response = createMessage(&headResponse, &resultSaveFile);
            log_info(logi, "Enviando mensaje de confirmacion");
            if (send(socketWorker, response->buffer, response->sizeBuffer, 0) == -1) {
                log_error(logi, "Error al enviar al worker save reduccionGlobal");
            }
            free(response->buffer);
            free(response);
            liberar_char_array(dirnomb);

            break;
        }
        default: {
            log_info(logi, "Codigo no reconocido worker");
        }
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

void activarSelect() {
    int control;
    char *puerto = string_itoa(configuracion->puerto);
    int activar = establecerConexion("127.0.0.1", puerto, logi, &control);
    close(activar);
    free(puerto);
}

void liberarSocket(int socket) {
    int i;
    FD_CLR(socket, &master);
    FD_CLR(socket, &read_fds);
    for (i = 0; i < MAXEXCLUDE; i++) {
        if (socketExclude[i] == -1) {
            socketExclude[i] = socket;
            break;
        }
    }
}

void incorporarSocket(int socket) {
    int i;
    FD_SET(socket, &master);
    FD_SET(socket, &read_fds);
    for (i = 0; i < MAXEXCLUDE; i++) {
        if (socketExclude[i] == socket) {
            socketExclude[i] = -1;
            break;
        }
    }
}
