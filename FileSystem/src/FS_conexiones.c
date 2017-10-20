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
#include <funcionesCompartidas/serializacion_yama_master.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "estructurasfs.h"


extern yamafs_config *configuracion;
extern t_list *nodos;
extern t_log *logi;
fd_set master;
fd_set read_fds;
int fdmax;
int yamasock;

void manejo_conexiones()
{


	log_info(logi,"Iniciando administrador de conexiones");

	char *puerto= string_itoa(configuracion->puerto);
	int control=0;
	if ((fdmax = configuracion->serverfs = makeListenSock(puerto,logi,&control)) < 0){
		perror("Error en algo de sockets %s\n");
		free(puerto);
		pthread_exit((void *)-1);
	}
	free(puerto);
	//Seteo en 0 el master y temporal
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	//Cargo el socket server
	FD_SET(configuracion->serverfs, &master);

	//Bucle principal
	while (1)
	{
		read_fds = master;

		int selectResult = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
		log_info(logi,"Actividad detectada en administrador de conexiones");

		if (selectResult == -1)
		{
			log_info(logi,"Error en el administrador de conexiones");
			break;

		}
		else
		{
			//Recorro los descriptores para ver quien llamo
			int i;
			for (i = 0; i <= fdmax; i++)
			{
				if (FD_ISSET(i, &read_fds))
				{
					//Se detecta alguien nuevo llamando?
					if (i == configuracion->serverfs)
					{
						//Gestiono la conexion entrante
						int nuevo_socket = aceptar_conexion(configuracion->serverfs,logi,&control);
						//Controlo que no haya pasado nada raro y acepto al nuevo

						int exitoso =realizar_handshake(nuevo_socket);
						if(exitoso == 1){
							//Cargo la nueva conexion a la lista y actualizo el maximo
							FD_SET(nuevo_socket, &master);

							if (nuevo_socket > fdmax){
								fdmax = nuevo_socket;
							}
						}else{
							close(nuevo_socket);
						}
					}
					else
					{
						int estado = direccionar(i);
						if(estado ==  -1){
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
    header *header_mensaje = malloc(sizeof(header));
    char *mensaje = getMessage(socket_rec, header_mensaje, &status);

    if (status == -1) {
        perror("Error recibiendo");
    } else if (status == 0) {
        log_info(logi, "Se desconecto socket");
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

    free(mensaje);
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

            respuesta->codigo = 0;
            respuesta->letra = 'F';
            respuesta->sizeData = 0;
            mensajeEnviar = createMessage(respuesta, "");

            send(nuevo_socket, mensajeEnviar->buffer, mensajeEnviar->sizeBuffer, 0);
            yamasock = nuevo_socket;
            retornar = 1;
            free(mensajeEnviar->buffer);
            free(mensajeEnviar);

        } else {

            log_info(logi, "Estado No estable - rechazar YAMA");
            respuesta->codigo = 1;
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

        log_info(logi, "Se conectó DATA_NODE");

        size_t leng;
        t_nodo *nodo_conectado = deserializar_nodo(buff, &leng);

        if (configuracion->inicio_limpio == 1) {


            respuesta->codigo = 0;
            respuesta->letra = 'F';
            respuesta->sizeData = sizeof(int);
            int resuesta = 1;
            char *buffer = malloc(sizeof(int));
            memcpy(buffer, &resuesta, sizeof(int));
            mensajeEnviar = createMessage(respuesta, buffer);

            enviar_message(nuevo_socket, mensajeEnviar, logi, &resuesta);
            free(buffer);
            NODO *nuevo_nodo = malloc(sizeof(NODO));
            nuevo_nodo->soket = nuevo_socket;
            nuevo_nodo->puerto = nodo_conectado->puerto;
            nuevo_nodo->ip = strdup(nodo_conectado->ip);
            nuevo_nodo->nombre = strdup(nodo_conectado->nodo);
            nuevo_nodo->espacio_total = nodo_conectado->sizeDatabin;
            nuevo_nodo->espacio_libre = nodo_conectado->sizeDatabin;

            configuracion->espacio_total += nuevo_nodo->espacio_total;
            configuracion->espacio_libre += nuevo_nodo->espacio_total;

            list_add(nodos, nuevo_nodo);
            retornar = 1;

            free(nodo_conectado->ip);
            free(nodo_conectado->nodo);
            free(nodo_conectado);
            free(mensajeEnviar->buffer);
            free(mensajeEnviar);
            free(buff);

        } else {

            //chequear que sea un nodo conectado con anterioridad
            // si lo es aceptarlo si no rechazarlo
        }

    }

    free(identificacion);
    free(respuesta);
    log_info(logi, "HANDSHAKE");
    return retornar;
}

void atender_mensaje_YAMA(int codigo, void *mensaje) {

    printf("mensaje:%s", (char *) mensaje);
    switch (codigo) {

        case 0:
            break;
        case 1:
            break;
        case 2:
            break;

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
