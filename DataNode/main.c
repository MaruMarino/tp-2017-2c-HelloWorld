#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/log.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/socket.h>
#include <memory.h>
#include "config.h"
#include "dataBin.h"
#include "listenRequest.h"

typedef struct {
    char *name;
    size_t name;
    unsigned int portDateNodo;
    unsigned int portWorker;
    char *ipWorker;
    size_t *sizeIpWorker;
    size_t sizeDataBin;
}Nodo;

int waitAccept(const int *socket ,t_log **file_log,config **config);
int main(int argc, char *argv[]){

    char *l = string_duplicate(" miguel ");

    t_log * file_log = crear_archivo_log("DateNode",true,"/home/elmigue/Documents/log/log");
    int control;


    escribir_log(file_log,"cargando el archivo de configuracion");
    config  *condifguracion = load_config(argv[1]);

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

    if(waitAccept(&socketCliente,&file_log,&condifguracion) == -1){
        escribir_error_log(file_log,"Error de aceptacion");
    }

    listenRequest(&socketCliente,&file_log,&dataBin);

    munmap(dataBin,sizeDataBin);
    close(socketCliente);

    return 0;
}


int waitAccept(const int *socket,t_log **file_log,config **config){

    void *Buffer = malloc(0);

    header response;
    response.letra = 'D';
    response.codigo = 1;
    response.sizeData = 0;

    message *res = createMessage(&response,Buffer);
    if (send(*socket, res->buffer, res->sizeBuffer, 0) < 0) {
        escribir_error_log(*file_log,"No pudo enviar el mjs de aceptacion");
        return -1;
    }
    int aceeptM;
    Buffer = getMessage(*socket,&response);

    memcpy(&aceeptM,Buffer,sizeof(int));
    if(aceeptM == 1){
        return 0;
    } else{
        return -1;
    }
}

