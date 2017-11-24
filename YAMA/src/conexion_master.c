#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/log.h>
#include <funcionesCompartidas/mensaje.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/serializacion_yama_master.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "estructuras.h"
#include "conexion_master.h"
#include "conexion_fs.h"
#include "manejo_tabla_estados.h"
#include "clocks.h"

extern t_configuracion *config;
extern t_log *yama_log;
extern t_list *masters;
extern int master_id;
fd_set master;
fd_set read_fds;
int fdmax;

void manejo_conexiones()
{
	int controlador = 0;

	escribir_log(yama_log, "Iniciando administrador de conexiones Master");
	//Seteo en 0 el master y temporal
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	//Cargo el socket server
	FD_SET(config->server_, &master);

	//Cargo el socket mas grande
	fdmax = config->server_;

	//Bucle principal
	while (1)
	{
		read_fds = master;

		selectX:;
		int selectResult = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
		escribir_log(yama_log, "Actividad detectada en administrador de conexiones");

		if (selectResult == -1)
		{
			if (errno == EINTR) goto selectX;
			escribir_error_log(yama_log, "Error en el administrador de conexiones");
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
					if (i == config->server_)
					{
						//Gestiono la conexion entrante
						escribir_log(yama_log, "Se detecto actividad en el server Master");
						int nuevo_socket = aceptar_conexion(config->server_, yama_log, &controlador);

						//Controlo que no haya pasado nada raro y acepto al nuevo
						if (controlador < 0)
							continue;

						t_master *master_ = malloc(sizeof(t_master));
						master_->master = master_id++;
						master_->socket_ = nuevo_socket;
						list_add(masters, master_);

						//Cargo la nueva conexion a la lista y actualizo el maximo
						FD_SET(nuevo_socket, &master);

						if (nuevo_socket > fdmax)
						{
							fdmax = nuevo_socket;
						}
					}
					else
					{
						manejar_respuesta(i) ;
					}
				}
			}
		}
	}
}


void manejar_respuesta(int socket_)
{
	header head;
	int status;
	char *mensaje = (char *)getMessage(socket_, &head, &status);

	if(status <= 0)
	{
		t_master *ms = find_master(socket_);
		if(ms != NULL)
			ms->socket_ += 1000;

		FD_CLR(socket_, &master);
		close(socket_);
	}

	//char *header = get_header(mensaje);
	if (head.letra == 'M' && status > 0)
	{
		//int codigo = get_codigo(mensaje);
		//char *info = get_mensaje(mensaje);
		switch (head.codigo)
		{
			case 0:; //procesar archivo -> peticion transformacion
				escribir_log(yama_log, "Se recibió archivo para procesar");
				solicitar_informacion_archivo(mensaje, socket_); //aca file me debería devolver algo
				//enviar_peticion_transformacion(socket_);
				break;
			case 5:; //reduccion local
				escribir_log(yama_log,"Se recibió el estado de una transformacion");
				t_estado_master *estado_tr = deserializar_estado_master(mensaje);
				if(estado_tr->estado == FINALIZADO_OK)
				{
					enviar_reduccion_local(estado_tr, socket_);
				}else
					replanificar(estado_tr, socket_);
				break;
			case 6:; //Reduccion global
				escribir_log(yama_log, "Se recibió estado de una reducción local");
				escribir_log(yama_log, "Chequear para empezar reduccion global");
				t_estado_master *estado_tr2 = deserializar_estado_master(mensaje);
				if(estado_tr2->estado == FINALIZADO_OK)
				{
					reduccion_global(socket_, estado_tr2);
				}
				else
				{
					escribir_error_log(yama_log, "Error en la reducción local, no se puede replanificar :(");
					matar_master(socket_);
				}
				break;
			case 8: //Respuesta almacenamiento final
				escribir_log(yama_log, "Se recibió el estado de almacenamiento final");
				t_estado_master *estado_tr8 = deserializar_estado_master(mensaje);
				t_master * ms2 = find_master(socket_);
				t_estado *est55 = get_estado(ms2->master, estado_tr8->nodo, -10, ALMACENAMIENTO_FINAL);
				if(estado_tr8->estado == FINALIZADO_OK)
					est55->estado = FINALIZADO_OK;
				else
					est55->estado = ERROR;

				matar_master(socket_);

				break;
			case 7: //Almacenamiento Final
				escribir_log(yama_log, "Se recibió el estado de una Reducción Global");
				t_estado_master *estado_tr3 = deserializar_estado_master(mensaje);
				if(estado_tr3->estado == FINALIZADO_OK)
				{
					t_master * ms = find_master(socket_);
					t_estado *est = get_estado(ms->master, estado_tr3->nodo, -10, REDUCCION_GLOBAL);
					t_worker *wk = find_worker(estado_tr3->nodo);est->estado = FINALIZADO_OK;
					t_almacenado *alma = malloc(sizeof(t_almacenado));
					alma->nodo = wk->nodo;
					alma->red_global = est->archivo_temporal;

					t_estado *esttt = generar_estado(ms->master,-10,alma->nodo->nodo,NULL,-10,-10,-10);
					esttt->archivo_temporal = est->archivo_temporal;
					esttt->etapa = ALMACENAMIENTO_FINAL;

					size_t len;
					int control;
					char *alm_ser = serializar_almacenado(alma, &len);

					header head;
					head.codigo = 4;
					head.letra = 'Y';
					head.sizeData = len;

					message *mens = createMessage(&head, alm_ser);
					enviar_message(socket_, mens, yama_log, &control);

					free(mens);
				}
				else
				{
					escribir_error_log(yama_log, "Error en la reducción global, bai");
					//finalizado error
					matar_master(socket_);
				}
				break;
			default:
				printf("default");
				break;
		}
		free(mensaje);
	} else
		log_error(yama_log, "Master desconectado");
}

void realizar_handshake_master(int socket_)
{
	int control;
	enviar(socket_, "Y000000000000000", yama_log, &control);
	manejar_respuesta(socket_);
	t_master *master = malloc (sizeof (t_master));
	master->master = master_id ++;
	master->socket_ = socket_;
	list_add(masters, master);
}

void enviar_peticion_transformacion(int socket_)
{
	t_nodo *nodo;
	t_transformacion *transformacion;
	header *head;
	message *mensaje;
	int control = 0;

	nodo = malloc(sizeof(t_nodo));
	nodo->ip = strdup("127.0.0.1");
	nodo->nodo = strdup("Nodo 1");
	nodo->puerto = 5002;

	transformacion = malloc(sizeof(t_transformacion));
	transformacion->bloque = 18;
	transformacion->bytes = 1024;
	transformacion->nodo = nodo;
	transformacion->temporal = strdup("/archivo/temporal_prueba");

	size_t size_buffer = 0;

	char *t_ser = serializar_transformacion(transformacion, &size_buffer);

	head = malloc(sizeof(header));
	head->codigo = 1;
	head->letra = 'Y';
	head->sizeData = size_buffer;

	mensaje = createMessage(head, t_ser);
	enviar_message(socket_, mensaje, yama_log, &control);

	//agregar frees
	free(head);
}

void matar_master(int socket_)
{
	recalcular_cargas(socket_);

	header head;
	head.codigo = 6;
	head.letra = 'Y';
	head.sizeData = 1;
	int control;


	message *mensaje = createMessage(&head, "");
	enviar_message(socket_, mensaje, yama_log, &control);
}
