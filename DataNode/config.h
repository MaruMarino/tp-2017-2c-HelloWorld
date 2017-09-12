//
// Created by elmigue on 09/09/17.
//

#ifndef TP_2017_2C_HELLOWORLD_CONFIG_H
#define TP_2017_2C_HELLOWORLD_CONFIG_H
struct config {
    char *ip_filesystem,
            *puerto_filesystem,
            *nombre_nodo,
            *puerto_dateNode,
            *ruta_databin;
};

struct config* load_config(char* path);


#endif //TP_2017_2C_HELLOWORLD_CONFIG_H
