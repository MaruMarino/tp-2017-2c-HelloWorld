/*
 * FS_funciones.c
 *
 *  Created on: 12/9/2017
 *      Author: utnso
 */


#include "FS_funciones.h"

#include <commons/string.h>
#include <stdio.h>
#include <stdlib.h>

#include "estructurasfs.h"

extern comando commands[];

#define cyan  "\x1B[36m"
#define sin "\x1B[0m"

comando *buscar_comando(char *nombre);

int fs_ls (char *h){
	printf("Ejecute ls \n");
	return 0;
}
int fs_rename (char *h){
	printf("Ejecute rename \n");
	return 0;
}
int fs_format (char *j){
	printf("Ejecute format \n");
	return 0;
}
int fs_mv (char *k){
	printf("Ejecute mv\n");
	return 0;
}
int fs_ayuda (char *pedido_ayuda){

	printf("COMANDO | OBJETIVO | SINTAXIS\n");

	int var;
	for (var = 0; var < 14; ++var) {
		printf("%s%s%s |  %s  |  %s  \n",cyan,commands[var].name,sin,commands[var].doc, commands[var].sintax);
	}
	return 0;
}
int fs_rm (char *m){
	printf("Ejecute rm\n");
	return 0;
}
int fs_payuda (char *duda){

	char *comando_pedido= string_substring_from(duda,1);
	string_trim(&comando_pedido);

	comando *buscado =buscar_comando(comando_pedido);
	if(buscado == NULL){
		printf(" Ingrese un comando válido,para consultar los disponibles ingrese '%sayuda%s'\n",cyan,sin);
	}
	else{
		printf("%s%s%s | %s | %s \n",cyan,buscado->name,sin,buscado->doc, buscado->sintax);
	}


	free(comando_pedido);

	return 0;
}
int fs_cat (char *n){
	printf("Ejecute cat \n");
	return 0;
}
int fs_mkdir (char *n){
	printf("Ejecute mkdir con %s \n",n);
	return 0;
}
int fs_cpfrom (char *o){
	printf("Ejecute cpfrom \n");
	return 0;
}
int fs_cpto (char *p){
	printf("Ejecute cpto \n");
	return 0;
}
int fs_cpblock (char *q){
	printf("Ejecute cpblock \n");
	return 0;
}
int fs_md5 (char *r){
	printf("Ejecute md5 \n");
	return 0;
}
int fs_info (char *s){
	printf("Ejecute info \n");
	return 0;
}

comando *buscar_comando(char *nombre){

	int i;
	for (i = 0; i< 14; i++){
		if (string_starts_with(nombre,commands[i].name) == 1){
			return (&commands[i]);
		}
	}
	return ((comando *)NULL);

}
