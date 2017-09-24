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
#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "estructurasfs.h"
#include "FS_conexiones.h"
#include "FS_consola.h"
#include "FS_administracion.h"


yamafs_config *configuracion;
t_log *logi;
t_list *nodos;
t_directory directorios[100];
int const maxline = 0x10000;

void leer_configuracion(char *);
void liberar_memoria(void);
void inicializaciones(void);


int main(int argc, char **argv) {

	logi = log_create("/home/utnso/yamafslog","YamaFS",true,LOG_LEVEL_INFO);
	inicializaciones();
	leer_configuracion(argv[1]);
	if(argc == 3 && (!strncmp(argv[2],"--clean",7))){

		log_info(logi, "Iniciando YAMA-FS de cero");
		configuracion->inicio_limpio = 1;

	}else{

		log_info(logi, "Restaurando estructuras de un estado anterior, si existe");
		configuracion->inicio_limpio = 0;
		int res = recuperar_estructuras_administrativas();
		if(res != 1){
			liberar_memoria();
			return EXIT_FAILURE;
		}
		log_info(logi, "Listo");
	}

	pthread_t hiloConexiones;
	//pthread_t hiloConsola;

	pthread_create(&hiloConexiones,NULL, (void*)manejo_conexiones, NULL);
	//pthread_create(&hiloConsola,NULL,(void *)iniciar_consola_FS,NULL);

	//if argv[2] --clean ->ignorar estado anterior
	//else restaurar yamaFS desde un estado anterior

	//pthread_join(hiloConsola,NULL);
	pthread_join(hiloConexiones,NULL);


	//k onda wey

	liberar_memoria();
	return EXIT_SUCCESS;
}

void leer_configuracion(char *path){

	t_config *aux_config = config_create(path);

	configuracion->dir_estructuras= strdup(config_get_string_value(aux_config,"RUTA_ESTRUCTURAS"));
	configuracion->ip = strdup(config_get_string_value(aux_config,"IP"));
	configuracion->puerto = config_get_int_value(aux_config,"PUERTO");


	log_info(logi,"Configuracion del Yama FS \n");
	log_info(logi,"IP: %s \n",configuracion->ip);
	log_info(logi,"PUERTO: %d \n",configuracion->puerto);
	log_info(logi,"Ruta donde se encuentran las estructuras administrativas: %s \n",configuracion->dir_estructuras);

	config_destroy(aux_config);
}
void inicializaciones(void){

	configuracion = malloc(sizeof (yamafs_config));
	configuracion->estado_estable = 0;
	nodos = list_create();
}

void liberar_memoria(void ){

	free(configuracion->dir_estructuras);
	free(configuracion->ip);
	free(configuracion);

	log_destroy(logi);

	void _nodo_destroyer(NODO *self){
		free(self->ip);
		free(self->nombre);
		msync(self->bitmapNodo->bitarray,self->bitmapNodo->size,MS_SYNC);
		munmap(self->bitmapNodo->bitarray,self->bitmapNodo->size);
		free(self->bitmapNodo->bitarray);
		bitarray_destroy(self->bitmapNodo);
		free(self);
	}
	list_destroy_and_destroy_elements(nodos,(void *)_nodo_destroyer);



}
