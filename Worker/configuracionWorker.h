#ifndef WORKER_CONFIGURADORWORKER_H_
#define WORKER_CONFIGURADORWORKER_H_

struct conf_worker {
	char *ip_filesystem,
		 *puerto_filesystem,
		 *nombre_nodo,
		 *puerto_worker,
		 *ruta_databin;
};

struct conf_worker *cargarConfig(char *path);
void mostrarConfig(struct conf_worker *conf);
void liberarConfig(struct conf_worker *conf);

#endif /* WORKER_CONFIGURADORWORKER_H_ */
