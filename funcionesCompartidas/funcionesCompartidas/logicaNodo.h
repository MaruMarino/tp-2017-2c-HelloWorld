#ifndef FUNCIONESCOMPARTIDAS_LOGICANODO_H_
#define FUNCIONESCOMPARTIDAS_LOGICANODO_H_

#define megaByte 0x100000

char *openDataBin(char *path, size_t *dsize, off_t tamanio);

void *getBloque(void **dataBinMap, unsigned int numberBloque);

void insertBloque(void **dataBinMap, void **buffer, unsigned int numberBloque, size_t sizeBuffer);

char *getDataBloque(char *datamap, size_t blk);

#endif /* FUNCIONESCOMPARTIDAS_LOGICANODO_H_ */
