/*
 * log.h
 *
 *  Created on: 29/11/2016
 *      Author: utnso
 */

#ifndef SRC_LOG_H_
#define SRC_LOG_H_

#include <commons/log.h>

//Funcion para crear archivo de log
//Toma como parametros la cabecera del archivo, booleano de print y el path del archivo
t_log *crear_archivo_log(char *nombre_cabecera, int imprimir, char *file);

//Funcion que escribe en archivo de log
void escribir_log(t_log *log, char *mensaje);

//Funcion que escribe un ERROR en archivo de log
void escribir_error_log(t_log *log, char *mensaje);

//Funcion que escribe un ERROR con numerico en archivo de log
void escribir_log_error_con_numero(t_log *log, char *mensaje, int un_numero);

//Recibe un texto y un numerico, muy util!
void escribir_log_con_numero(t_log *log, char *mensaje, int un_numero);

//Recibe y genera log recibiendo dos textos
void escribir_log_compuesto(t_log *log, char *mensaje, char *otro_mensaje);

//Recibe y genera log de ERROR recibiendo dos textos
void escribir_log_error_compuesto(t_log *log, char *mensaje, char *otro_mensaje);

//Elimina el archivo de log
void liberar_log(t_log *log);

#endif /* SRC_LOG_H_ */
