#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
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
extern pthread_mutex_t sem_yama;
/*CALIDAD - INICIO*/
int numero_pedido_trans;
int numero_pedido_reducc;
int numero_pedido_reducc_glo;
int numero_pedido_alm;
/*CALIDAD - FIN*/

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

//borrar
void calidad_transformacion(t_list *algo);

void escuchar_peticiones()
{
	/*CALIDAD - INICIO*/
	numero_pedido_trans = 0;
	numero_pedido_reducc = 0;
	numero_pedido_reducc_glo = 0;
	numero_pedido_alm = 0;
	/*CALIDAD - FIN*/

	int controlador = 0;
	int flag_continuar = 1;
	header head;

	while((flag_continuar)&&(controlador >= 0))
	{
		char *buffer = getMessageIntr(config->socket_yama, &head, &controlador);

		switch(head.codigo)
		{
			case 1: ;
				escribir_log(log_Mas, "Se recibio una/s peticion/es de transformacion");
				t_list *list_transf = deserializar_lista_transformacion(buffer);

				calidad_transformacion(list_transf);

				atender_tranformacion(list_transf);
				break;
			case 2: ;
				escribir_log(log_Mas, "Se recibio una peticion de reduccion local");
				t_redLocal *reduccion_local = deserializar_redLocal(buffer);

				//atender_reduccion_local(reduccion_local);

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
				matar_hilos();
				finalizar_tiempos();
				flag_continuar = 0;
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
		//Guardo los id de los hilos
		pthread_t hiloPrograma;

		t_transformacion *transf = list_get(list_transf,i);

		//ejecutar_transformador(transf);
		pthread_create(&hiloPrograma,&attr,(void*)ejecutar_transformador,transf);

		list_add(hilos, &hiloPrograma);
	}
	pthread_attr_destroy(&attr);

	//mostrar_estadisticas();
}

void ejecutar_transformador(t_transformacion *transf)
{
	int controlador = 0;
	int socket_local;
	header header_;

	agregar_transformacion();

	//Conecto con Worker
	char *port = string_itoa(transf->nodo->puerto);

	//Imprimo informacion de la peticion
	printf("Ip: %s Puerto: %s\n", transf->nodo->ip, port);//print prueba
	printf("Archivo temporal: %s\n", transf->temporal);

	//Imprimo mas info
	escribir_log_con_numero(log_Mas, "Bloque: ", transf->bloque);
	escribir_log_con_numero(log_Mas, "Bytes: ", transf->bytes);
	escribir_log_compuesto(log_Mas, "Temporal: ", transf->temporal);

	socket_local = establecerConexion(transf->nodo->ip, port, log_Mas, &controlador);
	free(port);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error conectandose a Worker: ", transf->nodo->nodo);
		error_transformacion(transf);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Conectado a Worker: ",transf->nodo->nodo);

	//Realizacion HandShake
	header_.letra = 'M';
	header_.codigo = 0;
	header_.sizeData = 0;

	message *mensj_hand = createMessage(&header_, "");
	enviar_messageIntr(socket_local, mensj_hand, log_Mas, &controlador);

	free(mensj_hand->buffer);
	free(mensj_hand);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando HandShake Worker: ", transf->nodo->nodo);
		error_transformacion(transf);
		return;
	}

	char *matame = getMessageIntr(socket_local, &header_, &controlador);

	free(matame);

	if((controlador < 0)||(header_.codigo != 0))
	{
		escribir_log_error_compuesto(log_Mas, "Error recibiendo HandShake Worker: ", transf->nodo->nodo);
		error_transformacion(transf);
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

	header_.letra = 'M';
	header_.codigo = 1;
	header_.sizeData = len_total;

	message *mensj_trans = createMessage(&header_, buffer_trans);
	enviar_messageIntr(socket_local, mensj_trans, log_Mas, &controlador);

	free(transf_work);
	free(buffer_trans);
	free(mensj_trans->buffer);
	free(mensj_trans);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando transformacion a Worker: ", transf->nodo->nodo);
		error_transformacion(transf);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Transformacion enviada a Worker: ",transf->nodo->nodo);

	//Recibo respuesta de Worker
	char *buffer_rta = getMessageIntr(socket_local, &header_, &controlador);

	if(controlador < 0)
	{
		escribir_log_error_compuesto(log_Mas, "Fallo recibir mensaje de estado transformacion de Worker: ", transf->nodo->nodo);
		error_transformacion(transf);
		return;
	}
	else if(header_.codigo == 8)
	{
		escribir_log_error_compuesto(log_Mas, "Error en procesamiento de transformacion de Worker: ", transf->nodo->nodo);
		error_transformacion(transf);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Respuesta estado transformacion recibida de Worker: ",transf->nodo->nodo);

	t_estado_master *t_estado = malloc(sizeof(t_estado_master));

	t_estado->estado = 2;
	t_estado->bloque = transf->bloque;
	t_estado->nodo = transf->nodo->nodo;

	header_.letra = 'M';
	header_.codigo = 5;

	char *estado_a_enviar = serializar_estado_master(t_estado, &header_.sizeData);

	message *mensj_transf_est = createMessage(&header_, estado_a_enviar);

	pthread_mutex_lock(&sem_yama);
	enviar_messageIntr(config->socket_yama, mensj_transf_est, log_Mas, &controlador);
	pthread_mutex_unlock(&sem_yama);

	free(estado_a_enviar);

	if(controlador > 0)
	{
		escribir_error_log(log_Mas, "Fallo enviar mensaje de estado a YAMA");
		error_transformacion(transf);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Proceso transformacion finalizada para Worker: ",transf->nodo->nodo);

	quitar_transformacion();

	free(buffer_rta);
	free(t_estado);
	free(mensj_transf_est->buffer);
	free(mensj_transf_est);
	free(transf->nodo->ip);
	free(transf->temporal);
	free(transf->nodo->nodo);
	free(transf->nodo);
	free(transf);
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

	pthread_mutex_lock(&sem_yama);
	enviar_messageIntr(config->socket_yama, mensj_error, log_Mas, &controlador);
	pthread_mutex_unlock(&sem_yama);

	agregar_fallo_transf();

	free(serializado);
	free(mensj_error->buffer);
	free(mensj_error);
	free(t_estado);
	free(header_d);

	free(transf->nodo->ip);
	free(transf->temporal);
	free(transf->nodo->nodo);
	free(transf->nodo);
	free(transf);
}

void atender_reduccion_local(t_redLocal *reduccion_local)
{
	int controlador = 0;
	int socket_local;
	header header_;

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
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Conectado a Worker: ",reduccion_local->nodo->nodo);

	//Realizacion HandShake
	header_.letra = 'M';
	header_.codigo = 0;
	header_.sizeData = 0;

	message *mensj_hand = createMessage(&header_, "");
	enviar_messageIntr(socket_local, mensj_hand, log_Mas, &controlador);

	free(mensj_hand->buffer);
	free(mensj_hand);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando HandShake Worker: ", reduccion_local->nodo->nodo);
		error_reduccion_local(reduccion_local);
		return;
	}

	char *matame = getMessageIntr(socket_local, &header_, &controlador);

	free(matame);

	if((controlador < 0)||(header_.codigo != 0))
	{
		escribir_log_error_compuesto(log_Mas, "Error recibiendo HandShake Worker: ", reduccion_local->nodo->nodo);
		error_reduccion_local(reduccion_local);
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

	header_.letra = 'M';
	header_.codigo = 2;
	header_.sizeData = len_total;

	message *mensj_trans = createMessage(&header_, buffer_trans);
	enviar_messageIntr(socket_local, mensj_trans, log_Mas, &controlador);

	free(buffer_trans);
	free(mensj_trans->buffer);
	free(mensj_trans);
	//destruir la lista y la estructura enviada

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando reduccion local a Worker: ", reduccion_local->nodo->nodo);
		error_reduccion_local(reduccion_local);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Reduccion local enviada a Worker: ",reduccion_local->nodo->nodo);

	//Recibo respuesta de Worker
	char *buffer_rta = getMessageIntr(socket_local, &header_, &controlador);

	if(controlador < 0)
	{
		escribir_log_error_compuesto(log_Mas, "Fallo recibir mensaje de estado reduccion local de Worker: ", reduccion_local->nodo->nodo);
		error_reduccion_local(reduccion_local);
		return;
	}
	else if(header_.codigo == 8)
	{
		escribir_log_error_compuesto(log_Mas, "Error en procesamiento de reduccion local de Worker: ", reduccion_local->nodo->nodo);
		error_reduccion_local(reduccion_local);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Respuesta estado recibida de Worker: ",reduccion_local->nodo->nodo);

	t_estado_master *t_estado = malloc(sizeof(t_estado_master));

	t_estado->estado = 2;
	t_estado->bloque = reduccion_local->bloque;
	t_estado->nodo = reduccion_local->nodo->nodo;

	header_.letra = 'M';
	header_.codigo = 6;

	char *estado_a_enviar = serializar_estado_master(t_estado, &header_.sizeData);

	message *mensj_transf_est = createMessage(&header_, estado_a_enviar);

	pthread_mutex_lock(&sem_yama);
	enviar_messageIntr(config->socket_yama, mensj_transf_est, log_Mas, &controlador);
	pthread_mutex_unlock(&sem_yama);

	free(estado_a_enviar);

	if(controlador > 0)
	{
		escribir_error_log(log_Mas, "Fallo enviar mensaje de estado a YAMA");
		error_reduccion_local(reduccion_local);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Proceso reduccion local finalizada para Worker: ",reduccion_local->nodo->nodo);

	quitar_reduccion_local();

	free(buffer_rta);
	free(t_estado);
	free(mensj_transf_est->buffer);
	free(mensj_transf_est);

	void _limpiar(char *texto)
	{
		free(texto);
	}

	list_destroy_and_destroy_elements(reduccion_local->archivos_temp, (void*) _limpiar);
	free(reduccion_local->temp_red_local);
	free(reduccion_local->nodo->ip);
	free(reduccion_local->nodo->nodo);
	free(reduccion_local->nodo);
	free(reduccion_local);
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

	pthread_mutex_lock(&sem_yama);
	enviar_messageIntr(config->socket_yama, mensj_error, log_Mas, &controlador);
	pthread_mutex_unlock(&sem_yama);

	agregar_fallo_reducc_local();

	free(serializado);
	free(mensj_error->buffer);
	free(mensj_error);
	free(t_estado);
	free(header_d);

	void _limpiar(char *texto)
	{
		free(texto);
	}

	list_destroy_and_destroy_elements(reduccion_local->archivos_temp, (void*) _limpiar);
	free(reduccion_local->temp_red_local);
	free(reduccion_local->nodo->ip);
	free(reduccion_local->nodo->nodo);
	free(reduccion_local->nodo);
	free(reduccion_local);
}

void atender_reduccion_global(t_list *lista_global)
{
	int controlador = 0;
	int socket_local;
	header header_;

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
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Conectado a Worker: ", encargado->nodo->nodo);

	//Realizacion HandShake
	header_.letra = 'M';
	header_.codigo = 0;
	header_.sizeData = 0;

	message *mensj_hand = createMessage(&header_, "");
	enviar_messageIntr(socket_local, mensj_hand, log_Mas, &controlador);

	free(mensj_hand->buffer);
	free(mensj_hand);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando HandShake Worker: ", encargado->nodo->nodo);
		error_reduccion_global(encargado);
		return;
	}

	char *matame = getMessageIntr(socket_local, &header_, &controlador);

	free(matame);

	if((controlador < 0)||(header_.codigo != 0))
	{
		escribir_log_error_compuesto(log_Mas, "Error recibiendo HandShake Worker: ", encargado->nodo->nodo);
		error_reduccion_global(encargado);
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

	void _agregar_nodo(t_redGlobal *aux)
	{
		if(strlen(aux->temp_red_local) == 0)
			return;

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

	header_.letra = 'M';
	header_.codigo = 3;
	header_.sizeData = len_total;

	message *mensj_reduc = createMessage(&header_, buffer_envio);
	enviar_messageIntr(socket_local, mensj_reduc, log_Mas, &controlador);

	free(buffer_envio);
	free(mensj_reduc->buffer);
	free(mensj_reduc);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando reduccion Global a Worker: ", encargado->nodo->nodo);
		error_reduccion_global(encargado);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Reduccion Global enviada a Worker: ",encargado->nodo->nodo);

	//Recibo respuesta de Worker
	char *buffer_rta = getMessageIntr(socket_local, &header_, &controlador);

	if(controlador < 0)
	{
		escribir_log_error_compuesto(log_Mas, "Fallo recibir mensaje de estado reduccion Global de Worker: ", encargado->nodo->nodo);
		error_reduccion_global(encargado);
		return;
	}
	else if(header_.codigo == 8)
	{
		escribir_log_error_compuesto(log_Mas, "Error en procesamiento de reduccion Global de Worker: ", encargado->nodo->nodo);
		error_reduccion_global(encargado);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Respuesta estado reduccion global recibida de Worker: ",encargado->nodo->nodo);

	t_estado_master *t_estado = malloc(sizeof(t_estado_master));

	t_estado->estado = 2;
	//t_estado->bloque = //;
	t_estado->nodo = encargado->nodo->nodo;

	header_.letra = 'M';
	header_.codigo = 7;

	char *estado_a_enviar = serializar_estado_master(t_estado, &header_.sizeData);

	message *mensj_transf_est = createMessage(&header_, estado_a_enviar);

	pthread_mutex_lock(&sem_yama);
	enviar_messageIntr(config->socket_yama, mensj_transf_est, log_Mas, &controlador);
	pthread_mutex_unlock(&sem_yama);

	free(estado_a_enviar);

	if(controlador > 0)
	{
		escribir_error_log(log_Mas, "Fallo enviar mensaje de estado a YAMA");
		error_reduccion_global(encargado);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Proceso reduccion global finalizada para Worker: ",encargado->nodo->nodo);

	quitar_reduccion_global();

	void eliminar_elementos(t_redGlobal *self)
	{
		free(self->nodo->nodo);
		free(self->nodo->ip);
		free(self->nodo);
		free(self->red_global);
		free(self->temp_red_local);
		free(self);
	}

	list_destroy_and_destroy_elements(lista_global,(void *)eliminar_elementos);
	free(buffer_rta);
	free(t_estado);
	free(mensj_transf_est->buffer);
	free(mensj_transf_est);
/*
	free(encargado->nodo->ip);
	free(encargado->nodo->nodo);
	free(encargado->nodo);
	free(encargado->red_global);
	free(encargado->temp_red_local);
	free(encargado);
*/
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

	pthread_mutex_lock(&sem_yama);
	enviar_messageIntr(config->socket_yama, mensj_error, log_Mas, &controlador);
	pthread_mutex_unlock(&sem_yama);

	agregar_fallo_reducc_global();

	free(serializado);
	free(mensj_error->buffer);
	free(mensj_error);
	free(t_estado);
	free(header_d);

	free(reduccion_global->nodo->ip);
	free(reduccion_global->nodo->nodo);
	free(reduccion_global->nodo);
	free(reduccion_global->red_global);
	free(reduccion_global->temp_red_local);
	free(reduccion_global);
}

void ejecutar_almacenamiento(t_almacenado *almacenado)
{
	int controlador = 0;
	int socket_local;
	header header_;

	agregar_almacenamiento();

	//Conecto con Worker
	char *port = string_itoa(almacenado->nodo->puerto);

	//Imprimo informacion de la peticion
	printf("Ip: %s Puerto: %s\n", almacenado->nodo->ip, port);//print prueba
	printf("Archivo almacenamiento: %s\n", almacenado->red_global);

	socket_local = establecerConexion(almacenado->nodo->ip, port, log_Mas, &controlador);
	free(port);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error conectandose a Worker: ", almacenado->nodo->nodo);
		error_almacenamiento(almacenado);

		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Conectado a Worker: ", almacenado->nodo->nodo);

	//Realizacion HandShake
	header_.letra = 'M';
	header_.codigo = 0;
	header_.sizeData = 0;

	message *mensj_hand = createMessage(&header_, "");
	enviar_messageIntr(socket_local, mensj_hand, log_Mas, &controlador);

	free(mensj_hand->buffer);
	free(mensj_hand);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando HandShake Worker: ", almacenado->nodo->nodo);
		error_almacenamiento(almacenado);
		return;
	}

	char *matame = getMessageIntr(socket_local, &header_, &controlador);

	free(matame);

	if((controlador < 0)||(header_.codigo != 0))
	{
		escribir_log_error_compuesto(log_Mas, "Error recibiendo HandShake Worker: ", almacenado->nodo->nodo);
		error_almacenamiento(almacenado);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "HandShake exitoso con Worker: ",almacenado->nodo->nodo);

	//Envio de pedido de almacenamiento final
	size_t len_total;
	char *buffer_envio = serializar_FName2(almacenado->red_global, config->path_file_destino, &len_total);

	header_.letra = 'M';
	header_.codigo = 4;
	header_.sizeData = len_total;

	message *mensj_almac = createMessage(&header_, buffer_envio);
	enviar_messageIntr(socket_local, mensj_almac, log_Mas, &controlador);

	free(buffer_envio);
	free(mensj_almac->buffer);
	free(mensj_almac);

	if(controlador > 0)
	{
		escribir_log_error_compuesto(log_Mas, "Error enviando almacenamineto a Worker: ", almacenado->nodo->nodo);
		error_almacenamiento(almacenado);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Almacenamiento enviado a Worker: ",almacenado->nodo->nodo);

	//Recibo respuesta de Worker
	char *buffer_rta = getMessageIntr(socket_local, &header_, &controlador);
	if(controlador < 0)
	{
		escribir_log_error_compuesto(log_Mas, "Fallo recibir mensaje de estado Almacenado de Worker: ", almacenado->nodo->nodo);
		error_almacenamiento(almacenado);
		return;
	}
	else if(header_.codigo == 8)
	{
		escribir_log_error_compuesto(log_Mas, "Error en procesamiento de Almacenado de Worker: ", almacenado->nodo->nodo);
		error_almacenamiento(almacenado);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Respuesta estado almacenamiento recibida de Worker: ", almacenado->nodo->nodo);

	t_estado_master *t_estado = malloc(sizeof(t_estado_master));

	t_estado->estado = 2;
	//t_estado->bloque = //;
	t_estado->nodo = almacenado->nodo->nodo;

	header_.letra = 'M';
	header_.codigo = 8;

	char *estado_a_enviar = serializar_estado_master(t_estado, &header_.sizeData);

	message *mensj_transf_est = createMessage(&header_, estado_a_enviar);

	pthread_mutex_lock(&sem_yama);
	enviar_messageIntr(config->socket_yama, mensj_transf_est, log_Mas, &controlador);
	pthread_mutex_unlock(&sem_yama);

	free(estado_a_enviar);

	if(controlador > 0)
	{
		escribir_error_log(log_Mas, "Fallo enviar mensaje de estado a YAMA");
		error_almacenamiento(almacenado);
		return;
	}
	else
		escribir_log_compuesto(log_Mas, "Proceso Almacenado finalizado para Worker: ", almacenado->nodo->nodo);

	quitar_almacenamiento();

	free(buffer_rta);
	free(t_estado);
	free(mensj_transf_est->buffer);
	free(mensj_transf_est);

	free(almacenado->nodo->ip);
	free(almacenado->nodo->nodo);
	free(almacenado->nodo);
	free(almacenado->red_global);
	free(almacenado);
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

	pthread_mutex_lock(&sem_yama);
	enviar_messageIntr(config->socket_yama, mensj_error, log_Mas, &controlador);
	pthread_mutex_unlock(&sem_yama);

	agregar_fallo_almac();

	free(serializado);
	free(mensj_error->buffer);
	free(mensj_error);
	free(t_estado);
	free(header_d);

	free(almacenado->nodo->ip);
	free(almacenado->nodo->nodo);
	free(almacenado->nodo);
	free(almacenado->red_global);
	free(almacenado);
}

void matar_hilos()
{
	void _destruir_elemento(pthread_t el_hilo){
		pthread_cancel(el_hilo);
		//free(el_hilo);
	}

	list_destroy_and_destroy_elements(hilos, (void*) _destruir_elemento);
}

void calidad_transformacion(t_list *lista)
{
	int sin_bloque = 0, sin_bytes = 0, sin_temporal = 0;
	int nodo_sin_nombre = 0, nodo_sin_ip = 0, nodo_sin_puerto = 0, nodo_sin_size = 0;
	int sizes;

	numero_pedido_trans ++;
	sizes = list_size(lista);

	void _chequear(t_transformacion *pedido)
	{
		if(pedido->bloque == 0)	sin_bloque++;
		if(pedido->bytes == 0)	sin_bytes++;
		if(pedido->temporal == NULL || strlen(pedido->temporal) == 0)	sin_temporal++;
		if(pedido->nodo->ip == NULL || strlen(pedido->nodo->ip) == 0)	nodo_sin_ip++;
		if(pedido->nodo->nodo == NULL)	nodo_sin_nombre++;
		if(pedido->nodo->puerto == 0)	nodo_sin_puerto++;
		if(pedido->nodo->sizeDatabin == 0)	nodo_sin_size++;
	}
	list_iterate(lista, (void*) _chequear);

	printf("Numero de peticion: %d\n", numero_pedido_trans);
	printf("Cantidad de elementos recibidos: %d\n", sizes);
	printf("Bloque con 0: %d\n", sin_bloque);
	printf("Bytes con 0: %d\n", sin_bytes);
	printf("Temporal NULOS: %d\n", sin_temporal);
	printf("Nodo con ip NULO: %d\n", nodo_sin_ip);
	printf("Nodo con Nombre NULO: %d\n", nodo_sin_nombre);
	printf("Nodo con Puerto 0: %d\n", nodo_sin_puerto);
	printf("Nodo con size 0: %d\n", nodo_sin_size);
}
