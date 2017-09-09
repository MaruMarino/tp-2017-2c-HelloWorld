/*
 * estructuras.h
 *
 *  Created on: 8/9/2017
 *      Author: utnso
 */

#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

typedef struct
{
	char *fs_ip;
	char *fs_puerto;
	int retardo_plan;
	char *algortimo_bal;
	char *yama_ip;
	char *yama_puerto;
	int socket_fs;

}t_configuracion;

typedef struct
{

}t_estado;

#endif /* ESTRUCTURAS_H_ */
