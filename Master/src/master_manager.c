#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <funcionesCompartidas/serializacion_yama_master.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/mensaje.h>
#include <funcionesCompartidas/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include "estructuras.h"

extern t_configuracion *config;
extern t_log *log_Mas;

void ejecutar_transformador(void *nodo);

void escuchar_peticiones()
{
	header *head = malloc(sizeof(head));

	while(1)
	{
		void *buffer = getMessage(config->socket_yama, head);

		switch(head->codigo)
		{
			case 1: ;
				escribir_log(log_Mas, "Se recibio una peticion de transformacion");
				ejecutar_transformador(buffer);
				break;
			case 2: ;
				escribir_log(log_Mas, "Se recibio una peticion de reduccion local");
				//llamar transformador
				break;
			default:
				puts("default");
				break;
		}
		free(buffer);
	}
}

void ejecutar_transformador(void *nodo)
{

}
