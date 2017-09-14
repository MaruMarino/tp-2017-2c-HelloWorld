/*
 * FS_conexiones.c
 *
 *  Created on: 8/9/2017
 *      Author: utnso
 */

#include "FS_conexiones.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <sys/socket.h>
#include <commons/string.h>
#include <commons/collections/queue.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/estructuras.h>

#include "estructurasfs.h"


extern yamafs_config *configuracion;
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
						if(exitoso == 0){
							//es yama y no estoy en condiciones de aceptarlo
							//enviar mensaje de rechazo
						}else{
							//Cargo la nueva conexion a la lista y actualizo el maximo
							FD_SET(nuevo_socket, &master);

							if (nuevo_socket > fdmax)
							{
								fdmax = nuevo_socket;
							}
						}
					}
					else
					{
						int estado = direccionar(i);
						if(estado ==  -1){
							break;
						}else{
							FD_CLR(i, &master);
							continue;
						}
					}
				}
			}
		}
	}
}

int direccionar(int socket_rec)
{
	log_info(logi,"DIRECCIONANDO . . .");
	char *mensaje = malloc(13);
	int status = recv(socket_rec, mensaje, 13, MSG_WAITALL);

	if (status == -1){
		perror("Error recibiendo");
	} else if (status == 0){
		log_info(logi,"Se desconecto socket");
	}else{
		log_info(logi,mensaje);
	}
	free(mensaje);

	return status;
}
int realizar_handshake(int nuevo_socket){

	char *identificacion = malloc(13);
	recv(nuevo_socket,identificacion,13,MSG_WAITALL);
	if(!strncmp(identificacion,"Y00",3)){
		// Se conecto YAMA
		// if estado estable lo acepto si no bai perra
		log_info(logi,"Se conecto YAMA");
		char *respuesta= strdup("F000000000000");
		send(nuevo_socket,respuesta,13,0);
		free(respuesta);

		yamasock = nuevo_socket;
		// crear info del yama
	}else if(!strncmp(identificacion,"N00",3)){
		// se conectó un data node
		// inicializar nodo
		log_info(logi,"Se conectó DATA_NODE");

		char *respuesta= strdup("F000000000000");
		send(nuevo_socket,respuesta,13,0);
		free(respuesta);
	}
	free(identificacion);
	log_info(logi,"HANDSHAKE");
	return 1;
}
