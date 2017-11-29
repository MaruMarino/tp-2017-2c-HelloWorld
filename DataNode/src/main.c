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

int waitAccept(int socket, t_log *file_log, config *config,size_t sizeDataBin);
void liberar_memoria(config *c,t_log* l);

int main(int argc, char *argv[]) {

    t_log *file_log = crear_archivo_log("DateNode", true,
                                        "/home/utnso/logDataNode");
    int control;

    escribir_log(file_log, "cargando el archivo de configuracion");
    config *condifguracion = load_config(argv[1]);

    escribir_log(file_log, "Abriendo DataBin");
    size_t sizeDataBin;
    size_t sizeFirstDefault = megaByte * 20;
    if (argv[2] != NULL) {
        sizeFirstDefault = megaByte * (size_t) strtol(argv[2],&argv[2],10);
    }
    void *dataBin= openDateBin(condifguracion->ruta_databin, &sizeDataBin, sizeFirstDefault);
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

    if (waitAccept(socketCliente, file_log, condifguracion, sizeDataBin) == -1) {
        return -1;
    }


    listenRequest(socketCliente, file_log, dataBin);

    munmap(dataBin, sizeDataBin);
    close(socketCliente);
    liberar_memoria(condifguracion,file_log);


    return 0;
}


int waitAccept(int socket, t_log *file_log, config *config,size_t sizeDataBin) {
    int controler;

    header response;
    response.letra = 'D';
    response.codigo = 1;
    response.sizeData = 0;

    t_nodo *InfoNodo = malloc(sizeof(t_nodo));

    InfoNodo->ip = config->ip_worker;
    InfoNodo->nodo = config->nombre_nodo;
    InfoNodo->puerto = (int) strtol(config->puerto_worker, &config->puerto_worker, 10);
    InfoNodo->sizeDatabin = (int) sizeDataBin;

    void *Buffer = serializar_nodo(InfoNodo, &response.sizeData);

    message *res = createMessage(&response, Buffer);
    if (enviar_messageIntr(socket,res,file_log,&controler) < 0) {
        escribir_error_log(file_log, "No pudo enviar el mjs de aceptacion");
        free(InfoNodo);
        free(Buffer);
        free(res->buffer);
        free(res);
        return -1;
    }

    int aceeptM = 0;
    free(Buffer);
    Buffer = getMessage(socket, &response, &controler);

    if (Buffer == NULL) {
        escribir_error_log(file_log, "No se puedo recibir el mensaje");
        free(InfoNodo);
        free(res->buffer);
        free(res);
        return -1;
    }
    memcpy(&aceeptM, Buffer, sizeof(int));

    if (aceeptM) {
        escribir_log(file_log, "Aceptado por FileSystem");
        free(InfoNodo);
        free(Buffer);
        free(res->buffer);
        free(res);
        return 0;
    } else {
        escribir_error_log(file_log, "No aceptado por FileSystem");
        free(InfoNodo);
        free(Buffer);
        free(res->buffer);
        free(res);
        return -1;
    }
}

void liberar_memoria(config *c,t_log* l){

	free(c->ip_filesystem);
	free(c->ip_worker);
	free(c->nombre_nodo);
	free(c->puerto_dateNode);
	free(c->puerto_filesystem);
	//free(c->puerto_worker);
	free(c->ruta_databin);

	free(c);

	log_destroy(l);

}
