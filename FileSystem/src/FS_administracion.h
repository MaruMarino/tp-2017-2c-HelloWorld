/*
 * FS_administracion.h
 *
 *  Created on: 19/9/2017
 *      Author: utnso
 */

#ifndef FS_ADMINISTRACION_H_
#define FS_ADMINISTRACION_H_

#include "estructurasfs.h"

int recuperar_estructuras_administrativas(void);
char *completar_path_metadata(char *);
int recuperar_arbol_directorios(void );
int recuperar_nodos(void);
int recuperar_bitmap_nodo(NODO *);

#endif /* FS_ADMINISTRACION_H_ */
