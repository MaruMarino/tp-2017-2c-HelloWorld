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

int setBlock(void *buffer, size_t size_buffer);

int exitProcess(NODO *nameNodo);

void checkStateNodos();

estado checkStateFileSystem();

void *contenido_archivo(char *pathlocal, int *fsize);

int dividir_enviar_archivo(char *contenido, int fsize, char *tipo); // tipo B-binrio T-texto

void disconnectedNodo(int socket);
/*todo Funciones para realizar Leer Archivo*/

#endif /* FS_INTERFAZ_NODOS_H_ */
