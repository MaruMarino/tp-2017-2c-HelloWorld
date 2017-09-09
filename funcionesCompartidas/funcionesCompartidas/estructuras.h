/*
 * estructuras.h
 *
 *  Created on: 9/9/2017
 *      Author: utnso
 */

#ifndef FUNCIONESCOMPARTIDAS_ESTRUCTURAS_H_
#define FUNCIONESCOMPARTIDAS_ESTRUCTURAS_H_

typedef struct
{
	char *nodo;
	char *ip;
	char *puerto;
}t_nodo;

typedef struct
{
	t_nodo *nodo;
	int bloque;
	int bytes;
	char *temporal;
}t_transformacion;

typedef struct
{
	t_nodo *nodo;
	char *temp_transformacion;
	char *temp_red_local;
}t_redLocal;

typedef struct
{
	t_nodo nodo;
	char *temp_red_local;
	char *red_global;
	int encargado;
}t_redGlobal;

typedef struct
{
	t_nodo nodo;
	char *red_global;
}t_almacenado;

typedef struct
{
	char *nodo;
	int bloque;
	int estado; //1: En proceso 2:Finalizado OK 3:Error
};



#endif /* FUNCIONESCOMPARTIDAS_ESTRUCTURAS_H_ */
