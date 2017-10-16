#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <funcionesCompartidas/serializacion_yama_master.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/serializacion.h>
#include <funcionesCompartidas/mensaje.h>
#include <funcionesCompartidas/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include "func_estadisticas.h"
#include "estructuras.h"

extern t_configuracion *config;
extern t_log *log_Mas;
extern t_list *hilos;

void error_reduccion_local(t_redLocal *reduccion_local);
void error_transformacion(t_transformacion *transf);
void ejecutar_transformador(t_transformacion *transf);
void atender_tranformacion(t_list *list_transf);
void atender_reduccion_local(t_redLocal *reduccion_local);

void escuchar_peticiones()
{
	int controlador;
	header *head = malloc(sizeof(head));

	while(1)
	{
		void *buffer = getMessage(config->socket_yama, head, &controlador);

		switch(head->codigo)
		{
			case 1: ;
				escribir_log(log_Mas, "Se recibio una/s peticion/es de transformacion");
				t_list *list_transf = deserializar_lista_transformacion(buffer);
				atender_tranformacion(list_transf);
				break;
			case 2: ;
				escribir_log(log_Mas, "Se recibio una peticion de reduccion local");
				t_redLocal *reduccion_local = deserializar_redLocal(buffer);
				atender_reduccion_local(reduccion_local);
				break;
			default:
				puts("default");
				break;
		}
		free(buffer);
	}
}

void atender_tranformacion(t_list *list_transf)
{
	int i;
	int size = list_size(list_transf);

	for(i=0; i<size; i++)
	{
		pthread_t hiloPrograma = malloc(sizeof(pthread_t));

		t_transformacion *transf = list_get(list_transf,i);
		pthread_create(&hiloPrograma,NULL,(void*)ejecutar_transformador,transf);

		list_add(hilos, hiloPrograma);
	}
}

void ejecutar_transformador(t_transformacion *transf)
{
	int controlador = 0;
	int socket_local;
	header *header = malloc(sizeof(header));

	agregar_transformacion();

	//Conecto con Worker
	char *port = string_itoa(transf->nodo->puerto);
	socket_local = establecerConexion(transf->nodo->ip, port, log_Mas, &controlador);
	free(port);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error conectandose a Worker: ", transf->nodo->nodo);
		error_transformacion(transf);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Conectado a Worker: ",transf->nodo->nodo);

	//Realizacion HandShake
	header->letra = 'M';
	header->codigo = 0;
	header->sizeData = 0;

	message *mensj_hand = createMessage(header, "");
	enviar_message(socket_local, mensj_hand, log_Mas, &controlador);
	free(mensj_hand);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando HandShake Worker: ", transf->nodo->nodo);
		error_transformacion(transf);
		free(header);
		return;
	}

	getMessage(socket_local, header, &controlador);

	if((controlador > 0)||(header->codigo != 0))
	{
		escribir_log_error_compuesto(log_Mas, "Error recibiendo HandShake Worker: ", transf->nodo->nodo);
		error_transformacion(transf);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "HandShake exitoso con Worker: ",transf->nodo->nodo);

	//Enviando transformacion a procesar
	t_info_trans *transf_work = malloc(sizeof(t_info_trans));

	transf_work->prog = config->script_trans;
	transf_work->size_prog = string_length(config->script_trans);
	transf_work->bloque = transf->bloque;
	transf_work->bytes_ocup = transf->bytes;
	transf_work->file_out = transf->temporal;

	size_t len_total;
	char *buffer_trans = serializar_info_trans(transf_work, &len_total);

	header->letra = 'M';
	header->codigo = 1;
	header->sizeData = len_total;

	message *mensj_trans = createMessage(header, buffer_trans);
	enviar_message(socket_local, mensj_trans, log_Mas, &controlador);

	free(transf_work);
	free(buffer_trans);
	free(mensj_trans);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando transformacion a Worker: ", transf->nodo->nodo);
		error_transformacion(transf);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Transformacion enviada a Worker: ",transf->nodo->nodo);

	//Recibo respuesta de Worker
	void *buffer_rta = getMessage(socket_local, header, &controlador);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Fallo recibir mensaje de estado transformacion de Worker: ", transf->nodo->nodo);
		error_transformacion(transf);
		free(header);
		return;
	}
	else if(header->codigo == 8)
	{
		escribir_log_error_compuesto(log_Mas, "Error en procesamiento de transformacion de Worker: ", transf->nodo->nodo);
		error_transformacion(transf);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Respuesta estado recibida de Worker: ",transf->nodo->nodo);

	t_estado_master *t_estado = malloc(sizeof(t_estado_master));

	t_estado->estado = 2;
	t_estado->bloque = transf->bloque;
	t_estado->nodo = transf->nodo->nodo;

	header->letra = 'M';
	header->codigo = 5;

	char *estado_a_enviar = serializar_estado_master(t_estado, &header->sizeData);

	message *mensj_transf_est = createMessage(header, buffer_rta);
	enviar_message(config->socket_yama, mensj_transf_est, log_Mas, &controlador);

	if(controlador > 0)
	{
		escribir_log_error(log_Mas, "Fallo enviar mensaje de estado a YAMA");
		error_transformacion(transf);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Proceso transformacion finalizada para Worker: ",transf->nodo->nodo);

	quitar_transformacion();

	free(buffer_rta);
	free(t_estado);
	free(mensj_transf_est);
	free(header);
}

void error_transformacion(t_transformacion *transf)
{
	int controlador;
	size_t len_total;

	t_estado_master *t_estado = malloc(sizeof(t_estado_master));
	t_estado->bloque = transf->bloque;
	t_estado->estado = 3;
	t_estado->nodo = transf->nodo->nodo;

	char *serializado = serializar_estado_master(t_estado, &len_total);

	header *header_d = malloc(sizeof(header));
	header_d->letra = 'M';
	header_d->codigo = 5;
	header_d->sizeData = len_total;

	message *mensj_error = createMessage(header_d, serializado);
	enviar_message(config->socket_yama, mensj_error, log_Mas, &controlador);

	//void *buffer = getMessage(config->socket_yama, header_d, &controlador);

	agregar_fallo_transf();
	matar_hilos();

	free(serializado);
	free(mensj_error);
	free(t_estado);
	free(header_d);
}

void atender_reduccion_local(t_redLocal *reduccion_local)
{
	//debo recibir lista, con n cantidad de peticiones de reduccion
	int controlador = 0;
	int socket_local;
	header *header = malloc(sizeof(header));

	agregar_reduccion();

	//Conecto con Worker
	char *port = string_itoa(reduccion_local->nodo->puerto);
	socket_local = establecerConexion(reduccion_local->nodo->ip, port, log_Mas, &controlador);
	free(port);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error conectandose a Worker: ", reduccion_local->nodo->nodo);
		error_reduccion_local(reduccion_local);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Conectado a Worker: ",reduccion_local->nodo->nodo);

	//Realizacion HandShake
	header->letra = 'M';
	header->codigo = 0;
	header->sizeData = 0;

	message *mensj_hand = createMessage(header, "");
	enviar_message(socket_local, mensj_hand, log_Mas, &controlador);
	free(mensj_hand);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando HandShake Worker: ", reduccion_local->nodo->nodo);
		error_reduccion_local(reduccion_local);
		free(header);
		return;
	}

	getMessage(socket_local, header, &controlador);

	if((controlador > 0)||(header->codigo != 0))
	{
		escribir_log_error_compuesto(log_Mas, "Error recibiendo HandShake Worker: ", reduccion_local->nodo->nodo);
		error_reduccion_local(reduccion_local);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "HandShake exitoso con Worker: ",reduccion_local->nodo->nodo);

	//Enviando reduccion local para procesar
	t_info_redLocal *reduc_loc_work = malloc(sizeof(t_info_redLocal));

	reduc_loc_work->file_out = reduccion_local->temp_red_local;


	size_t len_total;
	char *buffer_trans = serializar_info_redLocal(reduc_loc_work, &len_total);

	header->letra = 'M';
	header->codigo = 2;
	header->sizeData = len_total;

	message *mensj_trans = createMessage(header, buffer_trans);
	enviar_message(socket_local, mensj_trans, log_Mas, &controlador);

	free(buffer_trans);
	free(mensj_trans);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando reduccion local a Worker: ", reduccion_local->nodo->nodo);
		error_reduccion_local(reduccion_local);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Transformacion enviada a Worker: ",transf->nodo->nodo);

	//Recibo respuesta de Worker
	void *buffer_rta = getMessage(socket_local, header, &controlador);

	//!!!!!!!!!!!!
	//PREGUNTAR IÑAKI QUE CODIGO LLEGARA!!!!!!!!!
	//!!!!!!!!!!!!
	if((controlador > 0)||(header->codigo != 0))
	{
		escribir_log_error_compuesto(log_Mas, "Error respuesta estado transformacion de Worker: ", transf->nodo->nodo);
		error_transformacion(transf);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Respuesta estado recibida de Worker: ",transf->nodo->nodo);

	t_estado_master *t_estado = deserializar_estado_master(buffer_rta);

	header->letra = 'M';
	header->codigo = 5;
	//header->sizeData = len_total;

	message *mensj_transf_est = createMessage(header, buffer_rta);
	enviar_message(config->socket_yama, mensj_transf_est, log_Mas, &controlador);

	escribir_log_compuesto(log_Mas, "Proceso transformacion finalizada para Worker: ",transf->nodo->nodo);

	quitar_reduccion_local();

	free(buffer_rta);
	free(t_estado);
	free(mensj_transf_est);
	free(header);
}

void error_reduccion_local(t_redLocal *reduccion_local)
{

}

void matar_hilos()
{
	void _destruir_elemento(pthread_t *el_hilo){
		pthread_cancel(el_hilo);
		free(el_hilo);
	}

	list_clean_and_destroy_elements(hilos, _destruir_elemento);

	list_destroy(hilos);
}
