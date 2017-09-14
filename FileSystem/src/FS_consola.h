/*
 * consola_FS.h
 *
 *  Created on: 9/9/2017
 *      Author: utnso
 */

#ifndef FS_CONSOLA_H_
#define FS_CONSOLA_H_

#include "estructurasfs.h"

void iniciar_consola_FS(void);
void inicializar_completado(void);
int ejecutar_linea(char *linea);
char *command_generator ();
char **command_name_completion(const char *text,int start,int end);
comando *find_command (char *clinea);


#endif /* FS_CONSOLA_H_ */
