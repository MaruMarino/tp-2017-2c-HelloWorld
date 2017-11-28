#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <commons/string.h>
#include <commons/log.h>

#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/serializacion.h>
#include <funcionesCompartidas/generales.h>
#include <funcionesCompartidas/logicaNodo.h>

#include "nettingWorker.h"
#include "estructurasLocales.h"
#include "auxiliaresWorker.h"
#include "configuracionWorker.h"

#define maxline 0x100000

extern t_log *logw;
extern t_conf *conf;

void subrutinaEjecutor(int sock_m) {
    pid_t wp = getpid();
    log_trace(logw, "CHILD [%d]: Corriendo Subrutina Ejecutor", wp);

    header head;
    char *msj, *exe_fname, *data_fname, *fname;
    int status;
    int rta = FALLO;

    // filenames temporarios para programa y buffer de datos
    exe_fname = string_itoa(wp);
    string_append(&exe_fname, ".exec");
    data_fname = string_itoa(wp);
    string_append(&data_fname, ".dat");

    if (responderHandshake(sock_m) == -1) {
        log_error(logw, "Fallo respuesta de handshake a Master");
        terminarEjecucion(sock_m, rta, conf, exe_fname, data_fname);
    }

    msj = getMessage(sock_m, &head, &status);
    if (status == -1 || status == 0) {
        log_error(logw, "Error en la recepcion del mensaje");
        free(msj);
        terminarEjecucion(sock_m, rta, conf, exe_fname, data_fname);
    }

    switch (head.codigo) {
        case TRANSF:
            log_trace(logw, "CHILD [%d]: Ejecuta Transformacion", wp);

            t_info_trans *info_t = deserializar_info_trans(msj);

            if (crearArchivoBin(info_t->prog, info_t->size_prog, exe_fname) < 0 ||
                crearArchivoData(info_t->bloque, (size_t) info_t->bytes_ocup, data_fname)) {
                log_error(logw, "No se pudieron crear los archivos de trabajo.");
                liberador(4, msj, info_t->prog, info_t->file_out, info_t);
                terminarEjecucion(sock_m, rta, conf, exe_fname, data_fname);
            }

            if (makeCommandAndExecute(data_fname, exe_fname, info_t->file_out) < 0) {
                log_error(logw, "No se pudo completar correctamente la reduccion");
                liberador(4, msj, info_t->prog, info_t->file_out, info_t);
                terminarEjecucion(sock_m, rta, conf, exe_fname, data_fname);
            }

            log_trace(logw, "CHILD [%d]: Finaliza Transformacion", wp);
            liberador(4, msj, info_t->prog, info_t->file_out, info_t);
            break;

        case RED_L:
            log_trace(logw, "CHILD [%d]: Ejecuta Reduccion Local", wp);

            t_info_redLocal *info_rl = deserializar_info_redLocal(msj);

            if (crearArchivoBin(info_rl->prog, info_rl->size_prog, exe_fname) < 0) {
                log_error(logw, "No se pudo crear el ejecutable de reduccion.");
                liberarFnames(info_rl->files);
                liberador(4, msj, info_rl->file_out, info_rl->prog, info_rl);
                terminarEjecucion(sock_m, rta, conf, exe_fname, data_fname);
            }

            if (aparearFiles(info_rl->files, data_fname) == -1) {
                log_error(logw, "No se pudo aparear los archivos temporales");
                liberarFnames(info_rl->files);
                liberador(4, msj, info_rl->file_out, info_rl->prog, info_rl);
                terminarEjecucion(sock_m, rta, conf, exe_fname, data_fname);
            }

            if (makeCommandAndExecute(data_fname, exe_fname, info_rl->file_out) < 0) {
                log_error(logw, "No se pudo completar correctamente la reduccion");
                liberarFnames(info_rl->files);
                liberador(4, msj, info_rl->file_out, info_rl->prog, info_rl);
                terminarEjecucion(sock_m, rta, conf, exe_fname, data_fname);
            }

            log_trace(logw, "CHILD [%d]: Finaliza Reduccion Local", wp);
            liberarFnames(info_rl->files);
            liberador(4, msj, info_rl->file_out, info_rl->prog, info_rl);
            break;

        case RED_G:
            log_trace(logw, "CHILD [%d]: Ejecuta Reduccion Global", wp);

            t_info_redGlobal *info_rg = deserializar_info_redGlobal(msj);

            if (crearArchivoBin(info_rg->prog, info_rg->size_prog, exe_fname) < 0) {
                log_error(logw, "No se pudo crear el ejecutable de reduccion.");
                liberarInfoNodos(info_rg->nodos);
                liberador(4, msj, info_rg->prog, info_rg->file_out, info_rg);
                terminarEjecucion(sock_m, rta, conf, exe_fname, data_fname);
            }

            if (apareoGlobal(info_rg->nodos, data_fname) == -1) {
                log_error(logw, "Fallo apareamiento de archivos");
                liberarInfoNodos(info_rg->nodos);
                liberador(4, msj, info_rg->prog, info_rg->file_out, info_rg);
                terminarEjecucion(sock_m, rta, conf, exe_fname, data_fname);
            }

            if (makeCommandAndExecute(data_fname, exe_fname, info_rg->file_out) < 0) {
                log_error(logw, "No se pudo completar correctamente la reduccion");
                liberarInfoNodos(info_rg->nodos);
                liberador(4, msj, info_rg->prog, info_rg->file_out, info_rg);
                terminarEjecucion(sock_m, rta, conf, exe_fname, data_fname);
            }

            log_trace(logw, "CHILD [%d]: Finaliza Reduccion Global", wp);
            liberarInfoNodos(info_rg->nodos);
            liberador(4, msj, info_rg->prog, info_rg->file_out, info_rg);
            break;

        case ALMAC:
        	log_trace(logw, "CHILD [%d]: Ejecuta Almacenamiento Final", wp);

        	char *yamafn;
        	t_file *file;
        	fname = deserializar_FName2(msj, &yamafn);
        	log_trace(logw, "Se cargara el file %s para almacenarse como %s", fname, yamafn);

        	if ((file = cargarFile(fname, yamafn)) == NULL) {
        		log_error(logw, "Fallo cargar el t_file %s para enviar", fname);
        		liberador(3, msj, fname, yamafn);
        		terminarEjecucion(sock_m, rta, conf, exe_fname, data_fname);
        	}

        	if (almacenarFileEnFilesystem(conf->ip_fs, conf->puerto_fs, file) != 0) {
        		log_error(logw, "No se logro almacenar %s en FileSystem", file->fname);
        		liberador(6, msj, fname, yamafn, file->data, file->fname, file);
        		terminarEjecucion(sock_m, rta, conf, exe_fname, data_fname);
        	}
        	log_info(logw, "Se almaceno satisfactoriamente %s en FileSystem", file->fname);

        	log_trace(logw, "CHILD [%d]: Finaliza Almacenamiento Final", wp);
        	liberador(6, msj, fname, yamafn, file->data, file->fname, file);
        	break;
    }


    terminarEjecucion(sock_m, OK, conf, exe_fname, data_fname);
}

void subrutinaServidor(int sock_w) {
    log_trace(logw, "CHILD [%d]: Corriendo Subrutina Servidor", getpid());
    liberarConfig(conf);

    int ctl, ret;
    char *msj, *fname, *line, *fbuff;
    message *msg;
    FILE *f;
    header head_cli;
    header head_serv = {.letra = 'W', .codigo = FILE_SND};
    size_t len;
    size_t bufflen = maxline;

    if (responderHandshake(sock_w) == -1) {
        log_error(logw, "No se pudo responder al Worker. Cierro conexion");
        close(sock_w);
        exit(-1);
    }

    ctl = 0;
    msj = getMessage(sock_w, &head_cli, &ctl);
    if (ctl == -1 || ctl == 0) {
        log_error(logw, "Fallo conexion con Worker cliente o se desconecto");
        close(sock_w);
        free(msj);
        exit(-1);
    }

    fname = deserializar_FName(msj);
    if ((f = fopen(fname, "r")) == NULL) {
        log_error(logw, "Fallo fopen del path %s", fname);
        close(sock_w);
        exit(-1);
    }

    fbuff = malloc(bufflen);
    do {
        if (ctl <= 0) break;

        if ((ret = getline(&fbuff, &bufflen, f)) == -1 || bufflen > maxline) {
            strcpy(fbuff, "");
            if (feof(f)) {
                head_serv.codigo = APAR_EOF;
                ret = 0;
            } else if (ferror(f))
                head_serv.codigo = APAR_ERR;
            else // tamanio de linea invalido
                head_serv.codigo = APAR_INV;
        }

        line = serializar_stream(fbuff, strlen(fbuff) + 1, &len);
        head_serv.sizeData = len;
        msg = createMessage(&head_serv, line);
        enviar_message(sock_w, msg, logw, &ctl);

        liberador(3, msj, msg->buffer, msg, line);
    } while (ret != -1 && (msj = getMessage(sock_w, &head_cli, &ctl)));

    if (ret == 0) {
        log_info(logw, "Se desconecta el Worker cliente de forma esperada");

    } else if (ctl == 0) {
        log_error(logw, "Se desconecto el Worker cliente prematuramente");
        ret = -2;

    } else {
        log_error(logw, "Fallo recepcion del mensaje del cliente");
        ret = -3;
    }

    log_trace(logw, "CHILD [%d]: Finalizando Subrutina Servidor", getpid());
    close(sock_w);
    fclose(f);
    liberador(2, fbuff, fname);
    exit(ret);
}
