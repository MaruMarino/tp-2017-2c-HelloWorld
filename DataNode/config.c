//
// Created by elmigue on 09/09/17.
//
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <commons/config.h>


struct config* load_config(char* path){
    printf("cargan el path [%s]\n",path);
    t_config *config = config_create(path);
    struct config *load_conf = malloc(sizeof *load_conf);

    load_conf->ip_filesystem =  config_get_string_value(config,"IP_FILESYSTEM");
    load_conf->nombre_nodo =  config_get_string_value(config,"NOMBRE_NODO");
    load_conf->puerto_dateNode =  config_get_string_value(config,"PUERTO_DATANODE");
    load_conf->puerto_filesystem =  config_get_string_value(config,"PUERTO_FILESYSTEM");
    load_conf->ruta_databin =  config_get_string_value(config,"RUTA_DATABIN");

    return load_conf;

}