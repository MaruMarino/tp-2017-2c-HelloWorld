#ifndef FUNCIONESCOMPARTIDAS_ESTRUCTURAS_H_
#define FUNCIONESCOMPARTIDAS_ESTRUCTURAS_H_

#include <commons/collections/list.h>

typedef struct
{
	char *nodo;
	char *ip;
	int puerto;
	int sizeDatabin;
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
	t_list *archivos_temp;
	char *temp_red_local;
}t_redLocal;

typedef struct
{
	t_nodo *nodo;
	char *temp_red_local;
	char *red_global;
	int encargado;
}t_redGlobal;

typedef struct
{
	t_nodo *nodo;
	char *red_global;
}t_almacenado;

typedef struct
{
	char *nodo;
	int bloque;
	int estado; //1: En proceso 2:Finalizado OK 3:Error

}t_estado_master;

/* Estructuras que precisa Worker */

typedef struct
{
	size_t size_prog; // tamanio en bytes del programa
	char *prog; // programa de transferencia
	size_t bloque; // bloque sobre el cual aplicar
	int bytes_ocup; // bytes ocupados por bloque
	char *file_out; // nombre del archivo a generar
} t_info_trans;

typedef struct
{
	size_t size_prog; // tamanio en bytes del programa
	char *prog; // programa de reduccion local
	t_list *files; // lista compuesta por string's
	char *file_out;
} t_info_redLocal; // la recibe el encargado de reduc global

typedef struct
{
	char *ip;
	char *port;
	char *fname;
} t_info_nodo;

typedef struct
{
	size_t size_prog; // tamanio en bytes del programa
	char *prog; // programa de reduccion global
	t_list *nodos; // lista con t_info_nodo
	char *file_out;
} t_info_redGlobal;

typedef struct
{
	char *fname;
	off_t fsize;
	char *data;
}t_file; // para facilitar pasar un archivo con un nombre particular

typedef struct{

	char *nodo0; // nombre nodo donde esta la copia 0 de ese bloque del archivo
	int bloquenodo0; // bloque dentro del nodo donde esta la copia 0 de ese bloque del archivo
	char *nodo1; // nombre nodo donde esta la copia 1 de ese bloque del archivo
	int bloquenodo1; // nombre nodo donde esta la copia 1 de ese bloque del archivo
	int bytesEnBloque; // cantidad de bytes que conforma ese bloque ( <= 1MiB)

}bloqueArchivo;

#endif /* FUNCIONESCOMPARTIDAS_ESTRUCTURAS_H_ */
