#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <funcionesNet/funcionesNet.h>
#include <funcionesNet/mensaje.h>
#include <funcionesNet/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include "master_manager.h"
#include "estructuras.h"

t_configuracion *config;
t_list *transformar;
t_list *reducir;
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

	liberar_memoria();
	return EXIT_SUCCESS;
}

void inicializar_variables()
{
	config = malloc(sizeof(*config));
	config->ip = strdup("");
	config->puerto = strdup("");
	transformar = list_create();
	reducir = list_create();
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
	config->socket_yama = establecerConexion(config->ip, config->puerto);

	if(config->socket_yama<=0)
	{
		escribir_error_log(log_Mas, "Error conectandose a YAMA");
	}
	else
	{
		char *handshake = armar_mensaje("M00",config->path_file_target);
		enviar(config->socket_yama, handshake);
		free(handshake);
		escuchar_peticiones();
	}
}

void liberar_memoria()
{
	free(config->ip);
	free(config->puerto);
	free(config->path_conf);
	free(config->path_trans);
	free(config->path_reduc);
	free(config->path_file_target);
	free(config->path_file_destino);
	free(config);
	liberar_log(log_Mas);
}
