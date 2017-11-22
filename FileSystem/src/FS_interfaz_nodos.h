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

t_list  *get_copia_nodos_activos();

void disconnectedNodo(int socket);
/*todo Funciones para realizar Leer Archivo*/

void *leer_bloque(bloqueArchivo *bq,int copia); //si copia == 1 me fijo primero en la copia, else primero en el original

int crear_archivo_temporal(t_archivo *archivo,char *nomre_temporal);

#endif /* FS_INTERFAZ_NODOS_H_ */
