/*
 * FS_funciones.c
 *
 *  Created on: 12/9/2017
 *      Author: utnso
 */


#include "FS_funciones.h"

#include <commons/string.h>
#include <funcionesCompartidas/generales.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>

#include "FS_administracion.h"
#include "estructurasfs.h"

extern comando commands[];
extern yamafs_config *configuracion;
extern t_log *logi;

#define cyan  "\x1B[36m"
#define sin "\x1B[0m"

comando *buscar_comando(char *nombre);

int fs_ls(char *h) {
	printf("Ejecute ls \n");
	return 0;
}

int fs_rename(char *i) {
	printf("Ejecute rename \n");
	return 0;
}

int fs_format(char *j) {

	if (configuracion->inicio_limpio) {

        crear_subdirectorios();
        iniciar_arbol_directorios();
        iniciar_nodos();
        iniciar_bitmaps_nodos();

		configuracion->estado_estable = 1;

	}
	return 0;
}

int fs_mv(char *k) {
	printf("Ejecute mv\n");
	return 0;
}

int fs_ayuda(char *pedido_ayuda) {

	printf("COMANDO | OBJETIVO | SINTAXIS\n");

	int var;
	for (var = 0; var < 14; ++var) {
		printf("%s%s%s |  %s  |  %s  \n", cyan, commands[var].name, sin, commands[var].doc, commands[var].sintax);
	}
	return 0;
}

int fs_rm(char *m) {
	printf("Ejecute rm\n");
	return 0;
}

int fs_payuda(char *duda) {

	char *comando_pedido = string_substring_from(duda, 1);
	string_trim(&comando_pedido);

	comando *buscado = buscar_comando(comando_pedido);
	if (buscado == NULL) {
		printf("Ingrese un comando válido,para consultar los disponibles ingrese '%sayuda%s'\n", cyan, sin);
	} else {
		printf("%s%s%s | %s | %s \n", cyan, buscado->name, sin, buscado->doc, buscado->sintax);
	}


	free(comando_pedido);

	return 0;
}

int fs_cat(char *n) {
	printf("Ejecute cat \n");
	return 0;
}

int fs_mkdir(char *p) {

	log_info(logi,"Ejecute mkdir con %s \n", p);

	return 0;
}

int fs_cpfrom(char *q) {


	int i = 0;
	char *paths = string_substring_from(q,6);
	char **split_paths = string_split(paths," ");

	while(split_paths[i]!= NULL){
		i++;
	};

	if(i == 4){
		//todo cantidad correcta de parametros, ejecutar comando
	}else{
		printf("La cantidad de parámetros es incorrecta, ingrese '%s? cpfrom%s' para más información\n",cyan,sin);
	}

	log_info(logi,"Ejecute cpfrom \n");
	return 0;
}

int fs_cpto(char *r) {
    printf("Ejecute cpto \n");
    return 0;
}

int fs_cpblock(char *s) {
	printf("Ejecute cpblock \n");
	return 0;
}

int fs_md5(char *t) {
	printf("Ejecute md5 \n");
	return 0;
}

int fs_info(char *u) {
	printf("Ejecute info \n");
	return 0;
}

comando *buscar_comando(char *nombre) {

	int i;
	for (i = 0; i < 14; i++) {
		if (string_starts_with(nombre, commands[i].name) == 1) {
			return (&commands[i]);
		}
	}
	return ((comando *) NULL);

}
