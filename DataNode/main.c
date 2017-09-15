#include <stdio.h>
#include <commons/string.h>
#include <stdlib.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/log.h>
#include "listenRequest.h"
#include <sys/mman.h>
#include <unistd.h>
#include "config.h"
#include "dataBin.h"
#define megaByte 1048576


int main(int argc, char *argv[]){

    char *l = string_duplicate(" miguel ");

    t_log * file_log = crear_archivo_log("DateNode",true,"/home/elmigue/Documents/log/log");
    int control;


    escribir_log(file_log,"cargando el archivo de configuracion");
    struct config  *condifguracion = load_config(argv[1]);

    escribir_log(file_log,"Abriendo DataBin");
    size_t sizeDataBin;
    void *dataBin = openDateBin(&condifguracion->ruta_databin,&sizeDataBin,((size_t)megaByte)*20);
    if(dataBin == NULL){
        escribir_error_log(file_log,"Error al abrir el data.bin");
        return 1;
    }

    escribir_log(file_log,"Estableciendo Conneccion al File System");
    int socketCliente = establecerConexion(condifguracion->ip_filesystem,condifguracion->puerto_filesystem,file_log,&control);
    if(control != 0){
        escribir_error_log(file_log,"Error al intentar establecer conneccion");
        return -1;
    }

    listenRequest(&socketCliente,&file_log,&dataBin);

    munmap(dataBin,sizeDataBin);
    close(socketCliente);

    return 0;
}