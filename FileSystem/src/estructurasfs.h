/*
 * estructuras.h
 *
 *  Created on: 8/9/2017
 *      Author: utnso
 */

#ifndef ESTRUCTURASFS_H_
#define ESTRUCTURASFS_H_


typedef struct {
	char *ip;
	char *dir_estructuras;
	int puerto;
	int serverfs;
}yamafs_config;

typedef int Function (char *);

typedef struct {
  char *name;
  Function *func;
  char *doc;
  char *sintax;
} comando;

typedef struct{
	char *nombre;
	char *ip;
	int puerto;
	int soket;
	int espacio_total;
	int espacio_libre;
}NODO;


#endif /* ESTRUCTURASFS_H_ */
