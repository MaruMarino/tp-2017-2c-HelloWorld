#ifndef CONFIGURACION_WORKER_H_
#define CONFIGURACION_WORKER_H_

struct conf_worker {
	char *ip_fs,
		 *puerto_fs,
		 *nombre_nodo,
		 *puerto_worker,
		 *ruta_databin;
	int size_default;
};

struct conf_worker *cargarConfig(char *path);
void mostrarConfig(struct conf_worker *conf);
void liberarConfig(struct conf_worker *conf);

#endif /* CONFIGURACION_WORKER_H_ */
