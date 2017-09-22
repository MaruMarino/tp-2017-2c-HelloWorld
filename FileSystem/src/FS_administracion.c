/*
 * FS_administracion.c
 *
 *  Created on: 19/9/2017
 *      Author: utnso
 */

#include "FS_administracion.h"

#include <sys/mman.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "estructurasfs.h"


extern yamafs_config *configuracion;
extern t_log *logi;
extern t_directory directorios[100];
extern t_list *nodos;



int recuperar_estructuras_administrativas(void){

	int status;
	// recuperar arbol de directorios
	status = recuperar_arbol_directorios();
	if(status != 1){
		log_error(logi,"Error en la recuperacion del arbol de directorios, bai");
		return status;
	}
	// recuperar NODOS
	status =recuperar_nodos();
	if(status != 1){
		log_error(logi,"Error en la recuperacion de nodos, bai");
		return status;
	}
	// Recuperar bitmaps nodos
	int cantidad_nodos=list_size(nodos);
	int i=0;
	while(i< cantidad_nodos){

		NODO *unodo= list_get(nodos,i);
		status = recuperar_bitmap_nodo(unodo);
		if(status != 1){
			log_error(logi,"Error en la recuperacion de bitmaps, bai");
			return status;
		}
		i++;
	}

	// TODO: RECUPERAR METADA ARCHIVOS

	return 1;
}
char *completar_path_metadata(char *archivo){

	char *pmetadata = strdup(configuracion->dir_estructuras);
	char *pmdirectorios= strdup("");

	string_append(&pmdirectorios,pmetadata);
	if(string_ends_with(pmdirectorios,"/")){
		string_append(&pmdirectorios,archivo);
	}else{
		string_append(&pmdirectorios,"/");
		string_append(&pmdirectorios,archivo);
	}

	free(pmetadata);
	return pmdirectorios;
}

int recuperar_arbol_directorios(void){

	char *path_armado;
	// recuperar arbol de directorios
	path_armado= completar_path_metadata("directorios.dat");
	FILE *filedir = fopen(path_armado,"r");

	if(filedir == NULL){
		log_error(logi, "Error abriendo archivo 'directorios.dat'");
		free(path_armado);
		return -1;
	}else{
		fread(directorios,sizeof(t_directory),100,filedir);
		fclose(filedir);
	}

	free(path_armado);
	return 1;
}

int recuperar_nodos(void){

	char *path_armado;
	path_armado = completar_path_metadata("nodos.bin");

	struct stat data;
	int res = stat(path_armado,&data);
	if(res != 0){
		log_error(logi,"Error abriendo Nodos.bin");
		free(path_armado);
		return -1;
	}

	t_config *config_nodos = config_create(path_armado);

	configuracion->tamanio_total = config_get_int_value(config_nodos,"TAMANIO");
	configuracion->espacio_libre = config_get_int_value(config_nodos,"LIBRE");
	char **nombres_nodos = config_get_array_value(config_nodos,"NODOS");

	int i = 0;
	char *key_aux;

	while(nombres_nodos[i] != NULL){

		NODO *nodo_recuperado = malloc(sizeof(NODO));

		nodo_recuperado->nombre = malloc(strlen(nombres_nodos[i]));
		strcpy(nodo_recuperado->nombre,nombres_nodos[i]);

		key_aux= string_from_format("%sTotal",nombres_nodos[i]);
		nodo_recuperado->espacio_total = config_get_int_value(config_nodos,key_aux);
		free(key_aux);

		key_aux = string_from_format("%sLibre",nombres_nodos[i]);
		nodo_recuperado->espacio_libre = config_get_int_value(config_nodos,key_aux);
		free(key_aux);

		list_add(nodos,nodo_recuperado);
		i++;
	}

	free(path_armado);
	config_destroy(config_nodos);
	i=0;
	while(nombres_nodos[i]!= NULL){
		free(nombres_nodos[i]);
		i++;
	}
	free(nombres_nodos);

	return 1;
}

int recuperar_bitmap_nodo(NODO *unodo){

	struct stat mystat;

	char *aux_nombre_archivo = string_from_format("bitmaps/%s.dat",unodo->nombre);
	char *path_armado = completar_path_metadata(aux_nombre_archivo);

	int fdbitmap = open(path_armado,O_RDWR);
	if(fdbitmap <= 0){
		log_error(logi,"Error abriendo %s \n",aux_nombre_archivo);
		free(path_armado);
		return -1;
	}
	fstat(fdbitmap,&mystat);
	char *bitarray = mmap(0,mystat.st_size,PROT_READ|PROT_WRITE,MAP_SHARED,fdbitmap,0);
	if(bitarray == MAP_FAILED){
		log_error(logi,"Error en mmap :%s\n", strerror(errno));
		close(fdbitmap);
		free(path_armado);
		return -1;

	}
	unodo->bitmapNodo = bitarray_create_with_mode(bitarray,mystat.st_size,LSB_FIRST);
	free(aux_nombre_archivo);
	free(path_armado);
	close(fdbitmap);
	return 1;
}

