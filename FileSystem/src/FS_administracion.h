/*
 * FS_administracion.h
 *
 *  Created on: 19/9/2017
 *      Author: utnso
 */

#ifndef FS_ADMINISTRACION_H_
#define FS_ADMINISTRACION_H_

#include "estructurasfs.h"


// Funciones de recuperacion de estructuras administrativas de un estado anterior

int recuperar_estructuras_administrativas(void);

int recuperar_arbol_directorios(void);

int recuperar_nodos(void);

int recuperar_bitmap_nodo(NODO *);

int recuperar_metadata_archivos(void);

int recuperar_metadata_un_arhcivo(char *);

// Funciones de creacion de estructuras administrativas en un inicio limpio

void crear_subdirectorios(void);

int iniciar_arbol_directorios(void);

int iniciar_nodos(void);

int iniciar_bitmaps_nodos(void);


// Funciones para agregar/sacar/modificar/administrar elementos de las diferentes estructuras ya creadas (y actualizar archivos a la vez)

/* Dado un path (sin nombre de archivo) recorre el arbol de directorios y devuelve
 * -9 si el path no existe 0-si es "/"(root) N>0 indice del último directorio del path
 * Ejemplo: /user/Maru/fotos -> 3 (indice de /fotos) si existe -> -9 si no */
int existe_ruta_directorios(char *path);

/* Dado nombre de 1 directorio(nombre carpeta) y supuesto padre devuelve -9 si no
 * existe o N indice si existe en array de directorios */
int existe_dir_en_padre(char *nombre,int padre);

// 1- Existe archivo en ese directorio 0- No existe archivo en ese directorio
bool existe_archivo(char *nom,int padre);

/* Devuelve char array, en la primera posicion la ruta sin el archivo , en la segunda
 * el nombre de archivo
 * ejemplo: char **return = sacar_archivo("/home/utnso/archivo.csv")
 *  		return[0]=/home/utnso/ return[1]=archivo.csv*/
char **sacar_archivo(char *fullpath);
// Devuelve el indice en el que lo agregó
int agregar_directorio(char *nombre,int padre);

t_archivo *get_metadata_archivo(char *path);

t_archivo *get_metadata_archivo_sinvalidar(char *path,int padre);

NODO *get_NODO(char *nombre);

#endif /* FS_ADMINISTRACION_H_ */
