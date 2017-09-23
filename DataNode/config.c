//
// Created by elmigue on 09/09/17.
//
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <commons/config.h>


config* load_config(char* path){
    t_config * bufferConfig = config_create(path);
    config * configuracion = malloc(sizeof(config));

    configuracion->ip_filesystem =  config_get_string_value(bufferConfig,"IP_FILESYSTEM");
    configuracion->nombre_nodo =  config_get_string_value(bufferConfig,"NOMBRE_NODO");
    configuracion->puerto_dateNode =  config_get_string_value(bufferConfig,"PUERTO_DATANODE");
    configuracion->puerto_filesystem =  config_get_string_value(bufferConfig,"PUERTO_FILESYSTEM");
    configuracion->ruta_databin =  config_get_string_value(bufferConfig,"RUTA_DATABIN");
    configuracion->ruta_databin =  config_get_string_value(bufferConfig,"RUTA_DATABIN")

    return configuracion;

}