/*
 * estructuras.h
 *
 *  Created on: 8/9/2017
 *      Author: utnso
 */

#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <funcionesCompartidas/estructuras.h>

typedef struct
{
	char *fs_ip;
	char *fs_puerto;
	char *algortimo_bal;
	char *yama_puerto;
	int retardo_plan;
	int base;
	int socket_fs;
	int server_;

}t_configuracion;

typedef struct
{
	int master;
	int socket_;
}t_master;

typedef enum
{
	TRANSFORMACION,
	ESPERA_REDUCCION_LOCAL,
	REDUCCION_LOCAL,
	REDUCCION_GLOBAL,
	ALMACENAMIENTO_FINAL,
}e_etapa;

typedef enum
{
	EN_PROCESO,
	ERROR,
	FINALIZADO_OK,
}e_estado;

typedef struct
{
	int job;
	int master;
	char *nodo;
	int bloque;
	e_etapa etapa;
	char *archivo_temporal;
	e_estado estado;
	int cant_bloques_nodo;
	bool copia_disponible;
	char *nodo_copia;
	int bloque_copia;
	int bytes;
}t_estado;

typedef struct
{
	char *nodo;
	int n_bloque_archivo;
	int n_bloque;
	int bytes;
}t_bloque;

typedef struct
{
	t_nodo *nodo;
	int disponibilidad;
	int carga_actual;
	int carga_historica;
	bool clock;
	t_list *bloques;
}t_worker;

#endif /* ESTRUCTURAS_H_ */
