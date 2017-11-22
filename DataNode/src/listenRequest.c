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

t_log *_log_file;
int _socketCliente;
void *_dataBin;

int enviarBloque(header *req, void **buffer);

int insertarBloqueData(header *req, void **buffer);

void listenRequest(const int *socketCliente, t_log **file_log, void **dataBin) {
    _log_file = *file_log;
    _socketCliente = *socketCliente;
    _dataBin = *dataBin;
    int controle;

    escribir_log(*file_log, "Escuchando a maru a q me hable");
    void *bufferReq;
    header headerReq;
    while (1) {
        bufferReq = getMessageIntr(_socketCliente, &headerReq, &controle);
        if (bufferReq == NULL) {
            escribir_error_log(*file_log, "Se desconecto el server");
            return;
        }
        switch (headerReq.codigo) {
            case 1: {
                if (enviarBloque(&headerReq, &bufferReq) == -1) {
                    return;
                }
                break;
            }
            case 2: {
                if (insertarBloqueData(&headerReq, &bufferReq) == -1) {
                    return;
                }
                break;
            }
            default: {
            }
        }

        //free(bufferReq);
    }
}

int enviarBloque(header *req, void **buffer) {
    escribir_log(_log_file, "Buscando Bloque");
    unsigned int numBloque;
    memcpy(&numBloque, *buffer, req->sizeData);
    void *bloqueData = getBloque(&_dataBin, numBloque);
    printf("------Buscando bloque ----[%d]\n", numBloque);
    header headSend;
    headSend.letra = 'D';
    headSend.codigo = 1;
    headSend.sizeData = megaByte;
    message *bufferRes = createMessage(&headSend, bloqueData);
    if (send(_socketCliente, bufferRes->buffer, bufferRes->sizeBuffer, 0) < 0) {
        escribir_log(_log_file, "Error al enviar el bloque");
        return -1;
    }
    return 0;
}

int insertarBloqueData(header *req, void **buffer) {
    size_t bloqueData = req->sizeData - sizeof(int);
    if (bloqueData > megaByte) {
        escribir_log(_log_file, "El bloque de dato que se intenta escribir es mayor a un mega");
        return -1;
    }
    unsigned int numBloque;
    void *newbloqueData = malloc(bloqueData);
    memcpy(&numBloque, *buffer, sizeof(int));
    memcpy(newbloqueData, (*buffer + sizeof(int)), req->sizeData);
    printf("------insertando en el bloque ----[%d]\n", numBloque);
    insertBloque(&_dataBin, &newbloqueData, numBloque, bloqueData);

    return 1;
}
