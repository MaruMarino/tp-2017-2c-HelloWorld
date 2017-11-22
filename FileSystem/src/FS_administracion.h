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

int recuperar_metadata_un_arhcivo(char *,int);

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
int existe_dir_en_padre(char *nombre, int padre);

// 1- Existe archivo en ese directorio 0- No existe archivo en ese directorio
bool existe_archivo(char *nom, int padre);

/* Devuelve char array, en la primera posicion la ruta sin el archivo , en la segunda
 * el nombre de archivo
 * ejemplo: char **return = sacar_archivo("/home/utnso/archivo.csv")
 *  		return[0]=/home/utnso/ return[1]=archivo.csv*/
char **sacar_archivo(char *fullpath);

// Devuelve el indice en el que lo agregó
int agregar_directorio(char *nombre, int padre);

t_archivo *get_metadata_archivo(char *path);

t_archivo *get_metadata_archivo_sinvalidar(char *path, int padre);

NODO *get_NODO(char *nombre);

int liberarBloqueNodo(char *nameNodo, unsigned int numBlock);

bool directoryEmpty(int index);
// Re-persiste el archivo
void actualizar_arbol_directorios(void);
// Re-Calcular tamaño FS
void actualizar_FS_free(void);
// Por las sincronizar bitmaps mapeados a memoria
void sincronizar_bitmaps();
// Re-persiste el archivo
void actualizar_tabla_nodos(void);
// Crear estructuras Fisicas en FS-local para un archivo// tambien para actulizar
void crear_metadata_archivo(t_archivo *arch);
//eliminar metadata de un archivo
void eliminar_metadata_archivo(t_archivo *arch);
// eliminar directorio de una, sin verificar si vacío, donde estaban sus archivos
void eliminar_directorio(int index);



#endif /* FS_ADMINISTRACION_H_ */
