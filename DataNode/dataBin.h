//
// Created by elmigue on 08/09/17.
//

#ifndef TP_2017_2C_HELLOWORLD_OPENARCHIVEBIN_H
#define TP_2017_2C_HELLOWORLD_OPENARCHIVEBIN_H
char * openDateBin(char **path,size_t *leng,size_t tamanio);
void insertBloque(char* dataBinMap,char * buffer,unsigned int numberBloque);
char * getBloque(char * dataBinMap,unsigned int numberBloque);
#endif //TP_2017_2C_HELLOWORLD_OPENARCHIVEBIN_H
