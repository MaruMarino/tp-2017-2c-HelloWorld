#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <commons/string.h>
#include <commons/log.h>

#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/serializacion.h>
#include <funcionesCompartidas/generales.h>

#include "nettingWorker.h"
#include "estructurasLocales.h"
#include "auxiliaresWorker.h"

#define maxline 0x10000

extern t_log *logw;

// todo: que responda al Master por caso exitoso o caso fallido
void subrutinaEjecutor(int sock_m){
	responderHandshake(sock_m);

	header head;
	char *msj, *exe_fname, *data_fname;
	int status;

	// filenames temporarios para programa y buffer de datos
	exe_fname  = string_itoa(getpid()); string_append(&exe_fname, ".exec");
	data_fname = string_itoa(getpid()); string_append(&data_fname, ".dat");

	msj = getMessage(sock_m, &head, &status);
	if (status == -1 || status == 0){
		log_error(logw, "Error en la recepcion del mensaje");
		exit(-1);
	}

	switch(head.codigo){
	case 1:
		log_trace(logw, "Un Master pide transformacion");

		t_info_trans *info_t = deserializar_info_trans(msj);

		if (crearArchivoBin(info_t->prog, info_t->size_prog, exe_fname) < 0){// ||
			//!crearArchivoData(info_t->bloque, (size_t) info_t->bytes_ocup, data_fname)){
			log_error(logw, "No se pudieron crear los archivos de trabajo.");
			liberador(4, msj, info_t, exe_fname, data_fname);
			exit(-1);
		}
		crearArchivoData(info_t->bloque, (size_t) info_t->bytes_ocup, data_fname);
		if (!makeCommandAndExecute(data_fname, exe_fname, info_t->file_out)){
			log_error(logw, "No se pudo completar correctamente la reduccion");
			liberador(4, msj, info_t->prog, info_t->file_out, info_t);
			exit(-1);
		}

		break;

	case RED_L:
		log_trace(logw, "Un Master pide reduccion local");

		t_info_redLocal *info_rl = deserializar_info_redLocal(msj);

		if (!crearArchivoBin(info_rl->prog, info_rl->size_prog, exe_fname)){
			log_error(logw, "No se pudo crear el ejecutable de reduccion.");
			liberarFnames(info_rl->files);
			liberador(5, msj, info_rl->prog, info_rl, exe_fname, data_fname);
			exit(-1);
		}

		if (aparearFiles(info_rl->files, data_fname) == -1){
			log_error(logw, "No se pudo aparear los archivos temporales");
			liberarFnames(info_rl->files);
			liberador(5, msj, info_rl->prog, info_rl, exe_fname, data_fname);
			exit(-1);
		}

		if (!makeCommandAndExecute(data_fname, exe_fname, info_rl->file_out)){
			log_error(logw, "No se pudo completar correctamente la reduccion");
			liberarFnames(info_rl->files);
			liberador(5, msj, info_rl->prog, info_rl, exe_fname, data_fname);
			exit(-1);
		}

		break;

	case RED_G:
		log_trace(logw, "Un Master pide reduccion global");

		t_info_redGlobal *info_rg = deserializar_info_redGlobal(msj);

		//apareoGlobal()
		//ejecucion()
		reproducirFiles(info_rg->nodos);

		break;
	}

	exit(0);
}

void subrutinaServidor(int sock_w){

	int ctl, ret;
	char *msj, *fname, *line;
	message *m;
	FILE *f;
	header head_cli;
	header head_serv = {.letra = 'W', .codigo = 11};
	size_t len = 0;
	size_t bufflen = maxline;
	char *fbuff = malloc(bufflen);

	if (responderHandshake(sock_w) == -1){
		log_error(logw, "No se pudo responder al Worker. Cierro conexion");
		close(sock_w);
		exit(-1);
	}

	ctl = 0;
	msj = getMessage(sock_w, &head_cli, &ctl);
	if (ctl == -1 || ctl == 0){
		log_error(logw, "Fallo conexion con Worker cliente, o se desconecto");
		close(sock_w);
		exit(-1);
	}

	fname = deserializar_FName(msj);
	if((f = fopen(fname, "r")) == NULL){
		log_error(logw, "Fallo fopen del path %s", fname);
		close(sock_w);
		exit(-1);
	}

	do {
		if (ctl <= 0) break;

		if ((ret = getline(&fbuff, &bufflen, f)) == -1 || bufflen > maxline){
			fbuff = "";
			if (feof(f)){
				head_serv.codigo = 12;
				ret = 0;
			}
			else if(ferror(f))
				head_serv.codigo = -13;
			else // tamanio de linea invalido
				head_serv.codigo = -14;
		}

		line = serializar_stream(fbuff, bufflen, &len);
		head_serv.sizeData = len;
		m = createMessage(&head_serv, line);
		enviar_message(sock_w, m, logw, &ctl);

		liberador(3, msj, m, line);
	} while(ret != -1 && (msj = getMessage(sock_w, &head_cli, &ctl)));

	if (ctl == 0){
		log_info(logw, "Se desconecto el Worker cliente prematuramente");
		ret = -2;

	} else {
		log_info(logw, "Fallo recepcion del mensaje del cliente");
		ret = -3;
	}

	close(sock_w);
	fclose(f);
	liberador(2, fbuff, fname);
	exit(ret);
}
