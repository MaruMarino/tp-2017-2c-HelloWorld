#include <funcionesCompartidas/estructuras.h>
#include <commons/collections/list.h>
#include "showState.h"
#include "estructurasfs.h"
#include "stdio.h"

extern yamafs_config *configuracion;
//extern t_directory directorios[100];
extern t_list *nodos;
//extern t_list *archivos;
extern t_list *archivos;

int cantBlockfree(t_bitarray *listBit) {
    int i, cant = 0;
    for (i = 0; i < (int) listBit->size; ++i) {
        if (!bitarray_test_bit(listBit, i)) {
            cant++;
        }
    }
    return cant;
}

const char *getEstado(estado estado) {
    switch (estado) {
        case disponible:
            return "disponible";
        case no_disponible:
            return "no_disponible";
        default:
        	return "";
    }
}

void checkStateNodos() {
    NODO *nodo_fetch;
    puts("--------------- Estado Nodos---------------------");
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

void checkFileSystem(){
    puts("**** Estado file System ****");
    printf("%s \n",configuracion->estado_estable == 0 ? "No estable (T-T)" : " Estable ¯\\(^-^)/¯ ");
    puts("***********************");
}

void checkArchivos(){
    puts("___________ Estado file System ____________");
    int i,j;
    t_archivo * archFetch;
    bloqueArchivo *bloqueFetch;
    for (i = 0; i < archivos->elements_count ;i++) {
        archFetch = list_get(archivos, i);
        printf("Nombre --> %s\n", archFetch->nombre);
        printf("Estado --> %s\n", getEstado(archFetch->estado));
        printf("Tipo --> %s\n", archFetch->tipo);
        for(j=0;j< archFetch->bloques->elements_count;j++){
        	bloqueFetch = list_get(archFetch->bloques,j);
        	printf("----Bloque[%d] \n-----",j);
        	printf("NODO 0 --> %s\n", bloqueFetch->nodo0);
        	printf("numero bloque %d\n", bloqueFetch->bloquenodo0);
        	printf("NODO 1 --> %s\n", bloqueFetch->nodo1);
        	printf("numero bloque %d\n", bloqueFetch->bloquenodo1);
        	printf("Bytes --> %d\n", archFetch->tamanio);
        	puts("---------------");
        }
    }
    puts("____________________________________________");
}
