//
// Created by elmigue on 09/09/17.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include <commons/config.h>


config *load_config(char *path) {
    t_config *bufferConfig = config_create(path);
    config *configuracion = malloc(sizeof(config));
//hola

    configuracion->ip_filesystem = strdup(config_get_string_value(bufferConfig, "IP_FILESYSTEM"));
    configuracion->nombre_nodo = strdup(config_get_string_value(bufferConfig, "NOMBRE_NODO"));
    configuracion->puerto_dateNode = strdup(config_get_string_value(bufferConfig, "PUERTO_DATANODE"));
    configuracion->puerto_filesystem = strdup(config_get_string_value(bufferConfig, "PUERTO_FILESYSTEM"));
    configuracion->ruta_databin = strdup(config_get_string_value(bufferConfig, "RUTA_DATABIN"));
    configuracion->ip_worker = strdup(config_get_string_value(bufferConfig, "IP_WORKER"));
    configuracion->puerto_worker = strdup(config_get_string_value(bufferConfig, "PUERTO_WORKER"));

    config_destroy(bufferConfig);
    return configuracion;

}
