#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <commons/string.h>

#include <funcionesCompartidas/log.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/generales.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/serializacion.h>

#include "auxiliaresWorker.h"
#include "nettingWorker.h"

#define maxline 0x10000 // 1 MiB

static int cmpstr(const void *p1, const void *p2);
static void liberarApareo(int nfiles, FILE *fs[nfiles], char *ls[nfiles-1]);

extern t_log *logw;
char *databin;

char *crearComando(int nargs, char *fst, ...){
	log_trace(logw, "Se crea un comando de %d argumentos", nargs);

	int i;
	char *arg, *cmd;
	va_list argp;

	if (!(cmd = strdup(fst))){
		log_error(logw, "Fallo duplicacion del string %s", fst);
		return NULL;
	}

	va_start(argp, fst);
	for (i = 1; i < nargs; ++i){
		arg = va_arg(argp, char*);
		string_append(&cmd, arg);
	}

	return cmd;
}

int crearArchivoBin(char *bin, size_t bin_sz, char *fname){

	if (truncate(fname, 0) == -1){ // elimina fname en caso de que ya existiese
		perror("Error en trucate del archivo");
		log_error(logw, "Fallo truncate() de %s", fname);
		return -1;
	}

	FILE *f;
	if((f = fopen(fname, "wb")) == NULL){
		perror("No se pudo abrir el archivo");
		log_error(logw, "No se pudo abrir el archivo %s", fname);
		return -1;
	}

	if ((fwrite(bin, bin_sz, 1, f)) != 1){
		log_error(logw, "No se pudo escribir el programa en %s", fname);
		return -1;
	}

	fclose(f);
	return 0;
}

char *getDataBloque(size_t blk, size_t count){

	char *dat = malloc(count);
	return memcpy(dat, databin + maxline * blk, count);
}

int crearArchivoData(size_t blk, size_t count, char *fname){

	if (truncate(fname, 0) == -1){ // elimina fname en caso de que ya existiese
		perror("Error en trucate del archivo");
		log_error(logw, "Fallo truncate() de %s", fname);
		return -1;
	}

	FILE *f;
	if((f = fopen(fname, "w")) == NULL){
		perror("No se pudo abrir el archivo");
		log_error(logw, "No se pudo abrir el archivo %s", fname);
		return -1;
	}

	char *data = getDataBloque(blk, count);

	if ((fwrite(data, count, 1, f)) != 1){
		log_error(logw, "No se pudo escribir el programa en %s", fname);
		return -1;
	}

	fclose(f);
	return 0;
}

int aparearFiles(t_list *fnames, char *fout){

	int i;
	int nfiles = list_size(fnames) + 1; // + 1 para el fout
	FILE *fs[nfiles]; // almacenara un puntero a cada file;
	char *fn;

	for (i = 0; i < nfiles - 2; ++i){

		fn = list_get(fnames, i);
		if((fs[i] = fopen(fn, "r")) == NULL){
			perror("No se pudo abrir el archivo");
			log_error(logw, "No se pudo abrir el archivo %s", fn);
			return -1;
		}
	}

	if((fs[nfiles -1] = fopen(fout, "w")) == NULL){
		perror("No se pudo abrir el archivo");
		log_error(logw, "No se pudo abrir el archivo %s", fout);
		return -1;
	}

	return realizarApareo(nfiles, fs);
}

int realizarApareo(int nfiles, FILE *fs[nfiles]){

	int minp, i, nulls;
	int apareadas = 0;
	char *ls[nfiles - 1];

	for (i = 0; i < nfiles - 1; ++i)
		ls[i] = malloc(maxline);

	for (i = 0; i < nfiles - 1; ++i){
		fgets(ls[i], maxline, fs[i]);
		if (ferror(fs[i])){
			log_error(logw, "Error lectura de archivo apareado");
			liberarApareo(nfiles, fs, ls);
			return -1;
		}
	}

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
			minp = (strncmp(ls[minp], ls[i], maxline) > 0)? i : minp;
		}

		if (nulls == nfiles -1)
			break;

		fputs(ls[minp], fs[nfiles - 1]);
		if (feof(fs[nfiles - 1])){
			log_error(logw, "Error de escritura sobre archivo de output");
			liberarApareo(nfiles, fs, ls);
			return -1;
		}

		fgets(ls[minp], maxline, fs[minp]);
		if (ferror(fs[minp])){
			log_error(logw, "Error lectura de archivo apareado");
			liberarApareo(nfiles, fs, ls);
			return -1;

		} else if (feof(fs[minp])){
			fclose(fs[minp]); fs[minp] = NULL;
			free(ls[minp]);   ls[minp] = NULL;
		}
		apareadas++;
	}

	fclose(fs[nfiles - 1]);
	return apareadas;
}

int makeCommandAndExecute(char *data_fname, char *exe_fname, char *out_fname){

	char *cmd = crearComando(6, "cat ", data_fname, "|", exe_fname,
			" | sort -dib > ", out_fname);
	if (!cmd){
		log_error(logw, "Fallo la creacion del comando a ejecutar.");
		liberador(3, exe_fname, data_fname, cmd);
		return -1;
	}

	if (!system(cmd)){ // ejecucion del comando via system()
		log_error(logw, "Llamada a system() con comando %s fallo.", cmd);
		liberador(3, exe_fname, data_fname, cmd);
		return -1;
	}

	return 0;
}

static void liberarApareo(int nfiles, FILE *fs[nfiles], char *ls[nfiles-1]){

	int i;
	for (i = 0; i < nfiles; ++i){
		if (fs[i] == NULL)
			continue;

		fclose(fs[i]); fs[i] = NULL;
		free(ls[i]);   ls[i] = NULL;
	}
}

int apareoGlobal(t_list *nodos, char *fname){

	header head;
	t_info_nodo *n;
	FILE *fout;
	int i, ctl, nquant, min;
	nquant = list_size(nodos);

	char *lines[nquant], *msj;
	int fds[nquant];

	// Formalizar conexion con cada Nodo y crear su FILE correspondiente
	for (i = 0; i < nquant; ++i){
		n = list_get(nodos, i);
		if ((fds[i] = establecerConexion(n->ip, n->port, logw, &ctl)) == -1 ||
			realizarHandshake(fds[i], 'W') < 0){
			log_error(logw, "No se pudo conectar con Nodo en %s:%s", n->ip, n->port);
			return -1;
		}
	}

	if (truncate(fname, 0) == -1){
		log_error(logw, "Fallo truncate() del archivo output %s", fname);
		return -1;

	} else if((fout = fopen(fname, "w")) == NULL){
		log_error(logw, "No se pudo abrir el archivo de output %s", fname);
		return -1;
	}

	// Recibimos las primeras lineas
	for (i = 0; i < nquant; ++i){
		if (lines[i] == NULL && fds[i] ) continue;

		msj = getMessage(fds[i], &head, &ctl);
		if (ctl == -1 || ctl == 0){
			log_error(logw, "Fallo obtencion mensaje del Nodo en %s:%s", n->fname, n->ip, n->port);
			free(lines[i]); lines[i] = NULL;
			return -1;
		}

		lines[i] = deserializar_stream(msj, &head.sizeData);
	}

	while(1){ // todo: hacer convergente

		// comparar lineas; voy a tener int min = X con la posicion ganadora
		// llamo fprintf(fout, lines[i])
		fprintf(fout, lines[min]);
		// Obtenemos la proxima linea del Worker ganador
		msj = getMessage(fds[min], &head, &ctl);
		if (ctl == -1){
			log_error(logw, "Fallo obtencion mensaje del Nodo en %s:%s", n->fname, n->ip, n->port);
			free(lines[min]); lines[min] = NULL;
			return -1;

		} else if (head.codigo == 12){
			log_trace(logw, "Se recibio EOF para Nodo en %s:%s", n->fname, n->ip, n->port);
			close(fds[min]); fds[min] = -1;
			lines[min] = NULL;
		}

		lines[min] = deserializar_stream(msj, &head.sizeData);



		// Recibir la linea del Nodo que falte
		for (i = 0; i < nquant; ++i){
			if (fds[i] < 0) continue; // el fd esta cerrado

			msj = getMessage(fds[i], &head, &ctl);
			if (ctl == -1 || ctl == 0){
				log_error(logw, "Fallo obtencion mensaje del Nodo en %s:%s", n->fname, n->ip, n->port);
				// todo: liberar todos los recursos que se pierden
				free(lines[i]);
				fds[i] = -1;
				return -1;
			}



			lines[i] = deserializar_stream(msj, &head.sizeData);
		}



	}

	return 0;
}

int reproducirFiles(t_list *nodos){

	int i;
	char *f_data;
	size_t fsize;
	t_info_nodo *n;

	for (i = 0; i < list_size(nodos); ++i){
		n = list_get(nodos, i);

		if ((f_data = obtenerFileData(n, &fsize)) == NULL){
			log_error(logw, "Fallo obtencion de %s del Nodo en %s:%s", n->fname, n->ip, n->port);
			return -1;
		}

		if (crearArchivoBin(f_data, fsize, n->fname) == -1){
			log_error(logw, "Fallo creacion del archivo %s", n->fname);
			return -1;
		}

		free(f_data);
	}

	return 0;
}

//todo: rehacer esto que hiciste par nada INIAKI
char *obtenerFileData(t_info_nodo *n, size_t *fsize){

	int ctl, nodo_fd;
	char *serial_data, *fdata;
	message *msj;
	header head = {.letra = 'W', .codigo = 20, .sizeData = strlen(n->fname) + 1};

	if ((nodo_fd = establecerConexion(n->ip, n->port, logw, &ctl)) == -1){
		log_error(logw, "No se pudo conectar con Nodo en %s:%s", n->ip, n->port);
		return NULL;
	}

	if (realizarHandshake(nodo_fd, 'W') < 0){
		log_error(logw, "Fallo handshaking con Nodo en %s:%s", n->ip, n->port);
		return NULL;
	}

	msj = createMessage(&head, n->fname);
	if (enviar_message(nodo_fd, msj, logw, &ctl) < 0){
		log_error(logw, "Fallo envio de mensaje al Nodo en %s:%s", n->fname, n->ip, n->port);
		free(msj);
		return NULL;
	}

	serial_data = getMessage(nodo_fd, &head, &ctl);
	if (ctl == -1 || ctl == 0){
		log_error(logw, "Fallo obtencion mensaje del Nodo en %s:%s", n->fname, n->ip, n->port);
		free(msj);
		free(serial_data);
		return NULL;
	}

	fdata = deserializar_stream(serial_data, fsize);

	close(nodo_fd);
	free(msj);
	free(serial_data);

	return fdata;
}

int aparearFiles_(int nfiles, char *fst, ...){

	int i;
	char *fname;
	FILE *fs[nfiles]; // almacenara un puntero a cada file;
	va_list filep;
	va_start(filep, fst);

	if((fs[0] = fopen(fst, "r")) == NULL){
		perror("No se pudo abrir el archivo");
		log_error(logw, "No se pudo abrir el archivo %s", fst);
		return -1;
	}

	// abrimos todos los archivos en modo lectura salvo el ultimo
	for (i = 1; i < nfiles -1; ++i){
		fname = va_arg(filep, char*);
		if((fs[i] = fopen(fname, "r")) == NULL){
			perror("No se pudo abrir el archivo");
			log_error(logw, "No se pudo abrir el archivo %s", fname);
			return -1;
		}
	}

	// el ultimo file lo abrimos en modo escritura
	fname = va_arg(filep, char*);
	if((fs[nfiles - 1] = fopen(fname, "w")) == NULL){
		perror("No se pudo abrir el archivo");
		log_error(logw, "No se pudo abrir el archivo %s", fname);
		return -1;
	}

	return realizarApareo(nfiles, fs);
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
	char *aux = malloc(maxline);

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
		log_error(logw, "Fallo truncate() de %s", path);
		return -1;
	}

	FILE *f;
	if ((f = fopen(path, "w")) == NULL){
		perror("Error en fopen:");
		log_error(logw, "Fallo fopen() de %s", path);
		return -1;
	}

	int i;
	for (i = 0; i < dim && !ferror(f); ++i)
		fputs(lines[i], f);

	if (ferror(f)){
		log_error(logw, "Error durante escritura de %s", path);
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
		log_error(logw, "Fallo conteo de lineas! No se carga el array...");
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
		log_error(logw, "Fallo fgets()! No se carga el array...");
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



