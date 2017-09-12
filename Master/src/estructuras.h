#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

typedef struct
{
	char *ip;
	char *puerto;
	int socket_yama;
	char *path_conf;
	char *path_trans;
	char *path_reduc;
	char *path_file_target;
	char *path_file_destino;
}t_configuracion;

typedef struct
{
	char *nodo;
	char *ip;
	int puerto;
	int bloque;
	int bytes_ocup;
	char *file_temp;
	int socket_nodo;
	char *status;
}t_transf;

#endif /* ESTRUCTURAS_H_ */
