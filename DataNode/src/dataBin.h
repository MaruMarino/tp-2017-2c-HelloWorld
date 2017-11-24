//
// Created by elmigue on 08/09/17.
//

#ifndef TP_2017_2C_HELLOWORLD_OPENARCHIVEBIN_H
#define TP_2017_2C_HELLOWORLD_OPENARCHIVEBIN_H
#define megaByte 1048576

char *openDateBin(char *path, size_t *leng, size_t tamanio);

void insertBloque(void **dataBinMap, void **buffer, unsigned int numberBloque, size_t sizeBuffer);

void *getBloque(void **dataBinMap, unsigned int numberBloque);

#endif //TP_2017_2C_HELLOWORLD_OPENARCHIVEBIN_H
