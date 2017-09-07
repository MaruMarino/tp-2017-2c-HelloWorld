#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <funcionesNet/funcionesNet.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include "estructuras.h"

extern t_configuracion *config;

void escuchar_peticiones()
{
	char *peticion;

	while(1)
	{
		peticion = recibir(config->socket_yama);
		puts(peticion);
		//peticion = recibir(config->socket_yama, controlador);
	}
}
