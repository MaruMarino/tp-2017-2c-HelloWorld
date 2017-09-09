/*
 ============================================================================
 Name        : FileSystem.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <commons/config.h>
#include <commons/log.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "estructuras.h"
#include "FS_conexiones.h"
#include "FS_consola.h"


yamafs_config *configuracion;
t_log *logi;

void leer_configuracion(char *);
void liberar_memoria();

int main(int argc, char **argv) {


	logi = log_create("/home/utnso/yamafslog","YamaFS",false,LOG_LEVEL_INFO);
	leer_configuracion(argv[1]);
	pthread_t hiloConexiones;
	pthread_t hiloConsola;
	//pthread_t hiloPantalla; ->podria no ser un hilo?
	pthread_create(&hiloConexiones,NULL, (void*)manejo_conexiones, NULL);
	pthread_create(&hiloConsola,NULL,(void *)iniciar_consola_FS,NULL);
	//if argv[2] --clean ->ignorar estado anterior
	//else restaurar yamaFS desde un estado anterior
	pthread_join(hiloConexiones,NULL);
	pthread_join(hiloConsola,NULL);
	//k onda wey

	liberar_memoria();
	return EXIT_SUCCESS;
}

void leer_configuracion(char *path){

	configuracion = malloc(sizeof (yamafs_config));
	t_config *aux_config = config_create(path);

	configuracion->dir_estructuras= strdup(config_get_string_value(aux_config,"RUTA_ESTRUCTURAS"));
	configuracion->ip = strdup(config_get_string_value(aux_config,"IP"));
	configuracion->puerto = config_get_int_value(aux_config,"PUERTO");

	printf("Configuracion del Yama FS \n");
	printf("IP: %s \n",configuracion->ip);
	printf("PUERTO: %d \n",configuracion->puerto);
	printf("Ruta donde se encuentran las estructuras administrativas: %s \n",configuracion->dir_estructuras);

	log_info(logi,"Configuracion del Yama FS \n");
	log_info(logi,"IP: %s \n",configuracion->ip);
	log_info(logi,"PUERTO: %d \n",configuracion->puerto);
	log_info(logi,"Ruta donde se encuentran las estructuras administrativas: %s \n",configuracion->dir_estructuras);

	config_destroy(aux_config);
}
void liberar_memoria(){

	free(configuracion->dir_estructuras);
	free(configuracion->ip);
	free(configuracion);

	log_destroy(logi);

}
