#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <commons/string.h>

#include <funcionesCompartidas/logicaNodo.h>
#include <funcionesCompartidas/log.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/generales.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/serializacion.h>

#include "configuracionWorker.h"
#include "auxiliaresWorker.h"
#include "nettingWorker.h"
#include "estructurasLocales.h"

#define MINSTR(A, PA, B, PB) ((strcmp(A, B) < 0)? (PA) : (PB))

#define maxline 0x100000 // 1 MiB
#define FD_SELF -1

extern char *databin;
extern size_t dsize;

static void liberarBuffers(int n, char **buff);

static void liberarFILEs(int n, FILE *fs[n]);

static void liberarLines(int n, char *ls[n]);

static void liberarApareo(int nfiles, FILE *fs[nfiles], char *ls[nfiles - 1]);

static void cerrarSockets(int nfds, int *fds);

extern t_log *logw;
extern char *databin;

char *crearComando(int nargs, char *fst, ...) {
    log_trace(logw, "Se crea un comando de %d argumentos", nargs);

    int i;
    char *arg, *cmd;
    va_list argp;

    if (!(cmd = strdup(fst))) {
        log_error(logw, "Fallo duplicacion del string %s", fst);
        return NULL;
    }

    va_start(argp, fst);
    for (i = 1; i < nargs; ++i) {
        arg = va_arg(argp, char*);
        string_append(&cmd, arg);
    }

    return cmd;
}

int crearArchivoBin(char *bin, size_t bin_sz, char *fname) {
    log_trace(logw, "Se crea archivo ejecutable %s", fname);

    FILE *f;
    if ((f = fopen(fname, "wb")) == NULL) {
        perror("No se pudo abrir el archivo");
        log_error(logw, "No se pudo abrir el archivo %s", fname);
        return -1;
    }

    if ((fwrite(bin, bin_sz, 1, f)) != 1) {
        log_error(logw, "No se pudo escribir el programa en %s", fname);
        fclose(f);
        return -1;
    }

    // read-execute binary
    if (chmod(fname, 0500) == -1) {
        log_error(logw, "No se pudo dar permiso de ejecucion a %s", fname);
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

int crearArchivoData(size_t blk, size_t count, char *fname) {
    log_trace(logw, "Se crea archivo de datos %s", fname);

    FILE *f;
    if ((f = fopen(fname, "w")) == NULL) {
        perror("No se pudo abrir el archivo");
        log_error(logw, "No se pudo abrir el archivo %s", fname);
        return -1;
    }

    char *data = getDataBloque(databin, blk);
    if ((fwrite(data, count, 1, f)) != 1) {
        log_error(logw, "No se pudo escribir el programa en %s", fname);
        fclose(f);
        return -1;
    }

    // read-only data
    if (chmod(fname, 0400) == -1) {
        log_error(logw, "No se pudo dar permiso de ejecucion a %s", fname);
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

int aparearFiles(t_list *fnames, char *fout) {
    log_trace(logw, "Se van a aparear %d files", list_size(fnames));

    int i;
    int nfiles = list_size(fnames) + 1; // + 1 para el fout
    FILE *fs[nfiles]; // almacenara un puntero a cada file;
    char *fn;

    for (i = 0; i < nfiles - 1; ++i) {

        fn = list_get(fnames, i);
        if ((fs[i] = fopen(fn, "r")) == NULL) {
            perror("No se pudo abrir el archivo");
            log_error(logw, "No se pudo abrir el archivo %s", fn);
            liberarFILEs(i - 1, fs);
            return -1;
        }
    }

    if ((fs[nfiles - 1] = fopen(fout, "w")) == NULL) {
        perror("No se pudo abrir el archivo");
        log_error(logw, "No se pudo abrir el archivo %s", fout);
        liberarFILEs(nfiles - 2, fs);
        return -1;
    }

    return realizarApareo(nfiles, fs);
}

int realizarApareo(int nfiles, FILE *fs[nfiles]) {
    log_trace(logw, "Se comienza a realizar el apareo efectivo de los files");

    int minp, i, nulls;
    int apareadas = 0;
    int nlines = nfiles - 1;
    char *ls[nlines];

    for (i = 0; i < nlines; ++i)
        ls[i] = malloc(maxline);

    for (i = 0; i < nlines; ++i) {
        fgets(ls[i], maxline, fs[i]);
        if (ferror(fs[i])) {
            log_error(logw, "Error lectura de archivo apareado");
            liberarApareo(nfiles, fs, ls);
            return -1;
        }
    }

    while (1) {
        for (i = minp = nulls = 0; i < nlines; ++i) {

            if (fs[minp] == NULL) {
                nulls++;
                minp++;
                continue;

            } else if (fs[i] == NULL)
                continue;

            minp = (strcoll(ls[minp], ls[i]) > 0) ? i : minp;
        }

        if (nulls == nlines)
            break;

        fputs(ls[minp], fs[nfiles - 1]);
        if (feof(fs[nfiles - 1])) {
            log_error(logw, "Error de escritura sobre archivo de output");
            liberarApareo(nfiles, fs, ls);
            return -1;
        }

        fgets(ls[minp], maxline, fs[minp]);
        if (ferror(fs[minp])) {
            log_error(logw, "Error lectura de archivo apareado");
            liberarApareo(nfiles, fs, ls);
            return -1;

        } else if (feof(fs[minp])) {
            log_trace(logw, "File EOF alcanzado para fs[%d]", minp);
            fclose(fs[minp]);
            fs[minp] = NULL;
            free(ls[minp]);
            ls[minp] = NULL;
            nulls++;
        }
        apareadas++;
    }
    log_info(logw, "Apareo finalizado OK");
    liberarApareo(nfiles, fs, ls);
    return apareadas;
}

int makeCommandAndExecute(char *data_fname, char *exe_fname, char *out_fname) {
    log_trace(logw, "CHILD [%d] crea y ejecuta su comando", getpid());

    char *cmd = crearComando(7, "cat ", data_fname, "|", "./", exe_fname,
                             " | sort > ", out_fname);
    if (!cmd) {
        log_error(logw, "Fallo la creacion del comando a ejecutar.");
        return -1;
    }

    log_trace(logw, "Se llama a system para ejecutar comando %s", cmd);

    if (system(cmd) != 0) { // ejecucion del comando via system()
        log_error(logw, "Llamada a system() con comando %s fallo.", cmd);
        free(cmd);
        return -1;
    }

    free(cmd);
    return 0;
}

int conectarYCargar(int nquant, t_list *nodos, int **fds, char ***lns) {
    log_trace(logw, "Se conecta a cada nodo y cargan las primeras lineas a aparear");

    int i, ctl;
    char *msj;
    message *req;
    header head = {.letra = 'W', .codigo = FILE_REQ};
    t_info_nodo *n;

    // Formalizar conexion con cada Nodo y crear su FILE correspondiente
    for (i = 0; i < nquant; ++i) {
        n = list_get(nodos, i);

        if (((*fds)[i] = establecerConexion(n->ip, n->port, logw, &ctl)) == -1) {
            log_error(logw, "No se pudo conectar con Nodo en %s:%s", n->ip, n->port);
            cerrarSockets(i - 1, *fds);
            return -1;
        }

        if (realizarHandshake((*fds)[i], 'W') < 0) {
            log_error(logw, "No se pudo conectar con Nodo en %s:%s", n->ip, n->port);
            cerrarSockets(i, *fds);
            return -1;
        }

        // Enviar el filename que debe abrir el Worker Servidor
        msj = serializar_FName(n->fname, &head.sizeData);
        req = createMessage(&head, msj);
        if (enviar_message((*fds)[i], req, logw, &ctl) < 0) {
            log_error(logw, "Fallo envio de mensaje a Nodo en %s:%s", n->ip, n->port);
            cerrarSockets(i, *fds);
            liberador(3, msj, req->buffer, req);
            return -1;
        }
        liberador(3, msj, req->buffer, req);

        msj = getMessageIntr((*fds)[i], &head, &ctl);
        if (ctl == -1 || ctl == 0) {
            log_error(logw, "Fallo obtencion mensaje del Nodo en %s:%s", n->ip, n->port);
            cerrarSockets(i, *fds);
            liberarBuffers(i, *lns);
            free(msj);
            return -1;
        }

        (*lns)[i] = deserializar_stream(msj, &head.sizeData);
        free(msj);
    }

    return 0;
}

//todo: fortificar error checking y memory freeing
int apareoGlobal(t_list *nodos, char *fname) {
    log_trace(logw, "Se realizara apareo global de files");

    FILE *fout;
    char **lines, *msj;
    int i, ctl, min, nquant, remaining, *fds, lcount;
    remaining = nquant = list_size(nodos);
    header head = {.letra = 'W', .codigo = FILE_REQ, .sizeData = 1};
    message *req = createMessage(&head, "");

    lines = malloc((size_t) nquant * sizeof *lines);
    fds = malloc((size_t) nquant * sizeof *fds);

    if ((fout = fopen(fname, "w")) == NULL) {
        perror("Fallo fopen()");
        log_error(logw, "No se pudo abrir el archivo de output %s", fname);
        liberador(4, lines, fds, req->buffer, req);
        return -1;
    }

    // Preparamos las primeras lineas a comparar
    if (conectarYCargar(nquant, nodos, &fds, &lines) == -1) {
        log_error(logw, "Fallo conexion y carga de textos a aparear");
        liberador(4, lines, fds, req->buffer, req);
        return -1;
    }

    lcount = nquant; // inicializa contador de lineas de archivos
    while (remaining) {

        // Obtenemos posicion del menor string
        for (min = 0, i = 1; i < nquant; ++i) {
            if (lines[min] == NULL) {
                min++;
                continue;

            } else if (lines[i] == NULL)
                continue;

            min = MINSTR(lines[min], min, lines[i], i);
        }

        // Escribimos la linea en el FILE de output
        if (fputs(lines[min], fout) < 0) {
            log_error(logw, "Fallo escribir linea %s en %s", lines[min], fout);
            liberarBuffers(nquant, lines);
            cerrarSockets(nquant, fds);
            liberador(4, lines, fds, req->buffer, req);
            fclose(fout);
            return -1;
        }

        // Pedimos la proxima linea al Nodo ganador
        if (enviar_message(fds[min], req, logw, &ctl) == -1) {
            log_error(logw, "Fallo pedido siguiente linea a Nodo en %d", fds[min]);
            liberarBuffers(nquant, lines);
            cerrarSockets(nquant, fds);
            liberador(4, lines, fds, req->buffer, req);
            fclose(fout);
            return -1;
        }

        // Obtenemos la proxima linea del Nodo ganador
		msj = getMessageIntr(fds[min], &head, &ctl);
        if (ctl == -1) {
            log_error(logw, "Fallo obtencion mensaje de Nodo en %d", fds[min]);
            liberarBuffers(nquant, lines);
            cerrarSockets(nquant, fds);
            liberador(4, lines, fds, req->buffer, req);
            fclose(fout);
            return -1;

        } else if (head.codigo == APAR_ERR || head.codigo == APAR_INV) {
            log_error(logw, "Nodo en %d fallo al leer su linea", fds[min]);
            liberarBuffers(nquant, lines);
            cerrarSockets(nquant, fds);
            liberador(5, lines, fds, req->buffer, req, msj);
            fclose(fout);
            return -1;

        } else if (head.codigo == APAR_EOF) {
            log_trace(logw, "Se recibio EOF para Nodo en %d", fds[min]);
            close(fds[min]);
            fds[min] = 0;
            liberador(2, lines[min], msj);
            lines[min] = NULL;
            remaining--;
            continue;
        }

        // Deserializamos la linea sobre el buffer de lineas
        free(lines[min]);
        lines[min] = deserializar_stream(msj, &head.sizeData);
        free(msj);
        lcount++;
    }

    liberador(4, lines, fds, req->buffer, req);
    fclose(fout);
    return lcount;
}

int almacenarFileEnFilesystem(char *fs_ip, char *fs_port, t_file *file){
	log_trace(logw, "Se realiza el almacenamiento en YAMAFS de %s", file->fname);

	char *msj;
	int sock_fs, ctl, rta;
	header head;

	if ((sock_fs = enviarFile(fs_ip, fs_port, file)) == -1){
		log_error(logw, "Fallo enviar %s a FileSystem en %s:%s", file->fname, fs_ip, fs_port);
		return -1;
	}

	msj = getMessageIntr(sock_fs, &head, &ctl);
	if (ctl == -1 || ctl == 0)
		log_error(logw, "Fallo obtencion mensaje de FileSystem en %d. Error: %d", sock_fs, ctl);

	else if (head.codigo != 14)
		log_error(logw, "Recibio de FileSystem en %d un mensaje invalido: %d", sock_fs, head.codigo);

	else
		memcpy(&rta, msj, sizeof(int));

	free(msj);
	close(sock_fs);
	return rta;
}

t_file *cargarFile(char *fname, char *yamafn) {
    log_trace(logw, "Se carga el file %s en un archivo para enviar", fname);

    FILE *f;
    t_file *file = malloc(sizeof *file);
    file->fname = strdup(yamafn);

    if ((f = fopen(fname, "r")) == NULL) {
        log_error(logw, "No se pudo abrir el archivo %s", fname);
        liberador(2, file->fname, file);
        return NULL;
    }

    if ((file->fsize = fsize(f)) == 0) {
        log_error(logw, "Fallo calculo del file size de %s", fname);
        liberador(2, file->fname, file);
        fclose(f);
        return NULL;
    }
    file->data = malloc((size_t) file->fsize);

    fread(file->data, (size_t) file->fsize, 1, f);
    if (ferror(f)) {
        log_error(logw, "Fallo lectura de datos del FILE %s", fname);
        liberador(3, file->data, file->fname, file);
        fclose(f);
        return NULL;
    }

    fclose(f);
    return file;
}

int enviarFile(char *fs_ip, char *fs_port, t_file *file){

	int sock_fs, ctl;
	message *msj;
	char *file_serial;
	header head = {.letra = 'W', .codigo = ALMAC_FS};

	file_serial = serializar_File(file, &head.sizeData);
	msj = createMessage(&head, file_serial);

	if ((sock_fs = establecerConexion(fs_ip, fs_port, logw, &ctl)) == -1) {
		log_error(logw, "Fallo conectar con FileSystem en %s:%s", fs_ip, fs_port);
		liberador(3, file_serial, msj->buffer, msj);
		return -1;
	}

	if (realizarHandshake(sock_fs, 'F') == -1) {
		log_error(logw, "Fallo handshaking con %s:%s", fs_ip, fs_port);
		liberador(3, file_serial, msj->buffer, msj);
		close(sock_fs);
		return -1;
	}

	if (enviar_messageIntr(sock_fs, msj, logw, &ctl) == -1) {
		log_error(logw, "Fallo enviar message a FileSystem en %s:%s", fs_ip, fs_port);
		liberador(3, file_serial, msj->buffer, msj);
		close(sock_fs);
		return -1;
	}
	log_trace(logw, "Se envio el file %s a FileSystem en %d", file->fname, sock_fs);

	liberador(3, file_serial, msj->buffer, msj);
	return sock_fs;
}

off_t fsize(FILE *f) {
    off_t len, off_init;
    off_init = (f->_offset == -1) ? 0 : (off_t) f->_offset;
    if (fseek(f, 0, SEEK_END) != 0) return 0;
    if ((len = ftell(f)) == -1) len = 0;
    if (fseek(f, off_init, SEEK_SET) != 0) return 0;
    return len;
}

void cleanWorkspaceFiles(int nfiles, char *fst, ...) {

    char *fname;
    va_list filesp;
    va_start(filesp, fst);

    if (unlink(fst) < 0) {
        perror("Fallo unlink:");
        log_info(logw, "No pudo borrar el archivo %s. Continuando...", fst);
    }
    for (nfiles--; nfiles > 0; nfiles--) {
        fname = va_arg(filesp, char*);
        if (unlink(fname) < 0) {
            perror("Fallo unlink");
            log_info(logw, "No pudo borrar el archivo %s. Continuando...", fname);
        }
    }
}

void terminarEjecucion(int fd_m, int cod_rta, t_conf *conf, char *exe, char *dat) {
    log_trace(logw, "Se termina la ejecucion del CHILD [%d]", getpid());

    enviarResultado(fd_m, cod_rta);
    close(fd_m);

    cleanWorkspaceFiles(2, exe, dat);
    liberador(2, exe, dat);
    liberarConfig(conf);
    log_destroy(logw);
    munmap(databin, dsize);
    exit(cod_rta);
}

static void liberarFILEs(int n, FILE *fs[n]) {
    for (n--; n > 0; n--)
        if (fs[n] != NULL) {
            fclose(fs[n]);
            fs[n] = NULL;
        }
}

static void liberarLines(int n, char *ls[n]) {
    for (n--; n > 0; n--)
        if (ls[n] != NULL) {
            free(ls[n]);
            ls[n] = NULL;
        }
}

static void liberarApareo(int nfiles, FILE *fs[nfiles], char *ls[nfiles - 1]) {
    liberarFILEs(nfiles, fs);
    liberarLines(nfiles - 1, ls);
}

static void cerrarSockets(int nfds, int *fds) {
    for (; nfds > 0; nfds--)
        close(fds[nfds]);
}

static void liberarBuffers(int n, char **buff) {
    return;
    for (; n > 0; n--)
        free(buff[n]);
}
