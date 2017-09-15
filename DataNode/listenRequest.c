//
// Created by elmigue on 14/09/17.
//

#include "listenRequest.h"
#include <funcionesCompartidas/funcionesNet.h>
#include "dataBin.h"
#include <funcionesCompartidas/log.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

t_log *_log_file;
int _socketCliente;
void *_dataBin;

int enviarBloque(header *req, void **buffer);

void listenRequest(const int *socketCliente, t_log **file_log, void **dataBin) {
    _log_file = *file_log;
    _socketCliente = *socketCliente;
    _dataBin = *dataBin;

    escribir_log(*file_log, "Escuchando a maru a q me hable");
    void *bufferReq;
    header headerReq;
    while (1) {
        bufferReq = getMessage(_socketCliente, &headerReq);
        if (bufferReq == NULL) {
            escribir_error_log(*file_log,"Se desconecto el server");
            return;
        }
        switch (headerReq.codigo) {
            case 1: {
                if (enviarBloque(&headerReq, &bufferReq) == -1) {
                    return;
                }
                break;
            }
            default: {
            }
        }

        free(bufferReq);
    }
}


int enviarBloque(header *req, void **buffer) {
    write(STDIN_FILENO, *buffer, req->sizeData);
    header headSend;
    char *mensaje = "Hola (*(*(*(*.*)*)*)*)";

    headSend.letra = 'D';
    headSend.codigo = 1;
    headSend.sizeData = strlen(mensaje);

    message *bufferRes = createMessage(&headSend, mensaje);

    if (send(_socketCliente, bufferRes->buffer, bufferRes->sizeBuffer, 0) < 0) {
        escribir_log(_log_file, "Error al enviar el bloque");
        return -1;
    }

    return 0;

}


