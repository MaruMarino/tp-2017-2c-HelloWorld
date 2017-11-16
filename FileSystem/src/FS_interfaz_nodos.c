/*
 * FS_interfz_nodos.c
 *
 *  Created on: 28/10/2017
 *      Author: utnso
 */

#include "FS_interfaz_nodos.h"
#include "estructurasfs.h"


#include <funcionesCompartidas/funcionesNet.h>
#include <commons/bitarray.h>
#include <commons/string.h>
#include <commons/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <funcionesCompartidas/estructuras.h>
#include <commons/collections/list.h>


#define Mib 1048576

//extern yamafs_config *configuracion;
extern t_log *logi;
//extern t_directory directorios[100];
extern t_list *nodos;
//extern t_list *archivos;
extern t_list *archivos;


/* FUNCIONES ALMACENAR ARCHIVO */

const char *getEstado(estado estado) {
	switch (estado) {
	case disponible:
		return "disponible";
	case no_disponible:
		return "no_disponible";
	}
}

int getBlockFree(t_bitarray *listBit) {
	int i;
	for (i = 0; i < listBit->size; ++i) {
		if (!bitarray_test_bit(listBit, i)) {
			return i;
		}
	}
	return -1;
}

NODO *getNodoMinusLoader(NODO *NodoExcluir, int *numBlock) {
	int maxLibreEspacio = 0;
	NODO *nodoMax = NULL;
	NODO *nodoFetch;
	int i;
	for (i = 0; i < nodos->elements_count; i++) {
		nodoFetch = list_get(nodos, i);
		if (nodoFetch->espacio_libre > maxLibreEspacio) {
			*numBlock = getBlockFree(nodoFetch->bitmapNodo);
			if (!(NodoExcluir != NULL && NodoExcluir->soket == nodoFetch->soket) && *numBlock != -1) {
				maxLibreEspacio = nodoFetch->espacio_libre;
				nodoMax = nodoFetch;
			}
		}
	}
	return nodoMax;
}

int cantBlockfree(t_bitarray *listBit) {
	int i, cant = 0;
	for (i = 0; i < listBit->size; ++i) {
		if (!bitarray_test_bit(listBit, i)) {
			cant++;
		}
	}
	return cant;
}

void checkStateNodos() {
	NODO *nodo_fetch;
	puts("------------------------------------");
	int i;
	for (i = 0; i < nodos->elements_count; ++i) {
		nodo_fetch = list_get(nodos, i);
		printf("name --> [%s]\n", nodo_fetch->nombre);
		printf("stado --> %s\n", getEstado(nodo_fetch->estado));
		printf("socket --> [%d]\n", nodo_fetch->soket);
		printf("bloque free %d\n", cantBlockfree(nodo_fetch->bitmapNodo));
		printf("Espacio Total %d\n", nodo_fetch->espacio_total);
		printf("Espacio Libre %d\n", nodo_fetch->espacio_libre);
	}
	puts("------------------------------------");
}

int setBlock(void *buffer, size_t size_buffer,t_list *bloquesArch) {
	checkStateNodos();
	int cantCopy = 0;
	int control;
	size_t bufferWithBlockSize = (size_buffer + sizeof(int));
	void *bufferWithBlock = malloc(bufferWithBlockSize);
	NODO *nodoSend = NULL;
	int nodoSendBlock;
	header reqRes;
	bloqueArchivo *nuevo = malloc(sizeof(bloqueArchivo));

	while (cantCopy < 2) {
		nodoSend = getNodoMinusLoader(nodoSend, &nodoSendBlock);
		if (nodoSend == NULL) return -1;
		reqRes.codigo = 2;
		reqRes.letra = 'F';
		reqRes.sizeData = bufferWithBlockSize;
		memcpy(bufferWithBlock, &nodoSendBlock, sizeof(int));
		memcpy((bufferWithBlock + sizeof(int)), buffer, size_buffer);
		message *request = createMessage(&reqRes, bufferWithBlock);

		if (enviar_messageIntr(nodoSend->soket, request, logi, &control) < 0) {
			puts("error al enviar primer bloque");
			return -1;
		}
		if(cantCopy == 0){
			nuevo->nodo0 = strdup(nodoSend->nombre);
			nuevo->bloquenodo0 = nodoSendBlock;
		}else{
			nuevo->nodo1 = strdup(nodoSend->nombre);
			nuevo->bloquenodo1 = nodoSendBlock;
		}

		bitarray_set_bit(nodoSend->bitmapNodo, nodoSendBlock);
		nodoSend->espacio_libre -= Mib;
		cantCopy++;

		free(request->buffer);
		free(request);
	}
	checkStateNodos();
	nuevo->bytesEnBloque= size_buffer;
	list_add(bloquesArch,nuevo);

	return 0;
}


int exitProcess(NODO *TestNodo) {
	int i;
	NODO *nodoFetch;
	for (i = 0; i < nodos->elements_count; ++i) {
		nodoFetch = list_get(nodos, i);
		if (strcmp(nodoFetch->nombre, TestNodo->nombre) == 0) {
			nodoFetch->soket = TestNodo->soket;
			nodoFetch->puerto = TestNodo->puerto;
			nodoFetch->ip = TestNodo->ip;
			nodoFetch->estado = disponible;
			return 1;
		}
	}
	return 0;
}

estado checkStateNodo(char *nameNodo) {
	int i;
	NODO *nodoFetch;
	for (i = 0; i < nodos->elements_count; ++i) {
		nodoFetch = list_get(nodos, i);
		if (strcmp(nodoFetch->nombre, nameNodo) == 0) {
			return nodoFetch->estado;
		}
	}
	return no_disponible;
}

estado checkStateArchive(t_archivo *archivo) {
	int i;
	bloqueArchivo *fetchBloque;
	for (i = 0; i < archivo->bloques->elements_count; ++i) {
		fetchBloque = list_get(archivo->bloques, i);
		if (!(checkStateNodo(fetchBloque->nodo0) == disponible || checkStateNodo(fetchBloque->nodo1) == disponible)) {
			return no_disponible;
		}
	}
	return disponible;
}

estado checkStateFileSystem() {
	int i;
	t_archivo *fetchArchivo;
	estado fileStable = no_disponible;
	for (i = 0; i < archivos->elements_count; ++i) {
		fetchArchivo = list_get(archivos, i);
		fetchArchivo->estado = (fileStable = checkStateArchive(fetchArchivo));
	}
	for (i = 0; i < archivos->elements_count; ++i) {
		printf("------------------Archivo [%d] ----------------------\n", i);
		fetchArchivo = list_get(archivos, i);
		printf("estado %s\n", getEstado(fetchArchivo->estado));
	}
	return fileStable;
}

void disconnectedNodo(int socket) {
	int i;
	NODO *nodoFetch;
	for (i = 0; i < nodos->elements_count; ++i) {
		nodoFetch = list_get(nodos, i);
		if (nodoFetch->soket == socket && nodoFetch->estado == disponible) {
			nodoFetch->estado = no_disponible;
		}
	}
	checkStateFileSystem();
}

t_list *escribir_desde_archivo(char *local_path,char file_type,int filesize){

	FILE *archi = fopen(local_path,"r");
	if(archi == NULL){
		log_info(logi, "Error abriendo archivo");
		return NULL;
	}

	int leido = 0;
	char *linea=NULL;
	char *buffer;
	ssize_t getln;
	size_t len_line,len;
	t_list *ba = list_create();

	if(file_type == 'B' || file_type == 'b'){

		int blocks,block_count,ultimoBloque;
		buffer = malloc(Mib);
		blocks = (filesize % Mib != 0) ? (filesize/Mib +1) : (filesize/Mib);
		ultimoBloque = (filesize % Mib != 0 || filesize < Mib) ? (filesize - (filesize/Mib)*Mib) : Mib;


		for(block_count=0;block_count < blocks-1; block_count ++){

			fread(buffer,Mib,1,archi);
			setBlock(buffer,Mib,ba);


		}
		fread(buffer,ultimoBloque,1,archi);
		setBlock(buffer,ultimoBloque,ba);
		free(buffer);


	}else {

		buffer = strdup("");
		getln = getline(&linea,&len,archi);
		while(getln != -1){

			len_line = strlen(linea);
			if(leido+len_line > Mib){
				// guardo bloque igual o menor a Mib
				setBlock(buffer,leido,ba);

				free(buffer);
				buffer =strdup("");
				leido = 0;
			}
			string_append(&buffer,linea);
			leido += len_line;
			len = 0;
			free(linea);
			linea = NULL;
			getln = getline(&linea,&len,archi);
		}
		//guardo bloque final/único si filsize < Mib
		setBlock(buffer,leido,ba);
		free(buffer);
	}

	fclose(archi);
	return ba;
}



bool hay_lugar_para_archivo(int filesize){

	int blocks = (filesize % Mib != 0) ? (filesize/Mib +1) : (filesize/Mib);
	int cantCopy = 0;
	NODO *nodoSend = NULL;
	int nodoSendBlock;
	int b;
	for(b=0;b<blocks;b++){
	while (cantCopy < 2) {
		nodoSend = getNodoMinusLoader(nodoSend, &nodoSendBlock);
		if (nodoSend == NULL) return false;
		cantCopy ++;
	}
	}

	return true;
}

int get_file_size(char *path){

	struct stat info;
	int ret = stat(path, &info);
	if(ret == -1){
		return ret;
	}
	return info.st_size;
}


/* todo: FUNCIONES LEER ARCHIVO */

