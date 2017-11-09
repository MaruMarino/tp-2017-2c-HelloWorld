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

int setBlock(void *buffer, size_t size_buffer) {
    checkStateNodos();
    int cantCopy = 0;
    int control;
    size_t bufferWithBlockSize = (size_buffer + sizeof(int));
    void *bufferWithBlock = malloc(bufferWithBlockSize);
    NODO *nodoSend = NULL;
    int nodoSendBlock;
    header reqRes;
    //int insertOk;
    //void *bufferResponse;
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
        /*
        bufferResponse = getMessage(nodoSend->soket,&reqRes,&control);
        if(bufferResponse == NULL){
            puts("error al recibir la respuesta");
            return -1;
        }

        if(reqRes.codigo != 6){
            puts("recivio otro mjs");
            return -1;
        }

        memcpy(&insertOk, bufferResponse,reqRes.sizeData);
        if(insertOk == -1){
            puts("no se inserto el bloque");
            return -1;
        }*/
        bitarray_set_bit(nodoSend->bitmapNodo, nodoSendBlock);
        nodoSend->espacio_libre -= Mib;
        cantCopy++;
    }
    checkStateNodos();
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

void *contenido_archivo(char *pathlocal, int *filesize) {

    FILE *archi = fopen(pathlocal, "r");

    struct stat info;

    stat(pathlocal, &info);
    *filesize = info.st_size;

    void *buff = malloc(*filesize);

    fread(buff, info.st_size, 1, archi);

    return buff;
}

int dividir_enviar_archivo(char *contenido, int fsize, char *tipo) {


    if (!strcmp(tipo, "B") || !strcmp(tipo, "b")) {
        // dividir normal en
    }
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

/* todo: FUNCIONES LEER ARCHIVO */

