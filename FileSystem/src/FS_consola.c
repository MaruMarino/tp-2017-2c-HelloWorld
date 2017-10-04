/*
 * FS_consola.c
 *
 *  Created on: 9/9/2017
 *      Author: utnso
 */

#include "FS_consola.h"

#include <commons/string.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "estructurasfs.h"
#include "FS_funciones.h"

#define sin "\x1B[0m"
#define rojo "\x1B[31m"
#define verde  "\x1B[32m"
#define amarillo  "\x1B[33m"
#define azul  "\x1B[34m"
#define magenta  "\x1B[35m"
#define cyan  "\x1B[36m"
#define blanco  "\x1B[37m"


comando commands[] = {
		{ "ayuda", fs_ayuda, "Mostrar menu de comandos", "ayuda"},
		{ "?", fs_payuda, "Ayuda sobre un comando en particular","? nombre_comando" },
		{ "format", fs_format, "Formatear Filesystem","format"},
		{ "rm", fs_rm, "Eliminar Archivo/Directorio/Nodo/Bloque","rm path_archivo |rm​ ​-d​ path_dir|rm ​-b ​path_archivo nro_bloque nro_copia"},
		{ "rename", fs_rename, "Renombrar un Archivo o Directorio","rename ​ path_original nombre_final" },
		{ "ls", fs_ls, "Listar archivos de un Directorio","ls ​ path_directorio" },
		{ "mv", fs_mv, "Mover un Archivo o Directorio","mv ​ path_original path_final" },
		{ "cat",fs_cat, "Mostrar contenido de un archivo como texto plano"," cat ​ path_archivo" },
		{ "mkdir",fs_mkdir, "Crear directorio", "mkdir ​ path_dir"},
		{ "cpfrom", fs_cpfrom, "Copiar un archivo local al yamafs","cpfrom ​ path_archivo_origen directorio_yamafs" },
		{ "cpto", fs_cpto, "Copiar un archivo del yamafs al local", "cpto ​ path_archivo_yamafs directorio_filesystem"},
		{ "cpblock", fs_cpblock, "Crear una copia de un bloque de un archivo en el nodo dado","cpblock ​ path_archivo nro_bloque id_nodo" },
		{ "md5", fs_md5, "Solicitar el MD5 de un archivo en yamafs","md5 ​ path_archivo_yamafs" },
		{ "info", fs_info, "Mostrar informacion de un archivo","info ​ path_archivo" },
		{ (char *)NULL, (Function *)NULL, (char *)NULL,(char *)NULL }
};


void iniciar_consola_FS(){

	sleep(1);
	system("clear");
	printf("%d \n",sizeof(commands));
	printf("\n  %s＼(*^0^*)／  Bienvenido al Proceso YAMA-File System   ＼(*^0^*)／ %s \n",amarillo,sin);
	printf("\n    Para obtener información sobre los comandos que puede ejecutar \n 	  y su sintaxis, ingrese '%sayuda%s' \n",cyan,sin);
	char * linea;
	inicializar_completado();
	while(1) {
		linea = readline("YAMA-FS->");
		if(!strncmp(linea,"exit",4)){
			printf("Bai\n");
			free(linea);
			break;

		}else{
			add_history (linea);
			ejecutar_linea(linea);
		}
		free(linea);
	}

}

void inicializar_completado(void){

	rl_attempted_completion_function = command_name_completion;

}

char **command_name_completion(const char *text,int start,int end){

	rl_attempted_completion_over = 1;
	return rl_completion_matches(text, command_generator);
}
int ejecutar_linea (char *line){


	comando *command = find_command (line);
	if (!command){
		printf ("No existe ese comando YAMA-FS.\n");
		return (-1);
	}

	return ((*(command->func)) (line));
}

comando *find_command (char *clinea){

	int i;
	for (i = 0; i< 14; i++)
		if (string_starts_with(clinea,commands[i].name) == 1){
			return (&commands[i]);
		}
	return ((comando *)NULL);
}
char *command_generator (char *text, int state){
	static int list_index, len;
	char *name;

	if (!state){
		list_index = 0;
		len = strlen (text);
	}

	while ((name = commands[list_index++].name)) {
		if (strncmp(name, text, len) == 0) {
			return name;
		}
	}

	return NULL;
}
