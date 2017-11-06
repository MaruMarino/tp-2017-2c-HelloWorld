/*
 * manejo_tabla_estados.h
 *
 *  Created on: 8/9/2017
 *      Author: utnso
 */

#ifndef MANEJO_TABLA_ESTADOS_H_
#define MANEJO_TABLA_ESTADOS_H_

char *generar_nombre_temporal(int job, char *nodo, int bloque);
t_estado *generar_estado(int master, int bloque, char *nodo, char *nodo2, int bloque2, int bytes);
void cambiar_estado(int master, char *nodo, int bloque, e_estado nuevo_estado, char *nombre);
t_estado *get_estado(int master, char *nodo, int bloque);


#endif /* MANEJO_TABLA_ESTADOS_H_ */
