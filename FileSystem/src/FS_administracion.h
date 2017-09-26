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
int recuperar_arbol_directorios(void );
int recuperar_nodos(void);
int recuperar_bitmap_nodo(NODO *);
int recuperar_metadata_archivos(void);
int recuperar_metadata_un_arhcivo(char *);

// Funciones de creacion de estructuras administrativas en un inicio limpio
void crear_subdirectorios(void);
int iniciar_arbol_directorios(void);
int iniciar_nodos(void);
int iniciar_bitmaps_nodos(void);

// Funciones para agregar/sacar/modificar elementos de las diferentes estructuras ya creadas (y actualizar archivos a la vez)
// todo proximamente



#endif /* FS_ADMINISTRACION_H_ */
