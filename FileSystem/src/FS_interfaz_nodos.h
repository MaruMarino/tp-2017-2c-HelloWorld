/*
 * FS_interfaz_nodos.h
 *
 *  Created on: 28/10/2017
 *      Author: utnso
 */

#ifndef FS_INTERFAZ_NODOS_H_
#define FS_INTERFAZ_NODOS_H_

#include <stdio.h>
#include "estructurasfs.h"
#include <funcionesCompartidas/estructuras.h>

/* Funciones para realizar Almacenar Archivo*/

int setBlock(void *, size_t size_buffer, t_list *ba);

int searchNodoInList(NODO *nameNodo);

estado checkStateFileSystem();

t_list *escribir_desde_archivo(char *local_path, char file_type, int filesize); // tipo B/b-binario T/t-texto

int get_file_size(char *);

bool hay_lugar_para_archivo(int size);

t_list *get_copia_nodos_activos();

void disconnectedNodo(int socket);

/*todo Funciones para realizar Leer Archivo*/

void *
leer_bloque(bloqueArchivo *bq, int copia); //si copia == 1 me fijo primero en la copia, else primero en el original

int crear_archivo_temporal(t_archivo *archivo, char *nomre_temporal);

/* Dado una lista de bloques que conformar un archivo, pide en simultaneo las distintas
 * partes a cada Nodo de interes y agrupa el resultado final en un char*
 * En caso exitoso retorna el char* con la info binaria del File
 * En caso de error retorna -1
 */
char *pedirFile(t_list *bloques, size_t size_archive);

/* Estructuras auxiliares para administrar el pedirFile() */
typedef struct {
    bloqueArchivo *bq;
    int ord; // orden del bloque en su Archivo
} _bloq;

typedef struct {
    int numberBlock;
    int orden;
    size_t sizeBuffer;
} bloqPedido;

struct _nodoCola {
    bool hay_pedidos;
    int fd;
    int node; // 0 || 1 -> representa en que nodo hay que pedir el bloque
    t_list *colaPedidos; // mallocar el maximo posible por NODO +1 (NULL)
};

/* Funciones auxiliares para administrar el pedirFile() */
int inicializarNodoCola(int lengthNodo, struct _nodoCola (*nodC)[lengthNodo], t_list *bloques);

void liberarNodoCola(int nq, struct _nodoCola (*nodC)[nq]);

int encolarSobreNodos(int lengthNodo, struct _nodoCola (*nodC)[lengthNodo], bloqueArchivo *bloque, int pos);

int delegarPedidos(int nq, struct _nodoCola (*nodC)[nq], int node);

void enviarPeticion(int socket, int bloque);

bool restanPedidos(int nq, struct _nodoCola (*nodC)[nq]);

#endif /* FS_INTERFAZ_NODOS_H_ */
