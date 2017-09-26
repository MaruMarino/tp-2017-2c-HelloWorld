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
	char *temp_transformacion;
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
	int len;
	char *fname;
} t_fname;

typedef struct
{
	size_t size_prog; // tamanio en bytes del programa
	char *prog; // programa de transferencia
	size_t bloque; // bloque sobre el cual aplicar
	int bytes_ocup; // bytes ocupados por bloque
	t_fname file_out; // nombre del archivo a generar
} t_info_trans;

typedef struct
{
	size_t size_prog; // tamanio en bytes del programa
	char *prog; // programa de transferencia
	t_list *files; // lista compuesta por t_fname
	t_fname file_out;
} t_info_redLocal, t_info_redGlobal; // la recibe el encargado de reduc global

typedef struct
{
	int iplen; // cant caracteres del IP
	char *ip; // ip del Worker responsable de reduccion
	int portlen; // cant caracteres del puerto
	char *port; // puerdo del Worker responsable de reduccion
	t_fname file_in; // archivo que debe enviar
} t_info_redGlobalSub;

#endif /* FUNCIONESCOMPARTIDAS_ESTRUCTURAS_H_ */
