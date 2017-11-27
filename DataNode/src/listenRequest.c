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
#include <asm/errno.h>
#include <bits/errno.h>
#include <errno.h>

t_log *_log_file;
int _socketCliente;
void *_dataBin;

int enviarBloque(header *req, void *buffer);

int insertarBloqueData(header *req, void *buffer);

void listenRequest(int socketCliente, t_log *file_log, void *dataBin) {
    _log_file = file_log;
    _socketCliente = socketCliente;
    _dataBin = dataBin;
    int controle;

    escribir_log(file_log, "****Escuchando a FileSystem*****");
    void *bufferReq;
    header headerReq;
    while (1) {
        log_info(file_log, "-----Esperando Peticion----");
        bufferReq = getMessageIntr(_socketCliente, &headerReq, &controle);
        log_info(file_log, "----Peticion Detectada----");
        if (bufferReq == NULL) {
            escribir_error_log(file_log, "Se desconecto el server");
            return;
        }
        switch (headerReq.codigo) {
            case 1: {
                if (enviarBloque(&headerReq, bufferReq) == -1) {
                    return;
                }
                break;
            }
            case 2: {
                if (insertarBloqueData(&headerReq, bufferReq) == -1) {
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

int enviarBloque(header *req, void *buffer) {
    unsigned int numBloque;
    int control;
    memcpy(&numBloque, buffer, req->sizeData);
    log_info(_log_file, "Buscando Bloque [%d]", numBloque);
    void *bloqueData = getBloque(&_dataBin, numBloque);
    header headSend;
    headSend.letra = 'D';
    headSend.codigo = 1;
    headSend.sizeData = megaByte;
    message *bufferRes = createMessage(&headSend, bloqueData);
    log_info(_log_file, "Enviando data del bloque [%d]", numBloque);
    if (enviar_messageIntr(_socketCliente, bufferRes, _log_file, &control) < 0) {
        escribir_log(_log_file, "Error al enviar el bloque");
        return -1;
    }
    free(bloqueData);
    free(bufferRes->buffer);
    free(bufferRes);
    return 0;
}

int insertarBloqueData(header *req, void *buffer) {
    size_t bloqueData = req->sizeData - sizeof(int);
    if (bloqueData > megaByte) {
        escribir_log(_log_file, "El bloque de dato que se intenta escribir es mayor a un mega");
        return -1;
    }
    unsigned int numBloque;
    void *newbloqueData = malloc(bloqueData);
    memcpy(&numBloque, buffer, sizeof(int));
    memcpy(newbloqueData, (buffer + sizeof(int)), bloqueData);
    log_info(_log_file, "----Insertando Bloque [%d]----", numBloque);
    insertBloque(&_dataBin, &newbloqueData, numBloque, bloqueData);
    free(newbloqueData);
    return 1;
}
