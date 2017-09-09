#include <stdio.h>
#include <stdlib.h>

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>

#include "configuracionWorker.h"

extern t_log *logger;

struct conf_worker *cargarConfig(char *path){
	log_trace(logger, "Se cargara la configuracion del path: %s\n", path);

	t_config *conf = config_create(path);
	struct conf_worker *conf_w = malloc(sizeof *conf_w);

	conf_w->ip_filesystem =     string_duplicate(config_get_string_value(conf, "IP_FILESYSTEM"));
	conf_w->puerto_filesystem = string_duplicate(config_get_string_value(conf, "PUERTO_FILESYSTEM"));
	conf_w->nombre_nodo =       string_duplicate(config_get_string_value(conf, "NOMBRE_NODO"));
	conf_w->puerto_worker =     string_duplicate(config_get_string_value(conf, "PUERTO_WORKER"));
	conf_w->ruta_databin =      string_duplicate(config_get_string_value(conf, "RUTA_DATABIN"));

	return conf_w;
}

void mostrarConfig(struct conf_worker *conf){

	log_info(logger, "IP_FILESYSTEM: %s",     conf->ip_filesystem);
	log_info(logger, "PUERTO_FILESYSTEM: %s", conf->puerto_filesystem);
	log_info(logger, "NOMBRE_NODO: %s",       conf->nombre_nodo);
	log_info(logger, "PUERTO_WORKER: %s",     conf->puerto_worker);
	log_info(logger, "RUTA_DATABIN: %s",      conf->ruta_databin);

}

void liberarConfig(struct conf_worker *conf){
	free(conf->ip_filesystem);
	free(conf->puerto_filesystem);
	free(conf->nombre_nodo);
	free(conf->puerto_worker);
	free(conf->ruta_databin);
	free(conf);
}


