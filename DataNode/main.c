#include <stdio.h>
#include <commons/temporal.h>
#include <stdlib.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/log.h>
#include <funcionesCompartidas/mensaje.h>
#include <sys/mman.h>
#include <unistd.h>
#include "config.h"
#include "OpenArchiveBin.h"
#define megaByte 1048576


int main(int argc, char *argv[]){

    t_log * file_log;

    printf("%s\n",temporal_get_string_time());

    file_log = crear_archivo_log("DateNode",true,"/home/elmigue/Documents/log/log");
    escribir_log(file_log,"cargando el archivo de configuracion");
    struct config  *condifguracion = load_config(argv[1]);

    escribir_log(file_log,"Abriendo DataBin");
    size_t sizeDataBin;
    char * dataBin = openDateBin(condifguracion->ruta_databin,&sizeDataBin,((size_t)megaByte)*20);
    if(dataBin == NULL){
        escribir_error_log(file_log,"Error al abrir el data.bin");
        return 1;
    }

    escribir_log(file_log,"Estableciendo Conneccion al File System");
    int cliente = establecerConexion(condifguracion->ip_filesystem,condifguracion->puerto_filesystem);
    if(cliente == -1){
        escribir_error_log(file_log,"Error al intentar establecer conneccion");
        return -1;
    }

    escribir_log(file_log,"Escuchando a maru a q me hable");
    char *mensaje = NULL;
    char *sendMjs = NULL;
    bool FlagFile = true;
    while(FlagFile){
        mensaje = recibir(cliente);
        if(mensaje == NULL){
            FlagFile = false;
        }
        int codigo = get_codigo(mensaje);
        switch (codigo){
            case 1:{
                sendMjs = armar_mensaje("D01","Todo Piola");
                enviar(cliente,sendMjs);
                break;
            }
            case 2:{
                sendMjs = armar_mensaje("D02","Al Toke Perro");
                enviar(cliente,sendMjs);
                break;
            }
            default:{
                sendMjs = NULL;
            };
        }
    }
    escribir_error_log(file_log,"Se deconecto el cliente");
    free(mensaje);
    free(sendMjs);
    munmap(dataBin,sizeDataBin);
    close(cliente);
    return 0;
}