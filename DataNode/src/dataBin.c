//
// Created by elmigue on 08/09/17.
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "dataBin.h"
#include <sys/mman.h>
#include <string.h>

char *openDateBin(char **path, size_t *leng, size_t tamanio) {
    int file;
    struct stat *infoFile = malloc(sizeof(struct stat));

    if (stat(*path, infoFile) == -1) {
        file = open(*path, O_RDWR | O_CREAT | O_TRUNC, 0666);
        ftruncate(file, tamanio);
        stat(*path, infoFile);
    } else {
        file = open(*path, O_RDWR, 0666);
    }

    if (file == -1) {
        return NULL;
    }
    *leng = (size_t) infoFile->st_size;
    char *fileMap = mmap(0, (size_t) infoFile->st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);
    if (fileMap == MAP_FAILED) {
        return NULL;
    }
    close(file);
    free(infoFile);
    return fileMap;
}

void *getBloque(void **dataBinMap, unsigned int numberBloque) {
    char *buffer = malloc(megaByte);
    memcpy(buffer, *dataBinMap + (megaByte * numberBloque), (size_t) megaByte);
    return buffer;
}

void insertBloque(void **dataBinMap, void **buffer, unsigned int numberBloque,size_t sizeBuffer) {
    memcpy((*dataBinMap + (megaByte * numberBloque)), *buffer,sizeBuffer);
}