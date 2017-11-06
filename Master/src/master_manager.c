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

void matar_hilos();
void error_almacenamiento(t_almacenado *almacenado);
void error_reduccion_global(t_redGlobal *reduccion_global);
void error_reduccion_local(t_redLocal *reduccion_local);
void error_transformacion(t_transformacion *transf);
void ejecutar_transformador(t_transformacion *transf);
void ejecutar_almacenamiento(t_almacenado *almacenado);
void atender_tranformacion(t_list *list_transf);
void atender_reduccion_local(t_redLocal *reduccion_local);
void atender_reduccion_global(t_list *lista_global);

void escuchar_peticiones()
{
	int controlador = 0;
	int flag_continuar = 1;
	header head;

	while((flag_continuar)&&(controlador >= 0))
	{
		char *buffer = getMessage(config->socket_yama, &head, &controlador);

		switch(head.codigo)
		{
			case 1: ;
				escribir_log(log_Mas, "Se recibio una/s peticion/es de transformacion");
				t_list *list_transf = deserializar_lista_transformacion(buffer);
				atender_tranformacion(list_transf);
				break;
			case 2: ;
				escribir_log(log_Mas, "Se recibio una peticion de reduccion local");
				t_redLocal *reduccion_local = deserializar_redLocal(buffer);
				pthread_t hiloPrograma;// = malloc(sizeof(pthread_t));
				pthread_create(&hiloPrograma,NULL,(void*)atender_reduccion_local,reduccion_local);
				list_add(hilos, &hiloPrograma);
				break;
			case 3: ;
				escribir_log(log_Mas, "Se recibio una peticion de reduccion global");
				t_list *list_red_global = deserializar_lista_redGlobal(buffer);
				atender_reduccion_global(list_red_global);
				break;
			case 4: ;
				escribir_log(log_Mas, "Se recibio una peticion de almacenamiento final");
				t_almacenado *almacenado = deserializar_almacenado(buffer);
				ejecutar_almacenamiento(almacenado);
				break;
			case 6: ;
				escribir_log(log_Mas, "Se recibio una peticion de finalizacion de Master");
				matar_hilos();
				flag_continuar = 0;
				break;
			default:
				puts("No se reconocio el mensaje recibido");
				break;
		}
		free(buffer);
	}
}

void atender_tranformacion(t_list *list_transf)
{
	int i;
	int size = list_size(list_transf);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	for(i=0; i<size; i++)
	{
		//verificar s se estan guardando bien los id de los hilos!
		pthread_t hiloPrograma;

		t_transformacion *transf = list_get(list_transf,i);

		pthread_create(&hiloPrograma,&attr,(void*)ejecutar_transformador,transf);

		list_add(hilos, &hiloPrograma);
	}

	pthread_attr_destroy(&attr);
}

void ejecutar_transformador(t_transformacion *transf)
{
	int controlador = 0;
	int socket_local;
	header *header = malloc(sizeof(header));

	agregar_transformacion();

	//Conecto con Worker
	char *port = string_itoa(transf->nodo->puerto);

	//Imprimo informacion de la peticion
	printf("Ip: %s Puerto: %s\n", transf->nodo->ip, port);//print prueba
	printf("Archivo temporak: %s\n", transf->temporal);

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

	char *matame = getMessage(socket_local, header, &controlador);

	free(matame);

	if((controlador < 0)||(header->codigo != 0))
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
	char *buffer_rta = getMessage(socket_local, header, &controlador);

	if(controlador < 0)
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
		escribir_log_compuesto(log_Mas, "Respuesta estado transformacion recibida de Worker: ",transf->nodo->nodo);

	t_estado_master *t_estado = malloc(sizeof(t_estado_master));

	t_estado->estado = 2;
	t_estado->bloque = transf->bloque;
	t_estado->nodo = transf->nodo->nodo;

	header->letra = 'M';
	header->codigo = 5;

	char *estado_a_enviar = serializar_estado_master(t_estado, &header->sizeData);

	message *mensj_transf_est = createMessage(header, estado_a_enviar);
	enviar_message(config->socket_yama, mensj_transf_est, log_Mas, &controlador);

	if(controlador > 0)
	{
		escribir_error_log(log_Mas, "Fallo enviar mensaje de estado a YAMA");
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
	t_estado->estado = 1;
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

	free(serializado);
	free(mensj_error);
	free(t_estado);
	free(header_d);
}

void atender_reduccion_local(t_redLocal *reduccion_local)
{
	int controlador = 0;
	int socket_local;
	header *header = malloc(sizeof(header));

	agregar_reduccion();

	//Conecto con Worker
	char *port = string_itoa(reduccion_local->nodo->puerto);

	//Imprimo informacion de la peticion
	printf("Ip: %s Puerto: %s\n", reduccion_local->nodo->ip, port);//print prueba
	printf("Archivo temporal: %s\n", reduccion_local->temp_red_local);

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

	char *matame = getMessage(socket_local, header, &controlador);

	free(matame);

	if((controlador < 0)||(header->codigo != 0))
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
	reduc_loc_work->files = list_create();
	reduc_loc_work->file_out = reduccion_local->temp_red_local;
	reduc_loc_work->size_prog = string_length(config->script_reduc);
	reduc_loc_work->prog = config->script_reduc;
	list_add_all(reduc_loc_work->files, reduccion_local->archivos_temp);

	size_t len_total;
	char *buffer_trans = serializar_info_redLocal(reduc_loc_work, &len_total);

	header->letra = 'M';
	header->codigo = 2;
	header->sizeData = len_total;

	message *mensj_trans = createMessage(header, buffer_trans);
	enviar_message(socket_local, mensj_trans, log_Mas, &controlador);

	free(buffer_trans);
	free(mensj_trans);
	//destruir la lista y la estructura enviada

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando reduccion local a Worker: ", reduccion_local->nodo->nodo);
		error_reduccion_local(reduccion_local);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Reduccion local enviada a Worker: ",reduccion_local->nodo->nodo);

	//Recibo respuesta de Worker
	char *buffer_rta = getMessage(socket_local, header, &controlador);

	if(controlador < 0)
	{
		escribir_log_error_compuesto(log_Mas, "Fallo recibir mensaje de estado reduccion local de Worker: ", reduccion_local->nodo->nodo);
		error_reduccion_local(reduccion_local);
		free(header);
		return;
	}
	else if(header->codigo == 8)
	{
		escribir_log_error_compuesto(log_Mas, "Error en procesamiento de reduccion local de Worker: ", reduccion_local->nodo->nodo);
		error_reduccion_local(reduccion_local);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Respuesta estado recibida de Worker: ",reduccion_local->nodo->nodo);

	t_estado_master *t_estado = malloc(sizeof(t_estado_master));

	t_estado->estado = 2;
	t_estado->bloque = reduccion_local->bloque;
	t_estado->nodo = reduccion_local->nodo->nodo;

	header->letra = 'M';
	header->codigo = 6;

	char *estado_a_enviar = serializar_estado_master(t_estado, &header->sizeData);

	message *mensj_transf_est = createMessage(header, estado_a_enviar);
	enviar_message(config->socket_yama, mensj_transf_est, log_Mas, &controlador);

	if(controlador > 0)
	{
		escribir_error_log(log_Mas, "Fallo enviar mensaje de estado a YAMA");
		error_reduccion_local(reduccion_local);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Proceso reduccion local finalizada para Worker: ",reduccion_local->nodo->nodo);

	quitar_reduccion_local();

	free(buffer_rta);
	free(t_estado);
	free(mensj_transf_est);
	free(header);
}

void error_reduccion_local(t_redLocal *reduccion_local)
{
	int controlador;
	size_t len_total;

	t_estado_master *t_estado = malloc(sizeof(t_estado_master));
	//t_estado->bloque = transf->bloque;
	t_estado->estado = 1;
	t_estado->nodo = reduccion_local->nodo->nodo;

	char *serializado = serializar_estado_master(t_estado, &len_total);

	header *header_d = malloc(sizeof(header));
	header_d->letra = 'M';
	header_d->codigo = 6;
	header_d->sizeData = len_total;

	message *mensj_error = createMessage(header_d, serializado);
	enviar_message(config->socket_yama, mensj_error, log_Mas, &controlador);

	//void *buffer = getMessage(config->socket_yama, header_d, &controlador);

	agregar_fallo_reducc_local();

	free(serializado);
	free(mensj_error);
	free(t_estado);
	free(header_d);
}

void atender_reduccion_global(t_list *lista_global)
{
	int controlador = 0;
	int socket_local;
	header *header = malloc(sizeof(header));

	agregar_reduccion_global();

	//Busco al encargado de la reduccion Global
	bool _buscar_encargado(t_redGlobal *aux){
		return aux->encargado == 1;
	}
	t_redGlobal *encargado = list_find(lista_global, (void*)_buscar_encargado);

	//Conecto con Worker
	char *port = string_itoa(encargado->nodo->puerto);

	//Imprimo informacion de la peticion
	printf("Ip: %s Puerto: %s\n", encargado->nodo->ip, port);//print prueba
	printf("Archivo temporal: %s\n", encargado->red_global);

	socket_local = establecerConexion(encargado->nodo->ip, port, log_Mas, &controlador);
	free(port);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error conectandose a Worker: ", encargado->nodo->nodo);
		error_reduccion_global(encargado);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Conectado a Worker: ", encargado->nodo->nodo);

	//Realizacion HandShake
	header->letra = 'M';
	header->codigo = 0;
	header->sizeData = 0;

	message *mensj_hand = createMessage(header, "");
	enviar_message(socket_local, mensj_hand, log_Mas, &controlador);
	free(mensj_hand);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando HandShake Worker: ", encargado->nodo->nodo);
		error_reduccion_global(encargado);
		free(header);
		return;
	}

	char *matame = getMessage(socket_local, header, &controlador);

	free(matame);

	if((controlador < 0)||(header->codigo != 0))
	{
		escribir_log_error_compuesto(log_Mas, "Error recibiendo HandShake Worker: ", encargado->nodo->nodo);
		error_reduccion_global(encargado);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "HandShake exitoso con Worker: ",encargado->nodo->nodo);

	//Armo la estructura para enviar a Worker
	t_info_redGlobal *redGlobal = malloc(sizeof(t_info_redGlobal));
	redGlobal->file_out = encargado->red_global;
	redGlobal->prog = config->script_reduc;
	redGlobal->size_prog = string_length(config->script_reduc);
	redGlobal->nodos = list_create();

	void _agregar_nodo(t_redGlobal *aux){
		t_info_nodo *nodo = malloc(sizeof(t_info_nodo));

		nodo->ip = aux->nodo->ip;
		nodo->port = string_itoa(aux->nodo->puerto);
		nodo->fname = aux->temp_red_local;

		list_add(redGlobal->nodos,nodo);
	}
	list_iterate(lista_global, (void*)_agregar_nodo);

	//Envio de pedido de reduccion Global
	size_t len_total;
	char *buffer_envio = serializar_info_redGlobal(redGlobal, &len_total);

	header->letra = 'M';
	header->codigo = 3;
	header->sizeData = len_total;

	message *mensj_reduc = createMessage(header, buffer_envio);
	enviar_message(socket_local, mensj_reduc, log_Mas, &controlador);

	free(buffer_envio);
	free(mensj_reduc);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando reduccion Global a Worker: ", encargado->nodo->nodo);
		error_reduccion_global(encargado);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Reduccion Global enviada a Worker: ",encargado->nodo->nodo);

	//Recibo respuesta de Worker
	char *buffer_rta = getMessage(socket_local, header, &controlador);

	if(controlador < 0)
	{
		escribir_log_error_compuesto(log_Mas, "Fallo recibir mensaje de estado reduccion Global de Worker: ", encargado->nodo->nodo);
		error_reduccion_global(encargado);
		free(header);
		return;
	}
	else if(header->codigo == 8)
	{
		escribir_log_error_compuesto(log_Mas, "Error en procesamiento de reduccion Global de Worker: ", encargado->nodo->nodo);
		error_reduccion_global(encargado);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Respuesta estado reduccion global recibida de Worker: ",encargado->nodo->nodo);

	t_estado_master *t_estado = malloc(sizeof(t_estado_master));

	t_estado->estado = 2;
	//t_estado->bloque = //;
	t_estado->nodo = encargado->nodo->nodo;

	header->letra = 'M';
	header->codigo = 7;

	char *estado_a_enviar = serializar_estado_master(t_estado, &header->sizeData);

	message *mensj_transf_est = createMessage(header, estado_a_enviar);
	enviar_message(config->socket_yama, mensj_transf_est, log_Mas, &controlador);

	if(controlador > 0)
	{
		escribir_error_log(log_Mas, "Fallo enviar mensaje de estado a YAMA");
		error_reduccion_global(encargado);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Proceso reduccion global finalizada para Worker: ",encargado->nodo->nodo);

	quitar_reduccion_global();

	free(buffer_rta);
	free(t_estado);
	free(mensj_transf_est);
	free(header);

	mostrar_estadisticas();
}

void error_reduccion_global(t_redGlobal *reduccion_global)
{
	int controlador;
	size_t len_total;

	t_estado_master *t_estado = malloc(sizeof(t_estado_master));
	//t_estado->bloque = transf->bloque;
	t_estado->estado = 1;
	t_estado->nodo = reduccion_global->nodo->nodo;

	char *serializado = serializar_estado_master(t_estado, &len_total);

	header *header_d = malloc(sizeof(header));
	header_d->letra = 'M';
	header_d->codigo = 7;
	header_d->sizeData = len_total;

	message *mensj_error = createMessage(header_d, serializado);
	enviar_message(config->socket_yama, mensj_error, log_Mas, &controlador);

	//void *buffer = getMessage(config->socket_yama, header_d, &controlador);

	agregar_fallo_reducc_global();

	free(serializado);
	free(mensj_error);
	free(t_estado);
	free(header_d);
}

void ejecutar_almacenamiento(t_almacenado *almacenado)
{
	int controlador = 0;
	int socket_local;
	header *header = malloc(sizeof(header));

	//Conecto con Worker
	char *port = string_itoa(almacenado->nodo->puerto);
	socket_local = establecerConexion(almacenado->nodo->ip, port, log_Mas, &controlador);
	free(port);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error conectandose a Worker: ", almacenado->nodo->nodo);
		error_almacenamiento(almacenado);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Conectado a Worker: ", almacenado->nodo->nodo);

	//Realizacion HandShake
	header->letra = 'M';
	header->codigo = 0;
	header->sizeData = 0;

	message *mensj_hand = createMessage(header, "");
	enviar_message(socket_local, mensj_hand, log_Mas, &controlador);
	free(mensj_hand);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando HandShake Worker: ", almacenado->nodo->nodo);
		error_almacenamiento(almacenado);
		free(header);
		return;
	}

	char *matame = getMessage(socket_local, header, &controlador);

	free(matame);

	if((controlador < 0)||(header->codigo != 0))
	{
		escribir_log_error_compuesto(log_Mas, "Error recibiendo HandShake Worker: ", almacenado->nodo->nodo);
		error_almacenamiento(almacenado);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "HandShake exitoso con Worker: ",almacenado->nodo->nodo);

	//Envio de pedido de reduccion Global
	size_t len_total;
	char *buffer_envio = serializar_FName(almacenado->red_global, &len_total);

	header->letra = 'M';
	header->codigo = 4;
	header->sizeData = len_total;

	message *mensj_almac = createMessage(header, buffer_envio);
	enviar_message(socket_local, mensj_almac, log_Mas, &controlador);

	free(buffer_envio);
	free(mensj_almac);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando almacenamineto a Worker: ", almacenado->nodo->nodo);
		error_almacenamiento(almacenado);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Almacenamiento enviado a Worker: ",almacenado->nodo->nodo);

	//Recibo respuesta de Worker
	char *buffer_rta = getMessage(socket_local, header, &controlador);

	if(controlador < 0)
	{
		escribir_log_error_compuesto(log_Mas, "Fallo recibir mensaje de estado Almacenado de Worker: ", almacenado->nodo->nodo);
		error_almacenamiento(almacenado);
		free(header);
		return;
	}
	else if(header->codigo == 8)
	{
		escribir_log_error_compuesto(log_Mas, "Error en procesamiento de Almacenado de Worker: ", almacenado->nodo->nodo);
		error_almacenamiento(almacenado);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Respuesta estado almacenamiento recibida de Worker: ", almacenado->nodo->nodo);

	t_estado_master *t_estado = malloc(sizeof(t_estado_master));

	t_estado->estado = 2;
	//t_estado->bloque = //;
	t_estado->nodo = almacenado->nodo->nodo;

	header->letra = 'M';
	header->codigo = 8;

	char *estado_a_enviar = serializar_estado_master(t_estado, &header->sizeData);

	message *mensj_transf_est = createMessage(header, estado_a_enviar);
	enviar_message(config->socket_yama, mensj_transf_est, log_Mas, &controlador);

	if(controlador > 0)
	{
		escribir_error_log(log_Mas, "Fallo enviar mensaje de estado a YAMA");
		error_almacenamiento(almacenado);
		free(header);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Proceso Almacenado finalizado para Worker: ", almacenado->nodo->nodo);

	quitar_almacenamiento();

	free(buffer_rta);
	free(t_estado);
	free(mensj_transf_est);
	free(header);
}

void error_almacenamiento(t_almacenado *almacenado)
{
	int controlador;
	size_t len_total;

	t_estado_master *t_estado = malloc(sizeof(t_estado_master));
	//t_estado->bloque = transf->bloque;
	t_estado->estado = 1;
	t_estado->nodo = almacenado->nodo->nodo;

	char *serializado = serializar_estado_master(t_estado, &len_total);

	header *header_d = malloc(sizeof(header));
	header_d->letra = 'M';
	header_d->codigo = 8;
	header_d->sizeData = len_total;

	message *mensj_error = createMessage(header_d, serializado);
	enviar_message(config->socket_yama, mensj_error, log_Mas, &controlador);

	//void *buffer = getMessage(config->socket_yama, header_d, &controlador);

	agregar_fallo_reducc_global();

	free(serializado);
	free(mensj_error);
	free(t_estado);
	free(header_d);
}

void matar_hilos()
{
	void _destruir_elemento(pthread_t el_hilo){
		pthread_cancel(el_hilo);
		//free(el_hilo);
	}

	list_clean_and_destroy_elements(hilos, (void*) _destruir_elemento);
	list_destroy(hilos);
}
