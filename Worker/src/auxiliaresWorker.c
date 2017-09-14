#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <commons/string.h>

#include <funcionesCompartidas/log.h>
#include <funcionesCompartidas/estructuras.h>

#include "auxiliaresWorker.h"

static int cmpstr(const void *p1, const void *p2);

int const maxline = 0x10000; // 1 MiB
extern t_log *logger;

char *crearComando(int nargs, char *fst, ...){
	log_trace(logger, "Se crea un comando de %d argumentos", nargs);

	int i;
	char *arg, *cmd;
	va_list argp;

	if (!(cmd = strdup(fst))){
		log_error(logger, "Fallo duplicacion del string %s", fst);
		return NULL;
	}

	va_start(argp, fst);
	for (i = 1; i < nargs; ++i){
		arg = va_arg(argp, char*);
		string_append(&cmd, arg);
	}

	return cmd;
}


int crearArchivoBin(t_info_trans *info, char *fname){

	if (truncate(fname, 0) == -1){ // elimina fname en caso de que ya existiese
		perror("Error en trucate del archivo");
		log_error(logger, "Fallo truncate() de %s", fname);
		return -1;
	}

	FILE *f;
	if((f = fopen(fname, "wb")) == NULL){
		perror("No se pudo abrir el archivo");
		log_error(logger, "No se pudo abrir el archivo %s", fname);
		return -1;
	}

	if ((fwrite(info->prog, info->size_prog, 1, f)) != 1){
		log_error(logger, "No se pudo escribir el programa en %s", fname);
		return -1;
	}

	fclose(f);
	return 0;
}

int aparearFiles(int nfiles, char *fst, ...){

	int i;
	char *fname;
	FILE *fs[nfiles]; // almacenara un puntero a cada file;
	va_list filep;
	va_start(filep, fst);

	if((fs[0] = fopen(fst, "r")) == NULL){
		perror("No se pudo abrir el archivo");
		log_error(logger, "No se pudo abrir el archivo %s", fst);
		return -1;
	}

	// abrimos todos los archivos en modo lectura salvo el ultimo
	for (i = 1; i < nfiles -1; ++i){
		fname = va_arg(filep, char*);
		if((fs[i] = fopen(fname, "r")) == NULL){
			perror("No se pudo abrir el archivo");
			log_error(logger, "No se pudo abrir el archivo %s", fname);
			return -1;
		}
	}

	// el ultimo file lo abrimos en modo escritura
	fname = va_arg(filep, char*);
	if((fs[nfiles - 1] = fopen(fname, "w")) == NULL){
		perror("No se pudo abrir el archivo");
		log_error(logger, "No se pudo abrir el archivo %s", fname);
		return -1;
	}

	return realizarApareo(nfiles, fs);
}

int realizarApareo(int nfiles, FILE *fs[nfiles]){

	int minp, i, nulls;
	int apareadas = 0;
	char *ls[nfiles - 1];

	for (i = 0; i < nfiles - 1; ++i)
		ls[i] = malloc((size_t) maxline);

	for (i = 0; i < nfiles - 1; ++i)
		fgets(ls[i], maxline, fs[i]);

	while(1){
		for (i = minp = nulls = 0; i < nfiles - 1; ++i){

			if (fs[minp] == NULL){
				nulls++;
				minp++;
				continue;

			} else if (fs[i] == NULL){
				nulls++;
				continue;
			}

			minp = (strncmp(ls[minp], ls[i], (size_t) maxline) > 0)? i : minp;
		}

		if (nulls == nfiles -1)
			break;

		fputs(ls[minp], fs[nfiles - 1]);
		fgets(ls[minp], maxline, fs[minp]);
		apareadas++;

		if (feof(fs[minp])){
			fclose(fs[minp]); fs[minp] = NULL;
			free(ls[minp]);   ls[minp] = NULL;
		}
	}

	fclose(fs[nfiles - 1]);
	return apareadas;
}


long int fsize(FILE* f){

    fseek(f, 0, SEEK_END);
    long int len = ftell(f);
    fseek(f, 0, SEEK_SET);
    return len;
}

void sort(char ***lines, size_t line_count){
	qsort((*lines), line_count, sizeof **lines, cmpstr);
}

/* magia en el ejemplo del `man qsort' */
static int cmpstr(const void *p1, const void *p2){
	return strcmp(*(char**) p1, *(char**) p2);
}

size_t countLines(FILE *f){

	long prev_pos = ftell(f);
	size_t lines = 0;
	char *aux = malloc((size_t) maxline);

	while(fgets(aux, maxline, f) != NULL)
		lines++;

	if (ferror(f)) // ocurrio un error durante lectura
		lines = 0;

	fseek(f, prev_pos, SEEK_SET);
	free(aux);
	return lines;
}

int writeArrayIntoFile(char **lines, int dim, const char *path){

	if (truncate(path, 0) == -1){
		perror("Error en trucate del archivo:");
		log_error(logger, "Fallo truncate() de %s", path);
		return -1;
	}

	FILE *f;
	if ((f = fopen(path, "w")) == NULL){
		perror("Error en fopen:");
		log_error(logger, "Fallo fopen() de %s", path);
		return -1;
	}

	int i;
	for (i = 0; i < dim && !ferror(f); ++i)
		fputs(lines[i], f);

	if (ferror(f)){
		log_error(logger, "Error durante escritura de %s", path);
		fclose(f);
		return -1;
	}

	fclose(f);
	return 0;
}

char **readFileIntoArray(FILE *f, size_t *dim){

	size_t i;
	long prev_pos = ftell(f);

	if ((*dim = countLines(f)) == 0){
		log_error(logger, "Fallo conteo de lineas! No se carga el array...");
		return NULL;
	}

	char *aux = malloc((size_t) maxline);
	char **lines = malloc(*dim * sizeof (char*));
	for (i = 0; i < *dim && !ferror(f); ++i){

		fgets(aux, maxline, f);
		lines[i] = malloc(strlen(aux));
		strcpy(lines[i], aux);
	}

	if (aux == NULL){ // avisamos el error y liberamos toda la memoria asignada
		log_error(logger, "Fallo fgets()! No se carga el array...");
		fseek(f, prev_pos, SEEK_SET);
		for (; i > 0; free(lines[i]), i--);
		free(lines);
		free(aux);
		*dim = 0;
		return NULL;
	}

	fseek(f, prev_pos, SEEK_SET);
	free(aux);
	return lines;
}



