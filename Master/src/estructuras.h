#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <time.h>

typedef struct
{
	char *ip;
	char *puerto;
	int socket_yama;
	char *path_conf;
	char *script_trans;
	char *script_reduc;
	char *path_file_target;
	char *path_file_destino;
}t_configuracion;

typedef struct
{
	int transf_ejecutando;
	int reduc_ejecutando;
	int transf_paralelo;
	int reduc_paralelo;
	int transf_total;
	int reduc_total;
	int reduc_glo_total;
	int alm_total;
	int fallo_transf;
	int fallo_reduc_local;
	int fallo_reduc_global;
	int fallo_almacenamiento;
	time_t *inicio_trans;
	time_t *fin_trans;
	time_t *inicio_reduc_local;
	time_t *fin_reduc_local;
	time_t *inicio_reduc_global;
	time_t *fin_reduc_global;
	time_t *inicio_alm;
	time_t *fin_alm;
}t_estadistica;

#endif /* ESTRUCTURAS_H_ */
