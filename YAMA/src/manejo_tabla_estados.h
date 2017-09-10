/*
 * manejo_tabla_estados.h
 *
 *  Created on: 8/9/2017
 *      Author: utnso
 */

#ifndef MANEJO_TABLA_ESTADOS_H_
#define MANEJO_TABLA_ESTADOS_H_

char *generar_nombre_temporal(int job, int nodo, int bloque);
void generar_estado(int master, int bloque, int nodo);
void cambiar_estado(int master, int job, int nodo, int bloque, e_estado nuevo_estado);
t_estado *get_estado(int master, int job, int nodo, int bloque);


#endif /* MANEJO_TABLA_ESTADOS_H_ */
