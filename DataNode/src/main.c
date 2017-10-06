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
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/serializacion_yama_master.h>

int waitAccept(const int *socket, t_log **file_log, config *config, const size_t *sizeDataBin);

int main(int argc, char *argv[]) {

    t_log *file_log = crear_archivo_log("DateNode", true,
                                        "/home/elmigue/Desktop/workSpace/tp-2017-2c-HelloWorld/DataNode/src/log/log");
    int control;

    escribir_log(file_log, "cargando el archivo de configuracion");
    config *condifguracion = load_config(argv[1]);

    escribir_log(file_log, "Abriendo DataBin");
    size_t sizeDataBin;
    size_t sizeFirstDefault = megaByte * 20;
    if (argv[2] != NULL) {
        sizeFirstDefault = megaByte * (size_t) strtol(argv[2],&argv[2],10);
    }
    void *dataBin = openDateBin(&condifguracion->ruta_databin, &sizeDataBin, sizeFirstDefault);
    if (dataBin == NULL) {
        escribir_error_log(file_log, "Error al abrir el data.bin");
        return 1;
    }

    escribir_log(file_log, "Estableciendo Conneccion al File System");
    int socketCliente = establecerConexion(condifguracion->ip_filesystem, condifguracion->puerto_filesystem, file_log,
                                           &control);
    if (control != 0) {
        escribir_error_log(file_log, "Error al intentar establecer conneccion");
        return -1;
    }

    if (waitAccept(&socketCliente, &file_log, condifguracion, &sizeDataBin) == -1) {
        return -1;
    }


    listenRequest(&socketCliente, &file_log, &dataBin);

    munmap(dataBin, sizeDataBin);
    close(socketCliente);

    return 0;
}


int waitAccept(const int *socket, t_log **file_log, config *config, const size_t *sizeDataBin) {
    int controler;

    header response;
    response.letra = 'D';
    response.codigo = 1;
    response.sizeData = 0;

    t_nodo *InfoNodo = malloc(sizeof(t_nodo));

    InfoNodo->ip = config->ip_worker;
    InfoNodo->nodo = config->nombre_nodo;
    InfoNodo->puerto = (int)strtol(config->puerto_worker,&config->puerto_worker,10);
    InfoNodo->sizeDatabin = (int) *sizeDataBin;

    void *Buffer = serializar_nodo(InfoNodo, &response.sizeData);

    message *res = createMessage(&response, Buffer);
    if (send(*socket, res->buffer, res->sizeBuffer, 0) < 0) {
        escribir_error_log(*file_log, "No pudo enviar el mjs de aceptacion");
        return -1;
    }

    int aceeptM = 0;
    Buffer = getMessage(*socket, &response, &controler);

    if(Buffer == NULL){
        escribir_error_log(*file_log, "No se puedo recibir el mensaje");
        return -1;
    }
    memcpy(&aceeptM, Buffer, sizeof(int));

    if (aceeptM) {
        escribir_log(*file_log,"Aceptado por FileSystem");
        return 0;
    } else {
        escribir_error_log(*file_log,"No aceptado por FileSystem");
        return -1;
    }
}

