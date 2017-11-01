#ifndef ESTRUCTURAS_LOCALES_H_
#define ESTRUCTURAS_LOCALES_H_

enum{
	TRANSF   = 1,  // transformacion
	RED_L    = 2,  // reduccion local
	RED_G    = 3,  // reduccion global
	ALMAC    = 4,  // almacenamiento final
	OK       = 7,  // job worker ejecucion exitosa
	FALLO    = 8,  // job worker ejecucion fallo
	ALMAC_FS = 9,  // almacenar en yamafs
	FILE_REQ = 10, // pedido de linea
	FILE_SND = 11, // envio de linea
	APAR_EOF = 12, // fin de IO
	APAR_ERR = 13, // error de IO
	APAR_INV = 14, // lectura linea tamanio invalido
};

#endif
