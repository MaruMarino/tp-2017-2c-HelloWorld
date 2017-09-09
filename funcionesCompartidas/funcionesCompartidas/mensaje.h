/*
 * mensaje.h
 *
 *  Created on: 23/4/2017
 *      Author: utnso
 */

#ifndef FUNCIONES_MENSAJE_H_
#define FUNCIONES_MENSAJE_H_

//Arma el texto de envio para mensajes
//Identificador: M22	Mensaje: /bin/prueba.json	->	M220000000015/bin/prueba.csv
char *armar_mensaje(char *identificador, char *mensaje);

//Devuelve el header del mensaje
//	Y110000000015/bin/prueba.csv	->	Y
char *get_header(char *mensaje);

//Devuelve el codigo del mensaje
//	Y110000000015/bin/prueba.csv	->	11
int get_codigo(char *mensaje);

//Obtiene el mensaje
//	Y110000000015/bin/prueba.csv	->	/bin/prueba.csv
char *get_mensaje(char *mensaje);

//Compara un header contra el header un mensaje
int comparar_header(char *identificador, char *header);

#endif /* FUNCIONES_MENSAJE_H_ */
