#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <funcionesCompartidas/log.h>

static int cmpstr(const void *p1, const void *p2);

int const maxline = 0x10000; // 1 MiB
extern t_log *logger;

long int fsize(FILE* f){

    fseek(f, 0, SEEK_END);
    long int len = ftell(f);
    fseek(f, 0, SEEK_SET);
    return len;
}

void sort(char ***lines, int line_count){
	qsort((*lines), line_count, sizeof **lines, cmpstr);
}

/* magia en el ejemplo del `man qsort' */
static int cmpstr(const void *p1, const void *p2){
	return strcmp(*(char**) p1, *(char**) p2);
}

int countLines(FILE *f){

	long prev_pos = ftell(f);
	size_t lines = 0;
	char *aux = malloc(maxline);

	while(fgets(aux, maxline, f) != NULL)
		lines++;

	if (ferror(f)) // ocurrio un error durante lectura
		lines = -1;

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

char **readFileIntoArray(FILE *f, int *dim){

	int i;
	long prev_pos = ftell(f);

	if ((*dim = countLines(f)) == -1){
		log_error(logger, "Fallo conteo de lineas! No se carga el array...");
		return NULL;
	}

	char *aux = malloc(maxline);
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
		*dim = -1;
		return NULL;
	}

	fseek(f, prev_pos, SEEK_SET);
	free(aux);
	return lines;
}



