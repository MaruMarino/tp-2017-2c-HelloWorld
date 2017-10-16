#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>

#include "logicaNodo.h"

#define blksize 0x100000 // 1 MiB

char *openDataBin(char *path, size_t *dsize, off_t tamanio) {

	int file;
	struct stat *infoFile = malloc(sizeof *infoFile);

	if (stat(path, infoFile) == -1) {
		file = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);
		ftruncate(file, tamanio);
		stat(path, infoFile);
	} else{
		if ((file = open(path, O_RDWR, 0666)) == -1)
			return NULL;
	}

	*dsize = (size_t) infoFile->st_size;
	char *fileMap = mmap(0, *dsize, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);
	if (fileMap == MAP_FAILED)
		return NULL;

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

char *getDataBloque(char *datamap, size_t blk){
	return datamap + blk * blksize;
}
