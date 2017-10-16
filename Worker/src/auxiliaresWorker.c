#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <commons/string.h>

#include <funcionesCompartidas/logicaNodo.h>
#include <funcionesCompartidas/log.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/generales.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/serializacion.h>

#include "auxiliaresWorker.h"
#include "nettingWorker.h"

#define MINSTR(A, PA, B, PB) ((strcmp(A, B) < 0)? (PA) : (PB))

#define maxline 0x100000 // 1 MiB

static void liberarApareo(int nfiles, FILE *fs[nfiles], char *ls[nfiles-1]);

extern t_log *logw;
extern char *databin;

// todo: trackeate toda mallocacion y frees()

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

	if (chmod(fname, 055) == -1){
		log_error(logw, "No se pudo dar permiso de ejecucion a %s", fname);
		return -1;
	}

	fclose(f);
	return 0;
}

int crearArchivoData(size_t blk, size_t count, char *fname){

	FILE *f;
	if((f = fopen(fname, "w")) == NULL){
		perror("No se pudo abrir el archivo");
		log_error(logw, "No se pudo abrir el archivo %s", fname);
		return -1;
	}

	char *data = getDataBloque(databin, blk);
	if ((fwrite(data, count, 1, f)) != 1){
		log_error(logw, "No se pudo escribir el programa en %s", fname);
		return -1;
	}

	free(data);
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

	char *cmd = crearComando(7, "cat ", data_fname, "|", "./", exe_fname,
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

// todo: revisar liberacion de recursos y parametros
int conectarYCargar(int nquant, t_list *nodos, int **fds, char ***lns){

	int i, ctl;
	char *msj;
	header head;
	t_info_nodo *n;

	// Formalizar conexion con cada Nodo y crear su FILE correspondiente
	for (i = 0; i < nquant; ++i){
		n = list_get(nodos, i);
		if ((*fds[i] = establecerConexion(n->ip, n->port, logw, &ctl)) == -1 ||
			realizarHandshake(*fds[i], 'W') < 0){
			log_error(logw, "No se pudo conectar con Nodo en %s:%s", n->ip, n->port);
			return -1;
		}

		msj = getMessage(*fds[i], &head, &ctl);
		if (ctl == -1 || ctl == 0){
			log_error(logw, "Fallo obtencion mensaje del Nodo en %s:%s", n->fname, n->ip, n->port);
			free(*lns[i]); *lns[i] = NULL;
			return -1;
		}

		*lns[i] = deserializar_stream(msj, &head.sizeData);
	}

	return 0;
}

int apareoGlobal(t_list *nodos, char *fname){

	header head;
	FILE *fout;
	char **lines, *msj;
	int i, ctl, min, nquant, remaining, *fds;
	remaining = nquant = list_size(nodos);

	lines = malloc((size_t) nquant * sizeof *lines);
	fds = malloc((size_t) nquant * sizeof *fds);

	if((fout = fopen(fname, "w")) == NULL){
		perror("Fallo fopen()");
		log_error(logw, "No se pudo abrir el archivo de output %s", fname);
		return -1;
	}

	// Preparamos las primeras lineas a comparar
	if (conectarYCargar(nquant, nodos, &fds, &lines) == -1){
		log_error(logw, "Fallo conexion y carga de textos a aparear");
		return -1;
	}
	while(remaining){

		// Obtenemos posicion del menor string
		for (min = 0, i = 1; i < nquant; ++i){
			if (lines[min] == NULL){
				min++;
				continue;

			} else if(lines[i] == NULL)
				continue;

			min = MINSTR(lines[min], min, lines[i], i);
		}

		// Escribimos la linea en el FILE de output
		fputs(lines[min], fout);

		// Obtenemos la proxima linea del menor string
		msj = getMessage(fds[min], &head, &ctl);
		if (ctl == -1){
			log_error(logw, "Fallo obtencion mensaje de Nodo en %d", fds[min]);
			free(lines[min]); lines[min] = NULL;
			return -1;

		} else if (head.codigo == 12){
			log_trace(logw, "Se recibio EOF para Nodo en %d", fds[min]);
			close(fds[min]); fds[min] = 0;
			lines[min] = NULL;
			remaining--;
			continue;
		}

		lines[min] = deserializar_stream(msj, &head.sizeData);
	}

	return 0;
}

int almacenarFileEnFilesystem(char *fs_ip, char *fs_port, char *fname){

	message *msj;
	t_file *file;
	int sock_fs, ctl;
	char *file_serial;
	header head = {.letra = 'W', .codigo = 20};

	if ((file = cargarFile(fname)) == NULL){
		log_error(logw, "Fallo cargar el t_file %s para enviar", fname);
		return -1;
	}
	file_serial = serializar_File(file, &head.sizeData);
	msj = createMessage(&head, file_serial);

	if ((sock_fs = establecerConexion(fs_ip, fs_port, logw, &ctl)) == -1){
		log_error(logw, "Fallo conectar con FileSystem en %s:%s", fs_ip, fs_port);
		liberador(3, file, file_serial, msj);
		return -1;
	}

	if (realizarHandshake(sock_fs, 'F') == -1){
		log_error(logw, "Fallo handshaking con %s:%s", fs_ip, fs_port);
		liberador(3, file, file_serial, msj);
		return -1;
	}

	if (enviar_message(sock_fs, msj, logw, &ctl) == -1){
		log_error(logw, "Fallo enviar message a FileSystem en %s:%s", fs_ip, fs_port);
		liberador(3, file, file_serial, msj);
		return -1;
	}

	close(sock_fs);
	liberador(3, file, file_serial, msj);
	return 0;
}

t_file *cargarFile(char *fname){

	FILE *f;
	t_file *file = malloc(sizeof *file);
	file->fname = strdup(fname);

	if ((f = fopen(fname, "w")) == NULL){
		log_error(logw, "No se pudo abrir el archivo %s", fname);
		liberador(2, file->fname, file);
		return NULL;
	}

	if ((file->fsize = fsize(f)) == -1){
		log_error(logw, "Fallo calculo del file size de %s", fname);
		liberador(2, file->fname, file);
		return NULL;
	}
	file->data = malloc((size_t) file->fsize);

	fread(file->data, (size_t) file->fsize, 1, f);
	if (ferror(f)){
		log_error(logw, "Fallo lectura de datos del FILE %s", fname);
		liberador(3, file->data, file->fname, file);
		return NULL;
	}

	return file;
}

off_t fsize(FILE* f){

	fseek(f, 0, SEEK_END);
    off_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    return len;
}

void cleanWorkspaceFiles(int nfiles, char *fst, ...){

	char *fname;
	va_list filesp;
	va_start(filesp, fst);

	unlink(fst);
	for (nfiles--; nfiles > 0; nfiles--){
		fname = va_arg(filesp, char*);
		unlink(fname);
	}
}

void terminarEjecucion(int fd_m, int cod_rta){
	enviarResultado(fd_m, cod_rta);
	exit(cod_rta);
}
