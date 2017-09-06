#include <stdio.h>
#include <stdlib.h>

#include <commons/config.h>
#include <commons/string.h>

#include "configuracionWorker.h"

struct conf_worker *cargarConfig(char *path){
	printf("Se cargara la configuracion del path: %s\n", path);

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

	printf("IP_FILESYSTEM: %s\n",     conf->ip_filesystem);
	printf("PUERTO_FILESYSTEM: %s\n", conf->puerto_filesystem);
	printf("NOMBRE_NODO: %s\n",       conf->nombre_nodo);
	printf("PUERTO_WORKER: %s\n",     conf->puerto_worker);
	printf("RUTA_DATABIN: %s\n",      conf->ruta_databin);

}

void liberarConfig(struct conf_worker *conf){
	free(conf->ip_filesystem);
	free(conf->puerto_filesystem);
	free(conf->nombre_nodo);
	free(conf->puerto_worker);
	free(conf->ruta_databin);
	free(conf);
}


