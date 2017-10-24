#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/log.h>
#include <funcionesCompartidas/mensaje.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "estructuras.h"
#include "conexion_master.h"
#include "clocks.h"
#include "conexion_fs.h"

extern t_configuracion *config;
extern t_log *yama_log;

bloqueArchivo *deserializar_bloque_archivo(char *serba){

	bloqueArchivo *nuevo = malloc(sizeof(bloqueArchivo));
	size_t aux;
	size_t desplazamiento = 0;

	memcpy(&aux,serba+desplazamiento,sizeof(int));
	desplazamiento += sizeof(int);
	nuevo->nodo0 = malloc(aux+1); nuevo->nodo0[aux] = '\0';
	memcpy(nuevo->nodo0,serba+desplazamiento,aux);
	desplazamiento += aux;

	memcpy(&aux,serba+desplazamiento,sizeof(int));
	desplazamiento += sizeof(int);
	nuevo->nodo1 = malloc(aux+1); nuevo->nodo0[aux] = '\0';
	memcpy(nuevo->nodo1,serba+desplazamiento,aux);
	desplazamiento += aux;

	memcpy(&nuevo->bloquenodo0,serba+desplazamiento,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&nuevo->bloquenodo1,serba+desplazamiento,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&nuevo->bytesEnBloque,serba+desplazamiento,sizeof(int));
	desplazamiento += sizeof(int);

	return nuevo;
}
t_list *deserializar_lista_bloque_archivo(char *serializacion){

	t_list *final = list_create();
	size_t desplazamiento=0;

	int cantnodos,i;

	memcpy(&cantnodos,serializacion+desplazamiento,sizeof(int));
	desplazamiento += sizeof(int);

	size_t aux;
	for(i=0;i<cantnodos;i++){

		memcpy(&aux,serializacion+desplazamiento,sizeof(int));
		desplazamiento += sizeof(int);
		char *baux = malloc(aux);
		memcpy(baux,serializacion+desplazamiento,aux);
		desplazamiento += aux;
		bloqueArchivo *nuevito = deserializar_bloque_archivo(baux);
		list_add(final,nuevito);
		free(baux);
	}

	return final;
}

void solicitar_informacion_archivo(char *info, int _socket)
{
	size_t size_buffer = 0;
	t_list *archivo_bloques;
	int control;
	header head;

	head.codigo = 5;
	head.letra = 'Y';
	head.sizeData = size_buffer;

	message *mensaje = createMessage(&head, info);
	enviar_message(config->socket_fs, mensaje, yama_log, &control);

	header head2;
	t_list *bloques_auxiliar = list_create();
	char *mensaje2 = getMessage(config->socket_fs, &head2, &control);

	if(head2.codigo == 3)
	{
		archivo_bloques = deserializar_lista_bloque_archivo(mensaje2);
		int i = 0;

		void _convertir_bloques(bloqueArchivo *bloque)
		{
			t_bloque *bl1 = malloc(sizeof(t_bloque));
			t_bloque *bl2 = malloc(sizeof(t_bloque));

			bl1->bytes = bloque->bytesEnBloque;
			bl1->nodo = strdup(bloque->nodo0);
			bl1->n_bloque = bloque->bloquenodo0;
			bl1->n_bloque_archivo = i;

			bl2->bytes = bloque->bytesEnBloque;
			bl2->nodo = strdup(bloque->nodo1);
			bl2->n_bloque = bloque->bloquenodo1;
			bl2->n_bloque_archivo = i;

			list_add(bloques_auxiliar, bl1);
			list_add(bloques_auxiliar, bl2);

			i++;
		}

		list_iterate(archivo_bloques, _convertir_bloques);
		ejecutar_clock(bloques_auxiliar,i,_socket);
	}


	void _destruir_bloque(bloqueArchivo *self)
	{
		free(self->nodo0);
		free(self->nodo1);
		free(self);
	}

	void _destruir_bloque2(t_bloque *self)
	{
		free(self->nodo);
		free(self);
	}

	/*free(mensaje->buffer);
	free(mensaje2->buffer);
	free(mensaje);
	free(mensaje2);*/
	list_destroy_and_destroy_elements(archivo_bloques, _destruir_bloque);
	list_destroy_and_destroy_elements(archivo_bloques, _destruir_bloque2);
}
