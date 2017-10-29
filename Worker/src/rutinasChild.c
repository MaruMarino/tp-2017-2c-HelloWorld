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
#include "configuracionWorker.h"

#define maxline 0x100000

extern t_log *logw;
extern t_conf *conf;

void subrutinaEjecutor(int sock_m){
	pid_t wp = getpid();
	log_trace(logw, "CHILD [%d]: Corriendo Subrutina Ejecutor", wp);

	header head;
	char *msj, *exe_fname, *data_fname, *fname;
	int status;
	int rta = FALLO;

	if (responderHandshake(sock_m) == -1){
		log_error(logw, "Fallo respuesta de handshake a Master");
		terminarEjecucion(sock_m, rta, conf);
	}

	msj = getMessage(sock_m, &head, &status);
	if (status == -1 || status == 0){
		log_error(logw, "Error en la recepcion del mensaje");
		free(msj);
		terminarEjecucion(sock_m, rta, conf);
	}

	// filenames temporarios para programa y buffer de datos
	exe_fname  = string_itoa(wp); string_append(&exe_fname, ".exec");
	data_fname = string_itoa(wp); string_append(&data_fname, ".dat");

	switch(head.codigo){
	case TRANSF:
		log_trace(logw, "CHILD [%d]: Ejecuta Transformacion", wp);

		t_info_trans *info_t = deserializar_info_trans(msj);

		if (crearArchivoBin(info_t->prog, info_t->size_prog, exe_fname) < 0 ||
			crearArchivoData(info_t->bloque, (size_t) info_t->bytes_ocup, data_fname) < 0){
			log_error(logw, "No se pudieron crear los archivos de trabajo.");
			liberador(6, msj, info_t->prog, info_t->file_out, info_t, exe_fname, data_fname);
			terminarEjecucion(sock_m, rta, conf);
		}
		// todo: mala validacion envias un 0 como success pero y entraba como error
		if (makeCommandAndExecute(data_fname, exe_fname, info_t->file_out)){
			log_error(logw, "No se pudo completar correctamente la reduccion");
			liberador(6, msj, info_t->prog, info_t->file_out, info_t, exe_fname, data_fname);
			terminarEjecucion(sock_m, rta, conf);
		}

		log_trace(logw, "CHILD [%d]: Finaliza Transformacion", wp);
		liberador(6, msj, info_t->prog, info_t->file_out, info_t, exe_fname, data_fname);
		break;

	case RED_L:
		log_trace(logw, "CHILD [%d]: Ejecuta Reduccion Local", wp);

		t_info_redLocal *info_rl = deserializar_info_redLocal(msj);
		//todo: mala validacion envias un 0 como success pero y entraba como error
		if (crearArchivoBin(info_rl->prog, info_rl->size_prog, exe_fname)){
			log_error(logw, "No se pudo crear el ejecutable de reduccion.");
			liberarFnames(info_rl->files);
			liberador(6, msj, info_rl->file_out, info_rl->prog, info_rl, exe_fname, data_fname);
			terminarEjecucion(sock_m, rta, conf);
		}

		if (aparearFiles(info_rl->files, data_fname) == -1){
			log_error(logw, "No se pudo aparear los archivos temporales");
			liberarFnames(info_rl->files);
			liberador(6, msj, info_rl->file_out, info_rl->prog, info_rl, exe_fname, data_fname);
			terminarEjecucion(sock_m, rta, conf);
		}
	//todo: mala validacion envias un 0 como success pero y entraba como error
		if (makeCommandAndExecute(data_fname, exe_fname, info_rl->file_out)){
			log_error(logw, "No se pudo completar correctamente la reduccion");
			liberarFnames(info_rl->files);
			liberador(6, msj, info_rl->file_out, info_rl->prog, info_rl, exe_fname, data_fname);
			terminarEjecucion(sock_m, rta, conf);
		}

		log_trace(logw, "CHILD [%d]: Finaliza Reduccion Local", wp);
		liberador(6, msj, info_rl->file_out, info_rl->prog, info_rl, exe_fname, data_fname);
		break;

	case RED_G:
		log_trace(logw, "CHILD [%d]: Ejecuta Reduccion Global", wp);

		t_info_redGlobal *info_rg = deserializar_info_redGlobal(msj);
//todo: mala validacion envias un 0 como success pero y entraba como error
		if (crearArchivoBin(info_rg->prog, info_rg->size_prog, exe_fname)){
			log_error(logw, "No se pudo crear el ejecutable de reduccion.");
			liberarInfoNodos(info_rg->nodos);
			liberador(6, msj, info_rg->prog, info_rg->file_out, info_rg, exe_fname, data_fname);
			terminarEjecucion(sock_m, rta, conf);
		}

		if (apareoGlobal(info_rg->nodos, info_rg->file_out) == -1){
			log_error(logw, "Fallo apareamiento de archivos");
			liberarInfoNodos(info_rg->nodos);
			liberador(6, msj, info_rg->prog, info_rg->file_out, info_rg, exe_fname, data_fname);
			terminarEjecucion(sock_m, rta, conf);
		}
//todo: mala validacion envias un 0 como success pero y entraba como error
		if (makeCommandAndExecute(data_fname, exe_fname, info_rg->file_out)){
			log_error(logw, "No se pudo completar correctamente la reduccion");
			liberarInfoNodos(info_rg->nodos);
			liberador(6, msj, info_rg->prog, info_rg->file_out, info_rg, exe_fname, data_fname);
			terminarEjecucion(sock_m, rta, conf);
		}

		log_trace(logw, "CHILD [%d]: Finaliza Reduccion Global", wp);
		liberador(6, msj, info_rg->prog, info_rg->file_out, info_rg, exe_fname, data_fname);
		break;

	case ALMAC:
		log_trace(logw, "CHILD [%d]: Ejecuta Almacenamiento Final", wp);

		fname = deserializar_FName(msj);

		if (almacenarFileEnFilesystem(conf->ip_fs, conf->puerto_fs, fname) == -1){
			log_error(logw, "No se logro almacenar %s en FileSystem", fname);
			liberador(4, msj, fname, exe_fname, data_fname);
			terminarEjecucion(sock_m, rta, conf);
		}

		log_trace(logw, "CHILD [%d]: Finaliza Almacenamiento Final", wp);
		liberador(4, msj, fname, exe_fname, data_fname);
		break;
	}

	terminarEjecucion(sock_m, OK, conf);
}

void subrutinaServidor(int sock_w){
	log_trace(logw, "CHILD [%d]: Corriendo Subrutina Servidor", getpid());
	liberarConfig(conf);

	int ctl, ret;
	char *msj, *fname, *line, *fbuff;
	message *m;
	FILE *f;
	header head_cli;
	header head_serv = {.letra = 'W', .codigo = 11};
	size_t len;
	size_t bufflen = maxline;

	if (responderHandshake(sock_w) == -1){
		log_error(logw, "No se pudo responder al Worker. Cierro conexion");
		close(sock_w);
		exit(-1);
	}

	ctl = 0;
	msj = getMessage(sock_w, &head_cli, &ctl);
	if (ctl == -1 || ctl == 0){
		log_error(logw, "Fallo conexion con Worker cliente o se desconecto");
		close(sock_w);
		exit(-1);
	}

	fname = deserializar_FName(msj);
	if((f = fopen(fname, "r")) == NULL){
		log_error(logw, "Fallo fopen del path %s", fname);
		close(sock_w);
		exit(-1);
	}

	fbuff = malloc(bufflen);
	do {
		if (ctl <= 0) break;

		if ((ret = getline(&fbuff, &bufflen, f)) == -1 || bufflen > maxline){
			strcpy(fbuff, "");
			if (feof(f)){
				head_serv.codigo = 12;
				ret = 0;
			}
			else if(ferror(f))
				head_serv.codigo = -13;
			else // tamanio de linea invalido
				head_serv.codigo = -14;
		}

		line = serializar_stream(fbuff, strlen(fbuff) + 1, &len);
		head_serv.sizeData = len;
		m = createMessage(&head_serv, line);
		enviar_message(sock_w, m, logw, &ctl);

		liberador(3, msj, m, line);
	} while(ret != -1 && (msj = getMessage(sock_w, &head_cli, &ctl)));

	if (ctl == 0){
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
