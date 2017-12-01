/*
 * FS_interfz_nodos.c
 *
 *  Created on: 28/10/2017
 *      Author: utnso
 */

#include "FS_interfaz_nodos.h"
#include "estructurasfs.h"


#include <funcionesCompartidas/funcionesNet.h>
#include <commons/bitarray.h>
#include <commons/string.h>
#include <commons/log.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/generales.h>
#include <commons/collections/list.h>
#include <sys/socket.h>
#include "FS_conexiones.h"
#include <errno.h>

#include "FS_administracion.h"


#define Mib 1048576

//extern yamafs_config *configuracion;
extern t_log *logi;
//extern t_directory directorios[100];
extern t_list *nodos;
//extern t_list *archivos;
extern t_list *archivos;


/* FUNCIONES ALMACENAR ARCHIVO */

int getBlockFree(t_bitarray *listBit) {
    int i;
    for (i = 0; i < listBit->size; ++i) {
        if (!bitarray_test_bit(listBit, i)) {
            return i;
        }
    }
    return -1;
}

NODO *getNodoMinusLoader(NODO *NodoExcluir, int *numBlock) {
    int maxLibreEspacio = 0;
    NODO *nodoMax = NULL;
    NODO *nodoFetch;
    int i;
    for (i = 0; i < nodos->elements_count; i++) {
        nodoFetch = list_get(nodos, i);
        if (NodoExcluir != NULL && NodoExcluir->soket == nodoFetch->soket) {
            continue;
        }
        if (nodoFetch->estado == disponible && nodoFetch->espacio_libre > maxLibreEspacio) {
            *numBlock = getBlockFree(nodoFetch->bitmapNodo);
            if (*numBlock != -1) {
                maxLibreEspacio = nodoFetch->espacio_libre;
                nodoMax = nodoFetch;
            }
        }
    }
    return nodoMax;
}

int setBlock(void *buffer, size_t size_buffer, t_list *bloquesArch) {
    int cantCopy = 0;
    int control;
    size_t bufferWithBlockSize = (size_buffer + sizeof(int));
    void *bufferWithBlock = malloc(bufferWithBlockSize);
    NODO *nodoSend = NULL;
    int nodoSendBlock;
    header reqRes;
    bloqueArchivo *nuevo = malloc(sizeof(bloqueArchivo));

    while (cantCopy < 2) {
        nodoSend = getNodoMinusLoader(nodoSend, &nodoSendBlock);
        if (nodoSend == NULL) {
            free(bufferWithBlock);
            free(nuevo);
            return -1;
        }
        reqRes.codigo = 2;
        reqRes.letra = 'F';
        reqRes.sizeData = bufferWithBlockSize;
        memcpy(bufferWithBlock, &nodoSendBlock, sizeof(int));
        memcpy((bufferWithBlock + sizeof(int)), buffer, size_buffer);
        message *request = createMessage(&reqRes, bufferWithBlock);

        if (enviar_messageIntr(nodoSend->soket, request, logi, &control) < 0) {
            puts("error al enviar primer bloque");
            return -1;
        }
        if (cantCopy == 0) {

            nuevo->nodo0 = strdup(nodoSend->nombre);
            nuevo->bloquenodo0 = nodoSendBlock;

        } else {

            nuevo->nodo1 = strdup(nodoSend->nombre);
            nuevo->bloquenodo1 = nodoSendBlock;

        }

        bitarray_set_bit(nodoSend->bitmapNodo, nodoSendBlock);
        nodoSend->espacio_libre -= Mib;
        cantCopy++;

        free(request->buffer);
        free(request);
    }
    nuevo->bytesEnBloque = size_buffer;
    list_add(bloquesArch, nuevo);
    free(bufferWithBlock);

    return 0;
}

int searchNodoInList(NODO *TestNodo) {
    int i;
    NODO *nodoFetch;
    for (i = 0; i < nodos->elements_count; ++i) {
        nodoFetch = list_get(nodos, i);
        if (strcmp(nodoFetch->nombre, TestNodo->nombre) == 0) {
            nodoFetch->soket = TestNodo->soket;
            nodoFetch->puerto = TestNodo->puerto;
            if (nodoFetch->ip != NULL) free(nodoFetch->ip);
            nodoFetch->ip = strdup(TestNodo->ip);
            nodoFetch->estado = disponible;
            return 1;
        }
    }
    return 0;
}

estado checkStateNodo(char *nameNodo) {
    int i;
    NODO *nodoFetch;
    for (i = 0; i < nodos->elements_count; ++i) {
        nodoFetch = list_get(nodos, i);
        if (strcmp(nodoFetch->nombre, nameNodo) == 0) {
            return nodoFetch->estado;
        }
    }
    return no_disponible;
}

estado checkStateArchive(t_archivo *archivo) {
    int i;
    bloqueArchivo *fetchBloque;
    for (i = 0; i < archivo->bloques->elements_count; ++i) {
        fetchBloque = list_get(archivo->bloques, i);
        if ((checkStateNodo(fetchBloque->nodo0) == no_disponible &&
             checkStateNodo(fetchBloque->nodo1) == no_disponible)) {
            archivo->estado = no_disponible;
            return no_disponible;
        }
    }
    archivo->estado = disponible;
    return disponible;
}

estado checkStateFileSystem() {
    int i;
    t_archivo *fetchArchivo;
    estado fileStable = disponible;
    for (i = 0; i < archivos->elements_count; ++i) {
        fetchArchivo = list_get(archivos, i);
        fetchArchivo->estado = checkStateArchive(fetchArchivo);
        if (fetchArchivo->estado == no_disponible) {
            fileStable = no_disponible;
        }
    }
    return fileStable;
}

void disconnectedNodo(int socket) {
    int i;
    NODO *nodoFetch;
    for (i = 0; i < nodos->elements_count; ++i) {
        nodoFetch = list_get(nodos, i);
        if (nodoFetch->soket == socket && nodoFetch->estado == disponible) {
            nodoFetch->estado = no_disponible;
        }
    }
    checkStateFileSystem();
}

t_list *escribir_desde_archivo(char *local_path, char file_type, int filesize) {

    FILE *archi = fopen(local_path, "r");
    if (archi == NULL) {
        log_info(logi, "Error abriendo archivo");
        return NULL;
    }

    int leido = 0;
    char *linea = NULL;
    char *buffer;
    ssize_t getln;
    size_t len_line, len;
    t_list *ba = list_create();
    //int alter = 0;

    if (file_type == 'B' || file_type == 'b') {

        int blocks, block_count, ultimoBloque;
        buffer = malloc(Mib);
        blocks = (filesize % Mib != 0) ? (filesize / Mib + 1) : (filesize / Mib);
        ultimoBloque = (filesize % Mib != 0 || filesize < Mib) ? (filesize - (filesize / Mib) * Mib) : Mib;


        for (block_count = 0; block_count < blocks - 1; block_count++) {

            fread(buffer, Mib, 1, archi);
            setBlock(buffer, Mib, ba);
            // alter = (alter + 1) % 2;


        }
        fread(buffer, ultimoBloque, 1, archi);
        setBlock(buffer, ultimoBloque, ba);
        free(buffer);


    } else {

        buffer = strdup("");
        getln = getline(&linea, &len, archi);
        while (getln != -1) {

            len_line = strlen(linea);
            if (leido + len_line > Mib) {
                // guardo bloque igual o menor a Mib
                setBlock(buffer, leido, ba);

                free(buffer);
                buffer = strdup("");
                leido = 0;
                // alter = (alter + 1) % 2;
            }
            string_append(&buffer, linea);
            leido += len_line;
            len = 0;
            free(linea);
            linea = NULL;
            getln = getline(&linea, &len, archi);
        }
        //guardo bloque final/único si filsize < Mib
        setBlock(buffer, leido, ba);
        free(buffer);
        if (linea) free(linea);
    }

    actualizar_tabla_nodos();

    fclose(archi);
    return ba;
}


aux_nodo *getFakeNodoMinusLoader(aux_nodo *NodoExcluir, t_list *fnodos) {
    int maxLibreEspacio = 0;
    aux_nodo *nodoMax = NULL;
    aux_nodo *nodoFetch;
    int i;
    for (i = 0; i < fnodos->elements_count; i++) {
        nodoFetch = list_get(fnodos, i);
        if (NodoExcluir != NULL && NodoExcluir->soket == nodoFetch->soket) {
            continue;
        }
        if (nodoFetch->espacio_libre > maxLibreEspacio) {
            maxLibreEspacio = nodoFetch->espacio_libre;
            nodoMax = nodoFetch;
        }
    }
    return nodoMax;
}

t_list *get_copia_nodos_activos() {

    t_list *aux = list_create();
    NODO *nodi = NULL;
    int i;
    for (i = 0; i < list_size(nodos); i++) {
        nodi = list_get(nodos, i);
        if (nodi->estado == disponible) {
            aux_nodo *aux_n = malloc(sizeof(aux_nodo));
            aux_n->espacio_libre = nodi->espacio_libre;
            aux_n->soket = nodi->soket;
            list_add(aux, aux_n);
        }
    }
    return aux;
}

bool hay_lugar_para_archivo(int filesize) {

    //todo: en realidad esa no es la cantidad real de bloques para un archivo de texto
    int blocks = (filesize % Mib != 0) ? (filesize / Mib + 1) : (filesize / Mib);
    t_list *lista = get_copia_nodos_activos();
    int cantCopy;
    aux_nodo *nodoSend;
    int b;
    void _liberar_aux_nodo(aux_nodo *self) {
        free(self);
    }
    for (b = 0; b < blocks; b++) {
        cantCopy = 0;
        nodoSend = NULL;
        while (cantCopy < 2) {
            nodoSend = getFakeNodoMinusLoader(nodoSend, lista);
            if (nodoSend == NULL) {
                list_destroy_and_destroy_elements(lista, (void *) _liberar_aux_nodo);
                return false;
            }
            cantCopy++;
            nodoSend->espacio_libre -= Mib;
        }
    }
    list_destroy_and_destroy_elements(lista, (void *) _liberar_aux_nodo);
    return true;
}

int get_file_size(char *path) {

    struct stat info;
    int ret = stat(path, &info);
    if (ret == -1) {
        return ret;
    }
    return info.st_size;
}


/* todo: FUNCIONES LEER ARCHIVO */

void *leer_bloque(bloqueArchivo *bq, int copia) {

    int bloque, ctrl = 0;
    header head;
    message *mensaje;
    void *buff = malloc(sizeof(int));
    void *buffinal = malloc((size_t) bq->bytesEnBloque + 1);
    memset(buffinal, '\0', (size_t) bq->bytesEnBloque + 1);
    NODO *nod = NULL;

    head.codigo = 1;
    head.letra = 'F';
    head.sizeData = sizeof(int);


    if (copia == 1) {

        nod = get_NODO(bq->nodo1);
        bloque = bq->bloquenodo1;

        if (nod == NULL) {
            nod = get_NODO(bq->nodo0);
            bloque = bq->bloquenodo0;
        }

        if (nod->estado == no_disponible) {
            bloque = bq->bloquenodo0;
            nod = get_NODO(bq->nodo0);
        }

    } else {
        nod = get_NODO(bq->nodo0);
        bloque = bq->bloquenodo0;

        if (nod == NULL) {
            nod = get_NODO(bq->nodo1);
            bloque = bq->bloquenodo1;
        }

        if (nod->estado == no_disponible) {
            bloque = bq->bloquenodo1;
            nod = get_NODO(bq->nodo1);
        }
    }

    if (nod == NULL) {
        free(buff);
        free(buffinal);
        return NULL;
    }

    memcpy(buff, &bloque, sizeof(int));

    liberarSocket(nod->soket);

    mensaje = createMessage(&head, buff);

    enviar_message(nod->soket, mensaje, logi, &ctrl);

    void *databloque = getMessageIntr(nod->soket, &head, &ctrl);


    memcpy(buffinal, databloque, (size_t) bq->bytesEnBloque);

    incorporarSocket(nod->soket);

    free(mensaje->buffer);
    free(mensaje);

    free(buff);
    free(databloque);

    return buffinal;

}

int crear_archivo_temporal(t_archivo *archivo, char *nombre_temporal) {

    int bloques = archivo->cantbloques;
    int alternar = 0, i;
    int retornar = -1;
    char *buff;
    bloqueArchivo *bq;

    if (archivo->estado == no_disponible) return -1;

    FILE *tmp = fopen(nombre_temporal, "w");
    buff = pedirFile(archivo->bloques, (size_t) archivo->tamanio); // todo: testar esto
//         ya no deberia hacer falta entrar al ciclo for()
    // pthread_mutex_lock(&mutex_socket);

    //  buff = pedirFile(archivo->bloques);
    /*
    for (i = 0; i < bloques; i++) {

        bq = list_get(archivo->bloques, i);

        buff = leer_bloque(bq, alternar);
        fwrite(buff, bq->bytesEnBloque, 1, tmp);
        fflush(tmp);
        free(buff);
        alternar = (alternar == 0) ? 1 : 0;
    }*/
    //pthread_mutex_unlock(&mutex_socket);

    if (buff != NULL) {
        fwrite(buff, (size_t) archivo->tamanio, 1, tmp);
        retornar = 0;
        free(buff);
    }
    fclose(tmp);
    return retornar;
}

// todo: se va a pelear con el select() -> sacar los FS pertinentes
// todo: los FD pertinentes estarian en la variable nodC[i].fd, tal vez no todos se necesitarian siempre...
char *pedirFile(t_list *bloques, size_t size_archive) { // este t_list contiene bloqueArchivo*

    int i, j, ctrl, lengthNodo, lengthBloques;
    char *data, *file_data;
    bool restantes = true;
    header head;

    lengthNodo = nodos->elements_count;
    lengthBloques = bloques->elements_count;

    struct _nodoCola nodC[lengthNodo];
    if (inicializarNodoCola(lengthNodo, &nodC, bloques) == -1) {
        log_error(logi, "No se pudo preparar los pedidos de bloques para un archivo");
        return NULL;
    }

    // tratamos de mallocar los bytes del archivo. Podria fallar si es un archivo super enorme mal...
    if ((file_data = malloc(size_archive)) == NULL) {
        log_error(logi, "No se pudieron allocar %d megabytes para el archivo pedido", lengthBloques);
        return NULL;
    }

    log_info(logi, "Enviando Peticiones");
    int posicionPeticion = 0;
    bloqPedido *bloquePedidoFetch;
    for (i = 0; i < lengthNodo; i++) {
        if (nodC[i].hay_pedidos) {
            for (j = 0; j < nodC[i].colaPedidos->elements_count; j++) {
                bloquePedidoFetch = list_get(nodC[i].colaPedidos, j);
                enviarPeticion(nodC[i].fd, bloquePedidoFetch->numberBlock);
            }
        }
    }

    log_info(logi, "Recolectando Bloque Enviados");
    for (i = 0; restantes; i = (i + 1) % lengthNodo) {
        if (!nodC[i].hay_pedidos) continue;
        log_info(logi, "buscando en socket %d position %d", nodC[i].fd, i);
        data = getMessageIntrNB(nodC[i].fd, &head, &ctrl);
        if (ctrl == -2) continue;
        if (ctrl == 0 || ctrl == -1) {
            log_info(logi, "Se desconecto Nodo en %d", nodC[i].fd); // todo: hacerle close(nodC[i].fd) ahora? o despues?
            nodC[i].hay_pedidos = false;
            nodC[i].connected = 0;
            disconnectedNodo(nodC[i].fd);
            incorporarSocket(nodC[i].fd);
            log_info(logi, "Se intentara buscar bloque en el Nodo alternativo...");
            if (delegarPedidos(lengthNodo, &nodC, i) == -1) {
                log_error(logi, "No es posible delegar el pedido a otro Nodo");
                break;
            }
        } else {
            bloquePedidoFetch = list_get(nodC[i].colaPedidos, posicionPeticion);
            memcpy(file_data + bloquePedidoFetch->pointerBuffer, data, bloquePedidoFetch->sizeBuffer);
            list_remove(nodC[i].colaPedidos, posicionPeticion);
            if (nodC[i].colaPedidos->elements_count ==
                0) { // todo: creo que esto funciona bien, pero revisar si se puede
                nodC[i].hay_pedidos = false;
                incorporarSocket(nodC[i].fd);
                restantes = restanPedidos(lengthNodo, &nodC);
                continue;
            }
            //bloquePedidoFetch = list_get(nodC[i].colaPedidos, posicionPeticion);
            //enviarPeticion(nodC[i].fd,bloquePedidoFetch->numberBlock);
        }
        free(data);
    }

    //incorporamos todos los fileDescription por las dudas
    for (i = 0; i < lengthNodo; i++) {
        incorporarSocket(nodC[i].fd);
    }

    // no se agotaron todos los Nodos => fue ejecucion erronea
    if (restantes) {
        free(file_data);
        return NULL;
    }

    return file_data;
}

void enviarPeticion(int socket, int bloque) {

    int ctrl;
    message *msj = NULL;
    header head = {.codigo = 1, .letra = 'F', .sizeData = sizeof(int)};
    void *bufferMjs = malloc(sizeof(int));

    memcpy(bufferMjs, &bloque, sizeof(int));
    msj = createMessage(&head, bufferMjs);
    enviar_message(socket, msj, logi, &ctrl);
    liberador(2, msj, bufferMjs);
}

int inicializarNodoCola(int lengthNodo, struct _nodoCola (*nodC)[lengthNodo], t_list *bloques) {

    int lengthBloque, i;
    bloqueArchivo *bloqueFetch;
    NODO *nodoFetch;

    lengthBloque = list_size(bloques);
    for (i = 0; i < lengthNodo; ++i) {
        nodoFetch = list_get(nodos, i);
        (*nodC)[i].fd = nodoFetch->soket;
        (*nodC)[i].connected = 1;
        (*nodC)[i].hay_pedidos = false;
        (*nodC)[i].colaPedidos = list_create();
        liberarSocket((*nodC)[i].fd);
    }
    int positionPointer = 0;

    for (i = 0; i < lengthBloque; ++i) {
        bloqueFetch = list_get(bloques, i);
        if (!encolarSobreNodos(lengthNodo, nodC, bloqueFetch, i, positionPointer)) {
            log_error(logi, "No se encontraron nodos disponibles para el bloque %d", i);
            return -1;
        }
        positionPointer += bloqueFetch->bytesEnBloque;
    }
    return 0;
}

int countColaNodo(char *nameNodo, int lengthNodo, struct _nodoCola (*nodC)[lengthNodo]) {
    int i;
    NODO *nodoFetch;
    for (i = 0; i < lengthNodo; ++i) {
        nodoFetch = get_NODO(nameNodo);
        if (nodoFetch->soket == (*nodC)[i].fd) {
            return (*nodC)[i].colaPedidos->elements_count;
        }
    }
    return -1;
}

int encolarSobreNodos(int lengthNodo, struct _nodoCola (*nodC)[lengthNodo], bloqueArchivo *bloque, int pos,
                      int positionPointer) {

    NODO *nodoFetchCopy0 = get_NODO(bloque->nodo0);
    NODO *nodoFetchCopy1 = get_NODO(bloque->nodo1);
    int i, numFD;
    int nodoCopy = 0;

    if (nodoFetchCopy0 == NULL || nodoFetchCopy1 == NULL) {
        log_error(logi, "Uno o Dos nodos no fueron encontrados");
        return 0;
    }

    if (nodoFetchCopy0->estado == no_disponible && nodoFetchCopy1->estado == no_disponible) {
        log_error(logi, "Los Nodos que contiene la copia no estan disponibles");
        return 0;
    }

    if (nodoFetchCopy0->estado == disponible && nodoFetchCopy1->estado == disponible) {
        if (countColaNodo(bloque->nodo0, lengthNodo, nodC) < countColaNodo(bloque->nodo1, lengthNodo, nodC)) {
            nodoCopy = 0;
        } else {
            nodoCopy = 1;
        }
    } else {
        if (nodoFetchCopy0->estado == disponible) {
            nodoCopy = 0;
        } else {
            nodoCopy = 1;
        }
    }


    bloqPedido *nuevoPedido = malloc(sizeof(bloqPedido));
    nuevoPedido->numberBlock = nodoCopy ? bloque->bloquenodo1 : bloque->bloquenodo0;
    nuevoPedido->orden = pos;
    nuevoPedido->sizeBuffer = (size_t) bloque->bytesEnBloque;
    nuevoPedido->pointerBuffer = positionPointer;
    nuevoPedido->bq = bloque;
    nuevoPedido->numCopy = nodoCopy;

    numFD = nodoCopy ? nodoFetchCopy1->soket : nodoFetchCopy0->soket;

    for (i = 0; i < lengthNodo; ++i) {
        if ((*nodC)[i].fd == numFD) {
            list_add((*nodC)[i].colaPedidos, nuevoPedido);
            (*nodC)[i].hay_pedidos = true;
        }
    }
    return 1;
}

int delegarPedidos(int lengthNodo, struct _nodoCola (*nodC)[lengthNodo], int node) {
    int j;
    int delego = false;
    char *nameNodo;
    NODO *nodoAdelegar;
    bloqPedido *fetchPedido;
    while ((*nodC)[node].colaPedidos->elements_count != 0) {
        fetchPedido = list_get((*nodC)[node].colaPedidos, 0);
        fetchPedido->numCopy = fetchPedido->numCopy ? 0 : 1;
        nameNodo = fetchPedido->numCopy ? fetchPedido->bq->nodo1 : fetchPedido->bq->nodo0;
        nodoAdelegar = get_NODO(nameNodo);
        for (j = 0; j < lengthNodo; ++j) {
            if ((*nodC)[j].connected && nodoAdelegar->soket == (*nodC)[j].fd) {
                list_add((*nodC)[j].colaPedidos, fetchPedido);
                delego = true;
                list_remove((*nodC)[node].colaPedidos, 0);
                enviarPeticion(nodoAdelegar->soket, fetchPedido->numberBlock);
                break;
            }
        }
        if (!delego) {
            return -1;
        }
    }

    return 0;
}


bool restanPedidos(int lengthNodo, struct _nodoCola (*nodC)[lengthNodo]) {

    int i;
    for (i = 0; i < lengthNodo; ++i)
        if ((*nodC)[i].hay_pedidos)
            return true;
    return false;
}