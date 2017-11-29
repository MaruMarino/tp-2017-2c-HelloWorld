#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/serializacion.h>
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

void solicitar_informacion_archivo(char *info, int _socket)
{
	size_t size_buffer = strlen(info) + 1;
	t_list *archivo_bloques;
	int control;
	header head;

	head.codigo = 5;
	head.letra = 'Y';
	head.sizeData = size_buffer;

	message *mensaje = createMessage(&head, info);
	enviar_messageIntr(config->socket_fs, mensaje, yama_log, &control);

	escribir_log(yama_log, "Se solicitó a FS información de los bloques del archivo");

	header head2;
	t_list *bloques_auxiliar = list_create();
	char *mensaje2 = getMessageIntr(config->socket_fs, &head2, &control);

	escribir_log(yama_log, "Se recibió la información de los bloques de FS");

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

			//logear info
		/*	escribir_log_con_numero(yama_log, "Bytes bloque1: ", bl1->bytes);
			escribir_log_con_numero(yama_log, "Bytes bloque2: ", bl2->bytes);
			escribir_log_compuesto(yama_log, "Nodo bloque1: ", bl1->nodo);
			escribir_log_compuesto(yama_log, "Nodo bloque2: ", bl2->nodo);
			escribir_log_con_numero(yama_log, "NumBloque bloque1: ", bl1->n_bloque);
			escribir_log_con_numero(yama_log, "NumBloque bloque2: ", bl2->n_bloque);
			escribir_log_con_numero(yama_log, "NumBloque Archivo bloque1: ", bl1->n_bloque_archivo);
			escribir_log_con_numero(yama_log, "NumBloque Archivo bloque2: ", bl2->n_bloque_archivo);
*/
			list_add(bloques_auxiliar, bl1);
			list_add(bloques_auxiliar, bl2);

			i++;
		}

		list_iterate(archivo_bloques,(void *)_convertir_bloques);
		printf("Cantidad nodos archivo: %d \n", i);
		ejecutar_clock(bloques_auxiliar,i,_socket);
	}
	else if(head.codigo == 4)
	{
		matar_master(_socket);
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
	free(mensaje2);
	list_destroy(archivo_bloques);
	free(mensaje->buffer);
	free(mensaje);
	//list_destroy_and_destroy_elements(archivo_bloques, _destruir_bloque);
	//list_destroy_and_destroy_elements(archivo_bloques, _destruir_bloque2);
}
