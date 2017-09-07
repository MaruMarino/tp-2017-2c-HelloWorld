#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <funcionesNet/funcionesNet.h>
#include <funcionesNet/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include "estructuras.h"

t_configuracion *config;
t_log *log_Mas;

void inicializar_variables();
void leer_configuracion();
void conectar_yama();
void liberar_memoria();

int main(int argc, char **argv)
{
	log_Mas = crear_archivo_log("Master",true,"/home/utnso/conf/master_log");
	inicializar_variables();

	config->path_conf = strdup(argv[1]);
	config->path_trans = strdup(argv[2]);
	config->path_reduc = strdup(argv[3]);
	config->path_file_target = strdup(argv[4]);
	config->path_file_destino = strdup(argv[5]);

	leer_configuracion();
	conectar_yama();
	//escuchar_peticiones();

	liberar_memoria();
	return EXIT_SUCCESS;
}

void inicializar_variables()
{
	config = malloc(sizeof(*config));
	config->ip = strdup("");
}

void leer_configuracion()
{
	escribir_log(log_Mas, "Leyendo configuracion");
	t_config *configuracion = config_create(config->path_conf);

	string_append(&config->ip, config_get_string_value(configuracion, "YAMA_IP"));
	string_append(&config->puerto, config_get_string_value(configuracion, "YAMA_PUERTO"));

	config_destroy(configuracion);
}

void conectar_yama()
{
	int control = 0;

	config->socket_yama = establecerConexion(config->ip, config->puerto);

	if(control<0)
	{
		escribir_error_log(log_Mas, "Error conectandose a YAMA");
	}
	else
	{
		enviar(config->socket_yama, "M000000000003abc");
		char *rta = recibir(config->socket_yama);
		puts(rta);
		free(rta);
	}
}

void liberar_memoria()
{
	free(config->ip);
	free(config->path_conf);
	free(config->path_trans);
	free(config->path_reduc);
	free(config->path_file_target);
	free(config->path_file_destino);
	free(config);
	liberar_log(log_Mas);
}
