/*
 * FS_administracion.c
 *
 *  Created on: 19/9/2017
 *      Author: utnso
 */

#include "FS_administracion.h"

#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <sys/socket.h>
#include <funcionesCompartidas/estructuras.h>
#include "estructurasfs.h"

#define Mib 1048576

extern yamafs_config *configuracion;
extern t_log *logi;
extern t_directory directorios[100];
extern t_list *nodos;
extern t_list *archivos;

// Funciones Auxiliares
static char *armar_string_nombres_nodos();

static char *completar_path_metadata(char *);

static char *nombres_subdirectorios(char *);

static char *nombres_archivos(char *);

static void liberar_char_array(char **);
//static int indice_padre_archivo(char *);


// Funciones de recuperacion de estructuras administrativas de un estado anterior //

int recuperar_estructuras_administrativas(void) {

    int status;
    // recuperar arbol de directorios
    status = recuperar_arbol_directorios();
    if (status != 1) {
        log_error(logi, "Error en la recuperacion del arbol de directorios, bai");
        return status;
    }
    // recuperar NODOS
    status = recuperar_nodos();
    if (status != 1) {
        log_error(logi, "Error en la recuperacion de nodos, bai");
        return status;
    }
    // Recuperar bitmaps nodos
    int cantidad_nodos = list_size(nodos);
    int i = 0;
    while (i < cantidad_nodos) {

        NODO *unodo = list_get(nodos, i);
        status = recuperar_bitmap_nodo(unodo);
        if (status != 1) {
            log_error(logi, "Error en la recuperacion de bitmaps, bai");
            return status;
        }
        i++;
    }

    status = recuperar_metadata_archivos();
    if (status != 1) {
        log_error(logi, "Error en la recuperacion de metadata de archivos, bai");
        return status;
    }

    return 1;
}

int recuperar_arbol_directorios(void) {

    char *path_armado;
    // recuperar arbol de directorios
    path_armado = completar_path_metadata("directorios.dat");
    FILE *filedir = fopen(path_armado, "r");

    if (filedir == NULL) {
        log_error(logi, "Error abriendo archivo 'directorios.dat'");
        free(path_armado);
        return -1;
    } else {
        fread(directorios, sizeof(t_directory), 100, filedir);
        fclose(filedir);
    }

    free(path_armado);
    return 1;
}

int recuperar_nodos(void) {

    char *path_armado;
    path_armado = completar_path_metadata("nodos.bin");

    struct stat data;
    int res = stat(path_armado, &data);
    if (res != 0) {
        log_error(logi, "Error abriendo Nodos.bin");
        free(path_armado);
        return -1;
    }

    t_config *config_nodos = config_create(path_armado);

    configuracion->espacio_total = config_get_int_value(config_nodos, "TAMANIO");
    configuracion->espacio_libre = config_get_int_value(config_nodos, "LIBRE");
    char **nombres_nodos = config_get_array_value(config_nodos, "NODOS");

    int i = 0;
    char *key_aux;

    while (nombres_nodos[i] != NULL) {

        NODO *nodo_recuperado = malloc(sizeof(NODO));

        nodo_recuperado->nombre = malloc(strlen(nombres_nodos[i]));
        strcpy(nodo_recuperado->nombre, nombres_nodos[i]);

        key_aux = string_from_format("%sTotal", nombres_nodos[i]);
        nodo_recuperado->espacio_total = config_get_int_value(config_nodos, key_aux);
        free(key_aux);

        key_aux = string_from_format("%sLibre", nombres_nodos[i]);
        nodo_recuperado->espacio_libre = config_get_int_value(config_nodos, key_aux);
        free(key_aux);

        list_add(nodos, nodo_recuperado);
        i++;
    }

    free(path_armado);
    config_destroy(config_nodos);
    liberar_char_array(nombres_nodos);

    return 1;
}

int recuperar_bitmap_nodo(NODO *unodo) {

    struct stat mystat;

    char *aux_nombre_archivo = string_from_format("bitmaps/%s.dat", unodo->nombre);
    char *path_armado = completar_path_metadata(aux_nombre_archivo);

    int fdbitmap = open(path_armado, O_RDWR);
    if (fdbitmap <= 0) {
        log_error(logi, "Error abriendo %s \n", aux_nombre_archivo);
        free(path_armado);
        return -1;
    }
    fstat(fdbitmap, &mystat);
    char *bitarray = mmap(0, mystat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fdbitmap, 0);
    if (bitarray == MAP_FAILED) {
        log_error(logi, "Error en mmap :%s\n", strerror(errno));
        close(fdbitmap);
        free(path_armado);
        return -1;

    }
    unodo->bitmapNodo = bitarray_create_with_mode(bitarray, mystat.st_size, LSB_FIRST);
    free(aux_nombre_archivo);
    free(path_armado);
    close(fdbitmap);
    return 1;
}

int recuperar_metadata_archivos(void) {

    char *nombres_sub = nombres_subdirectorios("archivos");
    if (nombres_sub == NULL) {
        return -1;
    } else if (!strncmp(nombres_sub, "NADA", 4)) {
        return 1;
    }
    char **aux_split = string_split(nombres_sub, "-");

    int i = 0;
    char *subdirectorio;
    char *fullpath;
    char *archivoss;

    while (aux_split[i] != NULL) {

        subdirectorio = string_from_format("archivos/%s", aux_split[i]);
        fullpath = completar_path_metadata(subdirectorio);
        archivoss = nombres_archivos(fullpath);

        if (archivoss == NULL) {
            free(subdirectorio);
            free(fullpath);
            liberar_char_array(aux_split);
            return -1;
        } else if (strncmp(archivoss, "NADA", 4) != 0) {
            char **aux_split_archs = string_split(archivoss, "-");
            int c = 0;
            char *fullpath_archivo;
            while (aux_split_archs[c] != NULL) {
                fullpath_archivo = string_from_format("%s/%s", fullpath, aux_split_archs[c]);
                recuperar_metadata_un_arhcivo(fullpath_archivo);
                free(fullpath_archivo);
                c++;
            }

            liberar_char_array(aux_split_archs);
            free(archivoss);
        }

        free(fullpath);
        free(subdirectorio);

        i++;
    }

    liberar_char_array(aux_split);
    free(nombres_sub);

    return 1;
}

int recuperar_metadata_un_arhcivo(char *fullpath) {

    t_archivo *archi = malloc(sizeof(t_archivo));
    t_config *info = config_create(fullpath);

    archi->tamanio = config_get_int_value(info, "TAMANIO");
    archi->tipo = strdup(config_get_string_value(info, "TIPO"));
    //archi->index_padre = todo:obtener indice del padre
    archi->cantbloques = ((config_keys_amount(info) - 2) / 3);
    archi->estado = no_disponible;

    archi->bloques = list_create();
    int i = 0;

    while (i < archi->cantbloques) {

        bloqueArchivo *nuevobloque = malloc(sizeof(bloqueArchivo));
        char *key_bloque_copia0 = string_from_format("BLOQUE%dCOPIA0", i);
        char *key_bloque_copia1 = string_from_format("BLOQUE%dCOPIA1", i);
        char *key_bloque_bytes = string_from_format("BLOQUE%dBYTES", i);

        char **copia0 = config_get_array_value(info, key_bloque_copia0);
        nuevobloque->nodo0 = strdup(copia0[0]);
        nuevobloque->bloquenodo0 = atoi(copia0[1]);

        char **copia1 = config_get_array_value(info, key_bloque_copia1);
        nuevobloque->nodo0 = strdup(copia1[0]);
        nuevobloque->bloquenodo1 = atoi(copia1[1]);

        nuevobloque->bytesEnBloque = config_get_int_value(info, key_bloque_bytes);

        list_add(archi->bloques, nuevobloque);

        liberar_char_array(copia0);
        liberar_char_array(copia1);
        free(key_bloque_copia0);
        free(key_bloque_copia1);
        free(key_bloque_bytes);

        i++;
    }

    list_add(archivos, archi);
    config_destroy(info);
    return 1;
}
// Funciones de creacion de estructuras administrativas en un inicio limpio //

int iniciar_arbol_directorios(void) {

    int i;
    for (i = 0; i < 100; i++) {
        directorios[i].index = i;
        memset(directorios[i].nombre, 0, 255);
        directorios[i].padre = -1;
    }
    char *path_armado = completar_path_metadata("directorios.dat");

    FILE *filedir = fopen(path_armado, "w+");
    fwrite(directorios, sizeof(t_directory), 100, filedir);
    fclose(filedir);

    free(path_armado);
    return 1;

}

int iniciar_nodos(void) {

    char *path_armado = completar_path_metadata("nodos.bin");
    t_config *tabla_nodos = malloc(sizeof(t_config));
    tabla_nodos->path = path_armado;

    t_dictionary *elementos = dictionary_create();
    char *espacio_total = string_itoa(configuracion->espacio_total);
    char *espacio_libre = string_itoa(configuracion->espacio_libre);

    dictionary_put(elementos, "TAMANIO", espacio_total);
    dictionary_put(elementos, "LIBRE", espacio_libre);


    char *nomnodos = armar_string_nombres_nodos();
    dictionary_put(elementos, "NODOS", nomnodos);

    void _agregar_info_nodo(NODO *self) {

        char *total = string_from_format("%sTotal", self->nombre);
        char *libre = string_from_format("%sLibre", self->nombre);
        espacio_total = string_itoa(self->espacio_total);
        espacio_libre = string_itoa(self->espacio_libre);

        dictionary_put(elementos, total, espacio_total);
        dictionary_put(elementos, libre, espacio_libre);

        free(total);
        free(libre);
        //free(espacio_total);
        //free(espacio_libre);
    }

    list_iterate(nodos, (void *) _agregar_info_nodo);
    tabla_nodos->properties = elementos;

    config_save(tabla_nodos);

    dictionary_destroy(tabla_nodos->properties);
    free(espacio_total);
    free(espacio_libre);
    free(nomnodos);
    free(tabla_nodos->path);
    free(tabla_nodos);

    return 1;
}

int iniciar_bitmaps_nodos(void) {

    void _create_bitmap(NODO *self) {

        int cantBloques = self->espacio_total / Mib;
        char *bitarray = malloc((size_t) cantBloques);
        memset(bitarray, 0, (size_t) cantBloques);
        self->bitmapNodo = bitarray_create_with_mode(bitarray, (size_t) cantBloques, LSB_FIRST);
    }

    list_iterate(nodos, (void *) _create_bitmap);

    void _save_bitmap_infile(NODO *self) {

        char *aux_nombre_archivo = string_from_format("bitmaps/%s.dat", self->nombre);
        char *path_armado = completar_path_metadata(aux_nombre_archivo);

        FILE *filedir = fopen(path_armado, "w+");
        fwrite(self->bitmapNodo->bitarray, self->bitmapNodo->size, 1, filedir);
        fclose(filedir);

        free(aux_nombre_archivo);
        free(path_armado);
    }

    list_iterate(nodos, (void *) _save_bitmap_infile);

    void _mmap_bitmap(NODO *self) {

        bitarray_destroy(self->bitmapNodo);
        recuperar_bitmap_nodo(self);

    }

    list_iterate(nodos, (void *) _mmap_bitmap);


    return 1;
}

void crear_subdirectorios(void) {

    char *path_armado = completar_path_metadata("archivos");
    mkdir(path_armado, 0775);
    free(path_armado);

    path_armado = completar_path_metadata("bitmaps");
    mkdir(path_armado, 0775);
    free(path_armado);

}
// Funciones para agregar/sacar/modificar elementos de las diferentes estructuras ya creadas //

// Funciones Auxiliares

static char *completar_path_metadata(char *archivo) {

    char *pmetadata = strdup(configuracion->dir_estructuras);
    char *pmdirectorios = strdup("");

    string_append(&pmdirectorios, pmetadata);
    if (string_ends_with(pmdirectorios, "/")) {
        string_append(&pmdirectorios, archivo);
    } else {
        string_append(&pmdirectorios, "/");
        string_append(&pmdirectorios, archivo);
    }

    free(pmetadata);
    return pmdirectorios;
}

static char *armar_string_nombres_nodos() {

    char *aux_string = strdup("");
    int i;
    int cantnodos = list_size(nodos);

    for (i = 0; i < (cantnodos - 1); i++) {

        NODO *aux_nodo = list_get(nodos, i);
        string_append(&aux_string, aux_nodo->nombre);
        string_append(&aux_string, ",");
    }
    if (i == cantnodos - 1) {
        NODO *aux_nodo = list_get(nodos, i);
        string_append(&aux_string, aux_nodo->nombre);
    }


    char *string_final = string_from_format("[%s]", aux_string);

    free(aux_string);

    return string_final;

}

static char *nombres_subdirectorios(char *donde) {

    DIR *directorio;
    struct dirent *entry;
    int cantidad_subdirectorios;
    char *path_armado = completar_path_metadata(donde);
    char *archivos = strdup("");
    directorio = opendir(path_armado);

    if (directorio == NULL) {
        perror("Error abriendo directorio de archivos");
        free(path_armado);
        free(archivos);
        return NULL;
    }

    cantidad_subdirectorios = 0;

    for (entry = readdir(directorio); entry != NULL; entry = readdir(directorio)) {

        if (((strncmp(entry->d_name, ".", 1)) || (strncmp(entry->d_name, "..", 1))) && entry->d_type == DT_DIR) {

            cantidad_subdirectorios++;

            string_append(&archivos, "-");
            string_append(&archivos, entry->d_name);

        }
    }
    if (cantidad_subdirectorios == 0) {
        free(archivos);
        return "NADA";
    }
    char *subnombres = string_substring_from(archivos, 1);
    free(path_armado);
    free(archivos);
    closedir(directorio);

    return subnombres;
}

static char *nombres_archivos(char *donde) {

    DIR *directorio;
    struct dirent *entry;
    int cantidad_archivos;
    char *path_armado = strdup(donde);
    char *archivos = strdup("");
    directorio = opendir(path_armado);

    if (directorio == NULL) {
        perror("Error abriendo directorio de archivos");
        free(path_armado);
        free(archivos);
        return NULL;
    }

    cantidad_archivos = 0;

    for (entry = readdir(directorio); entry != NULL; entry = readdir(directorio)) {

        if (((strncmp(entry->d_name, ".", 1)) || (strncmp(entry->d_name, "..", 1))) && entry->d_type == DT_REG) {

            cantidad_archivos++;

            string_append(&archivos, "-");
            string_append(&archivos, entry->d_name);

        }
    }
    if (cantidad_archivos == 0) {
        free(archivos);
        return "NADA";
    }
    char *subnombres = string_substring_from(archivos, 1);
    free(path_armado);
    free(archivos);
    closedir(directorio);

    return subnombres;

}

static void liberar_char_array(char **miarray) {

    int i = 0;
    while (miarray[i] != NULL) {
        free(miarray[i]);
        i++;
    }
    free(miarray);
}

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
        if (nodoFetch->espacio_libre > maxLibreEspacio) {
            *numBlock = getBlockFree(nodoFetch->bitmapNodo);
            if (!(NodoExcluir != NULL && NodoExcluir->soket == nodoFetch->soket) && *numBlock != -1) {
                maxLibreEspacio = nodoFetch->espacio_libre;
                nodoMax = nodoFetch;
            }
        }
    }
    return nodoMax;
}

int cantBlockfree(t_bitarray *listBit) {
    int i, cant = 0;
    for (i = 0; i < listBit->size; ++i) {
        if (!bitarray_test_bit(listBit, i)) {
            cant++;
        }
    }
    return cant;
}

void checkStateNodos() {
    NODO *nodo_fetch;
    puts("------------------------------------");
    for (int i = 0; i < nodos->elements_count; ++i) {
        nodo_fetch = list_get(nodos, i);
        printf("[%s]\n", nodo_fetch->nombre);
        printf("[%d]\n", nodo_fetch->soket);
        printf("bloque free %d\n", cantBlockfree(nodo_fetch->bitmapNodo));
        printf("Espacio Libre %d\n", nodo_fetch->espacio_libre);
    }
    puts("------------------------------------");
}

int setBlock(void *buffer, size_t size_buffer) {
    checkStateNodos();
    int cantCopy = 0;
    int control;
    size_t bufferWithBlockSize = (size_buffer + sizeof(int));
    void *bufferWithBlock = malloc(bufferWithBlockSize);
    NODO *nodoSend = NULL;
    int nodoSendBlock;
    header reqRes;
    int insertOk;
    void *bufferResponse;
    while (cantCopy < 2) {
        nodoSend = getNodoMinusLoader(nodoSend, &nodoSendBlock);
        if (nodoSend == NULL) return -1;
        reqRes.codigo = 2;
        reqRes.letra = 'F';
        reqRes.sizeData = bufferWithBlockSize;
        memcpy(bufferWithBlock, &nodoSendBlock, sizeof(int));
        memcpy((bufferWithBlock + sizeof(int)), buffer, size_buffer);
        message *request = createMessage(&reqRes, bufferWithBlock);
        if (send(nodoSend->soket, request->buffer, request->sizeBuffer, 0) < 0) {
            puts("error al enviar primer bloque");
            return -1;
        }
        /*
        bufferResponse = getMessage(nodoSend->soket,&reqRes,&control);
        if(bufferResponse == NULL){
            puts("error al recibir la respuesta");
            return -1;
        }

        if(reqRes.codigo != 6){
            puts("recivio otro mjs");
            return -1;
        }

        memcpy(&insertOk, bufferResponse,reqRes.sizeData);
        if(insertOk == -1){
            puts("no se inserto el bloque");
            return -1;
        }*/
        bitarray_set_bit(nodoSend->bitmapNodo, nodoSendBlock);
        nodoSend->espacio_libre -= Mib;
        cantCopy++;
    }

    /*checkStateNodos();
    size_t bufferWithBlockSize = (size_buffer + sizeof(int));
    void *bufferWithBlock = malloc(bufferWithBlockSize);

    int nodo_FistCopy_block;
    NODO *nodo_FistCopy = getNodoMinusLoader(NULL, &nodo_FistCopy_block);
    if (nodo_FistCopy == NULL) return -1;

    int nodo_SecondCopy_block;
    NODO *nodo_SecondCopy = getNodoMinusLoader(nodo_FistCopy, &nodo_SecondCopy_block);
    if (nodo_SecondCopy == NULL) return -1;

    header response;

    header request_head;
    request_head.codigo = 2;
    request_head.letra = 'F';
    request_head.sizeData = bufferWithBlockSize;
    memcpy(bufferWithBlock, &nodo_FistCopy_block, sizeof(int));
    memcpy((bufferWithBlock + sizeof(int)), buffer, size_buffer);
    message *request = createMessage(&request_head, bufferWithBlock);
    if (send(nodo_FistCopy->soket, request->buffer, request->sizeBuffer, 0) < 0) {
        printf("error al enviar primer bloque");
        return -1;
    }
    getMessage(nodo_FistCopy->soket)
    bitarray_set_bit(nodo_FistCopy->bitmapNodo, nodo_FistCopy_block);

    memcpy(bufferWithBlock, &nodo_SecondCopy_block, sizeof(int));
    if (send(nodo_SecondCopy->soket, request->buffer, request->sizeBuffer, 0) < 0) {
        printf("error al enviar segundo bloque");
        return -1;
    }
    bitarray_set_bit(nodo_SecondCopy->bitmapNodo, nodo_SecondCopy_block);

    checkStateNodos();*/
    checkStateNodos();
    return 0;
}
