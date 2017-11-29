#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>

#include <funcionesCompartidas/generales.h>

#include "configuracionWorker.h"

#define mib 0x100000 // 1 MiB

extern t_log *logw;

t_conf *cargarConfig(char *path){
	log_trace(logw, "Se cargara la configuracion del path: %s\n", path);

	t_config *conf = config_create(path);
	t_conf *conf_w = malloc(sizeof *conf_w);
	char *ip, *port, *node, *ipw, *portw, *dbpath;
	ip     = config_get_string_value(conf, "IP_FILESYSTEM");
	port   = config_get_string_value(conf, "PUERTO_FILESYSTEM");
	node   = config_get_string_value(conf, "NOMBRE_NODO");
	ipw    = config_get_string_value(conf, "IP_WORKER");
	portw  = config_get_string_value(conf, "PUERTO_WORKER");
	dbpath = config_get_string_value(conf, "RUTA_DATABIN");

	conf_w->ip_fs =         strdup(ip);
	conf_w->puerto_fs =     strdup(port);
	conf_w->nombre_nodo =   strdup(node);
	conf_w->ip_worker =     strdup(ipw);
	conf_w->puerto_worker = strdup(portw);
	conf_w->ruta_databin =  strdup(dbpath);

	config_destroy(conf);
	return conf_w;
}

void mostrarConfig(t_conf *conf){

	log_info(logw, "IP_FILESYSTEM:     %s", conf->ip_fs);
	log_info(logw, "PUERTO_FILESYSTEM: %s", conf->puerto_fs);
	log_info(logw, "NOMBRE_NODO:       %s", conf->nombre_nodo);
	log_info(logw, "IP_WORKER:         %s", conf->ip_worker);
	log_info(logw, "PUERTO_WORKER:     %s", conf->puerto_worker);
	log_info(logw, "RUTA_DATABIN:      %s", conf->ruta_databin);
}

void liberarConfig(t_conf *conf){
	liberador(7, conf->ip_fs, conf->puerto_fs, conf->nombre_nodo, conf->ip_worker,
			     conf->puerto_worker, conf->ruta_databin, conf);
}
