/*
 * estructuras.h
 *
 *  Created on: 8/9/2017
 *      Author: utnso
 */

#ifndef ESTRUCTURASFS_H_
#define ESTRUCTURASFS_H_

#include <commons/collections/list.h>
#include <commons/bitarray.h>

typedef enum {
	disponible,
	no_disponible,
}estado;


typedef struct {
	char *ip;
	char *dir_estructuras;
	int estado_estable; // 0- NO 1- SI
	int inicio_limpio; // 0- NO 1- SI
	int puerto;
	int serverfs;
	int espacio_total;
	int espacio_libre;
}yamafs_config;

typedef int Function ();

typedef struct {
	char *name;
	Function *func;
	char *doc;
	char *sintax;
} comando;

typedef struct{
	char *nombre;
	char *ip;
	int puerto;
	int soket;
	int espacio_total;
	int espacio_libre;
	t_bitarray *bitmapNodo;
}NODO;

typedef struct{
	int index;
	char nombre[255];
	int padre;
}t_directory;

/*typedef struct{

	char *nodo0; // nombre nodo donde esta la copia 0 de ese bloque del archivo
	int bloquenodo0; // bloque dentro del nodo donde esta la copia 0 de ese bloque del archivo
	char *nodo1; // nombre nodo donde esta la copia 1 de ese bloque del archivo
	int bloquenodo1; // nombre nodo donde esta la copia 1 de ese bloque del archivo
	int bytesEnBloque; // cantidad de bytes que conforma ese bloque ( <= 1MiB)

}bloqueArchivo;
 */
typedef struct{

	int tamanio; // tamaño total del archivo
	int index_padre; // indice del directorio padre
	estado estado; // disponible - no disponble
	char *tipo;
	int cantbloques; // Cantidad de bloques que tiene el archivo
	t_list *bloques; // lista de CANTBLOQUES elementos, de tipo bloqueArchivo

}t_archivo;



#endif /* ESTRUCTURASFS_H_ */
