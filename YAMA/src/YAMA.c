#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "estructuras.h"

t_log *yama_log;
t_configuracion *config;
t_list *tabla_estado;

void leer_configuracion();
void liberar_memoria();
void inicializar_variables();
void conectar_fs();
void crear_socket_servidor();

int main(int argc, char **argv)
{
	yama_log = crear_archivo_log("YAMA",true,"/home/utnso/conf/master_log");

	char *path = strdup(argv[1]);
	inicializar_variables();
	leer_configuracion(path);
	free(path);

	conectar_fs();
	crear_socket_servidor();


	return EXIT_SUCCESS;
}

void inicializar_variables()
{
	config = malloc (sizeof (t_configuracion*));
	config->algortimo_bal = strdup("");
	config->fs_ip = strdup("");
	config->fs_puerto = strdup("");
	config->yama_ip = strdup("");
	config->yama_puerto = strdup("");

	tabla_estado = list_create();
}

void liberar_memoria()
{
	free(config->algortimo_bal);
	free(config->yama_puerto);
	free(config->yama_ip);
	free(config->fs_ip);
	free(config->fs_puerto);
	free(config);
	list_destroy(tabla_estado);
}

void leer_configuracion(char *path)
{
	escribir_log(yama_log, "Leyendo configuracion");

	t_config *configuracion = config_create(path);

	string_append(&config->yama_ip, config_get_string_value(configuracion, "YAMA_IP"));
	string_append(&config->yama_puerto, config_get_string_value(configuracion, "YAMA_PUERTO"));
	string_append(&config->algortimo_bal, config_get_string_value(configuracion, "ALGORITMO_BALANCEO"));
	config->retardo_plan = config_get_int_value(configuracion, "RETARDO_PLANIFICACION");
	string_append(&config->fs_ip, config_get_string_value(configuracion, "FS_IP"));
	string_append(&config->fs_puerto, config_get_string_value(configuracion, "FS_PUERTO"));

	config_destroy(configuracion);
}

void conectar_fs()
{
	int control = 0;
	config->socket_fs = establecerConexion(config->fs_ip, config->fs_puerto);
	if(control<0)
	{
		escribir_error_log(yama_log, "Error conectandose a FS");
	}
	else
	{
		escribir_log(yama_log, "Conectado a FS");
		enviar(config->socket_fs, "Y000000000000");
		char *rta = recibir(config->socket_fs);
		puts(rta);
		free(rta);
	}
}

void crear_socket_servidor()
{

}
