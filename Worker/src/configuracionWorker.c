#include <stdio.h>
#include <stdlib.h>

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>

#include "configuracionWorker.h"

#define mib 0x100000 // 1 MiB

extern t_log *logw;

struct conf_worker *cargarConfig(char *path){
	log_trace(logw, "Se cargara la configuracion del path: %s\n", path);

	t_config *conf = config_create(path);
	struct conf_worker *conf_w = malloc(sizeof *conf_w);

	conf_w->ip_fs =     string_duplicate(config_get_string_value(conf, "IP_FILESYSTEM"));
	conf_w->puerto_fs = string_duplicate(config_get_string_value(conf, "PUERTO_FILESYSTEM"));
	conf_w->nombre_nodo =       string_duplicate(config_get_string_value(conf, "NOMBRE_NODO"));
	conf_w->puerto_worker =     string_duplicate(config_get_string_value(conf, "PUERTO_WORKER"));
	conf_w->ruta_databin =      string_duplicate(config_get_string_value(conf, "RUTA_DATABIN"));
	conf_w->size_default = config_get_int_value(conf, "SIZE_DEFAULT") * mib;

	return conf_w;
}

void mostrarConfig(struct conf_worker *conf){

	log_info(logw, "IP_FILESYSTEM: %s",     conf->ip_fs);
	log_info(logw, "PUERTO_FILESYSTEM: %s", conf->puerto_fs);
	log_info(logw, "NOMBRE_NODO: %s",       conf->nombre_nodo);
	log_info(logw, "PUERTO_WORKER: %s",     conf->puerto_worker);
	log_info(logw, "RUTA_DATABIN: %s",      conf->ruta_databin);

}

void liberarConfig(struct conf_worker *conf){
	free(conf->ip_fs);
	free(conf->puerto_fs);
	free(conf->nombre_nodo);
	free(conf->puerto_worker);
	free(conf->ruta_databin);
	free(conf);
}


