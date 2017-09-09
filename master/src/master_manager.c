#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <funcionesNet/funcionesNet.h>
#include <funcionesNet/mensaje.h>
#include <funcionesNet/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include "estructuras.h"

extern t_configuracion *config;
extern t_log *log_Mas;

void ejecutar_transformador(char *nodos);

void escuchar_peticiones()
{
	char *peticion, *codigo;

	while(1)
	{
		peticion = recibir(config->socket_yama);
		codigo = get_codigo(peticion);

		switch(atoi(codigo)) {
			case 1: ;
				escribir_log(log_Mas, "Se recibio una peticion de transformacion");
				char *mensaje_t = get_mensaje(peticion);
				ejecutar_transformador(mensaje_t);
				free(mensaje_t);
				break;
			case 2: ;
				escribir_log(log_Mas, "Se recibio una peticion de reduccion local");
				char *mensaje_r = get_mensaje(peticion);
				//llamar transformador
				free(mensaje_r);
				break;
			default:
				puts("default");
				break;
		}
		free(peticion);
		free(codigo);
	}
}

void ejecutar_transformador(char *nodos)
{
	//deberia procesar los nodos recibidos

}
