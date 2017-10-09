#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/mensaje.h>
#include <funcionesCompartidas/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include "master_manager.h"
#include "estructuras.h"

t_configuracion *config;
t_estadistica *estadistica;
t_list *transformar;
t_list *reducir;
t_log *log_Mas;

void inicializar_variables();
void leer_configuracion();
void conectar_yama();
void liberar_memoria();
char *leer_archivo(char *ruta_arch);

int main(int argc, char **argv)
{
	log_Mas = crear_archivo_log("Master",true,"/home/utnso/conf/master_log");
	inicializar_variables();

	config->path_conf = strdup(argv[1]);
	config->script_trans = leer_archivo(argv[2]);
	config->script_reduc = leer_archivo(argv[3]);
	config->path_file_target = strdup(argv[4]);
	config->path_file_destino = strdup(argv[5]);

	leer_configuracion();
	conectar_yama();

	liberar_memoria();
	return EXIT_SUCCESS;
}

void inicializar_variables()
{
	config = malloc(sizeof(t_configuracion));
	estadistica = malloc(sizeof(t_estadistica));
	estadistica->inicio_trans = malloc(sizeof(time_t));
	estadistica->fin_trans = malloc(sizeof(time_t));
	estadistica->inicio_reduc_local = malloc(sizeof(time_t));
	estadistica->fin_reduc_local = malloc(sizeof(time_t));
	estadistica->inicio_reduc_global = malloc(sizeof(time_t));
	estadistica->fin_reduc_global = malloc(sizeof(time_t));
	estadistica->inicio_alm = malloc(sizeof(time_t));
	estadistica->fin_alm = malloc(sizeof(time_t));
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

char *leer_archivo(char *ruta_arch)
{
	FILE *archivo = fopen(ruta_arch,"r");
	char *destino;

	if(archivo)
	{
		fseek(archivo,0L,SEEK_END);
		long int fsize = ftell(archivo);
		fseek(archivo,0,0);

		destino = malloc(fsize + 1);
		fread(destino,fsize,1,archivo);
		//fread(mensaje2,sizeof(char),final,archivo);

		fclose(archivo);
	}
	else printf("El archivo ingresado no existe o no se puede abrir\n");

	return destino;
}

void conectar_yama()
{
	int controlador = 0;

	config->socket_yama = establecerConexion(config->ip, config->puerto, log_Mas, &controlador);

	if(controlador!=0)
		escribir_error_log(log_Mas, "Error conectandose a YAMA");
	else
	{
		header *handshake = malloc(sizeof(header));
		handshake->codigo = 0;
		handshake->letra = 'M';
		handshake->sizeData = (size_t) string_length(config->path_file_target) + 1;

		message *mensaje = createMessage(handshake, config->path_file_target);
		enviar_message(config->socket_yama, mensaje, log_Mas, &controlador);

		free(handshake);
		escuchar_peticiones();
	}
}

void liberar_memoria()
{
	free(config->ip);
	free(config->puerto);
	free(config->path_conf);
	free(config->script_trans);
	free(config->script_reduc);
	free(config->path_file_target);
	free(config->path_file_destino);
	free(config);
	liberar_log(log_Mas);
}
