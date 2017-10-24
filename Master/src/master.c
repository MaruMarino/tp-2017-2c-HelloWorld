#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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

pthread_mutex_t mutex_estadistica;
t_list *hilos;
t_configuracion *config;
t_estadistica *estadistica;
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
	estadistica = malloc(sizeof(t_estadistica));
	estadistica->inicio_trans = malloc(sizeof(time_t));
	estadistica->fin_trans = malloc(sizeof(time_t));
	estadistica->inicio_reduc_local = malloc(sizeof(time_t));
	estadistica->fin_reduc_local = malloc(sizeof(time_t));
	estadistica->inicio_reduc_global = malloc(sizeof(time_t));
	estadistica->fin_reduc_global = malloc(sizeof(time_t));
	estadistica->inicio_alm = malloc(sizeof(time_t));
	estadistica->fin_alm = malloc(sizeof(time_t));
	estadistica->fallo_almacenamiento = 0;
	estadistica->fallo_reduc_global = 0;
	estadistica->fallo_reduc_local = 0;
	estadistica->fallo_transf = 0;
	estadistica->reduc_ejecutando = 0;
	estadistica->reduc_paralelo = 0;
	estadistica->reduc_total = 0;
	estadistica->reduc_glo_total = 0;
	estadistica->alm_total = 0;
	estadistica->transf_ejecutando = 0;
	estadistica->transf_paralelo = 0;
	estadistica->transf_total = 0;

	hilos = list_create();
	config = malloc(sizeof(t_configuracion));
	config->ip = strdup("");
	config->puerto = strdup("");

	pthread_mutex_init(&mutex_estadistica,NULL);
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

		fclose(archivo);
	}
	else printf("El archivo ingresado no existe o no se puede abrir\n");

	return destino;
}

void conectar_yama()
{
	int controlador = 0;

	config->socket_yama = establecerConexion(config->ip, config->puerto, log_Mas, &controlador);

	if(controlador !=0 )
		escribir_error_log(log_Mas, "Error conectandose a YAMA");
	else
	{
		header *handshake = malloc(sizeof(header));
		handshake->codigo = 0;
		handshake->letra = 'M';
		handshake->sizeData = (size_t) string_length(config->path_file_target) + 1;

		message *mensaje = createMessage(handshake, config->path_file_target);

		enviar_message(config->socket_yama, mensaje, log_Mas, &controlador);
		/*
		getMessage(config->socket_yama, handshake, &controlador);
		*/
		free(handshake);
		free(mensaje->buffer);
		free(mensaje);

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

	free(estadistica->inicio_trans);
	free(estadistica->fin_trans);
	free(estadistica->inicio_reduc_local);
	free(estadistica->fin_reduc_local);
	free(estadistica->inicio_reduc_global);
	free(estadistica->fin_reduc_global);
	free(estadistica->inicio_alm);
	free(estadistica->fin_alm);
	free(estadistica);
}
