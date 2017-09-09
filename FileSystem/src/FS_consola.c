/*
 * FS_consola.c
 *
 *  Created on: 9/9/2017
 *      Author: utnso
 */

#include "FS_consola.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#define sin "\x1B[0m"
#define rojo "\x1B[31m"
#define verde  "\x1B[32m"
#define amarillo  "\x1B[33m"
#define azul  "\x1B[34m"
#define magenta  "\x1B[35m"
#define cyan  "\x1B[36m"
#define blanco  "\x1B[37m"


void iniciar_consola_FS(){

	sleep(2);
	system("clear");

	printf("\n  %s＼(*^0^*)／  Bienvenido al Proceso YAMA-File System   ＼(*^0^*)／ %s \n",amarillo,sin);

	char * linea;
	printf("\n");
	  while(1) {

	    linea = readline(">");
	    if(linea) add_history(linea);

	    if(!strncmp(linea, "exit", 4)) {
	       free(linea);
	       break;
	    }

	    if(!strncmp(linea,"^C",2)){
	    	printf("ctrl-c, no hago nada");
	    }
	    printf("%s\n", linea);
	    free(linea);
	  }


}
