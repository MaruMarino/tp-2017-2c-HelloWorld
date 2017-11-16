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

/* Funciones para realizar Almacenar Archivo*/

int setBlock(void *, size_t size_buffer,t_list *ba);

int exitProcess(NODO *nameNodo);

void checkStateNodos();

estado checkStateFileSystem();


t_list *escribir_desde_archivo(char *local_path,char file_type,int filesize); // tipo B/b-binario T/t-texto

int get_file_size(char *);

bool hay_lugar_para_archivo(int size);

void disconnectedNodo(int socket);
/*todo Funciones para realizar Leer Archivo*/

#endif /* FS_INTERFAZ_NODOS_H_ */
