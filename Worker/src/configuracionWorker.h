#ifndef CONFIGURACION_WORKER_H_
#define CONFIGURACION_WORKER_H_

typedef struct{
	char *ip_fs,
		 *puerto_fs,
		 *nombre_nodo,
		 *puerto_worker,
		 *ruta_databin;
} t_conf;

t_conf *cargarConfig(char *path);
void mostrarConfig(t_conf *conf);
void liberarConfig(t_conf *conf);

#endif /* CONFIGURACION_WORKER_H_ */
