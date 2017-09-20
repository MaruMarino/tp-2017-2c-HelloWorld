#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/mensaje.h>
#include <funcionesCompartidas/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include "estructuras.h"

extern t_configuracion *config;
extern t_log *log_Mas;

void ejecutar_transformador(char *nodos);

void escuchar_peticiones()
{
	char *peticion;
	int controlador, codigo;

	while(1)
	{
		peticion = recibir(config->socket_yama, log_Mas, &controlador);
		codigo = get_codigo(peticion);

		switch(codigo)
		{
			case 1: ;
				escribir_log(log_Mas, "Se recibio una peticion de transformacion");
				ejecutar_transformador(peticion);
				break;
			case 2: ;
				escribir_log(log_Mas, "Se recibio una peticion de reduccion local");
				//llamar transformador
				break;
			default:
				puts("default");
				break;
		}
		free(peticion);
	}
}

void ejecutar_transformador(char *nodos)
{
	//deberia procesar los nodos recibidos

}
