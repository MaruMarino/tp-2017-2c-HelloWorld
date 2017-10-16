#include <stdlib.h>
#include <commons/log.h>

#include <funcionesCompartidas/funcionesNet.h>

#include "nettingWorker.h"

extern t_log *logw;

int responderHandshake(int fd_proc){

	int ctl;
	header head = {.letra = 'W', .codigo = 0, .sizeData = 0};
	message *msj = createMessage(&head, NULL);

	if (enviar_message(fd_proc, msj, logw, &ctl) < 0){
		free(msj);
		return -1;
	}

	free(msj);
	return 0;
}

int verificarConexion(header head, char proc, int cod){

	if (head.letra != proc){
		log_info(logw, "El emisor no es %c! Se recibio: %c", head.letra);
		return -1;

	} else if (head.codigo != cod){
		log_info(logw, "El codigo no es %d! Se recibio: %d", head.codigo);
		return -2;
	}

	log_info(logw, "El proceso identificado es %c con codigo %d", proc, cod);
	return 0;
}

int realizarHandshake(int fd_proc, char proc_expected){

	int ctl;
	header head = {.letra = 'W', .codigo = 0, .sizeData = 1};
	message *msj = createMessage(&head, "");

	if (enviar_message(fd_proc, msj, logw, &ctl) < 0){
		log_trace(logw, "Fallo realizarHandshake con %d", fd_proc);
		return -1;
	}

	free(getMessage(fd_proc, &head, &ctl));
	if (ctl == -1){
		log_trace(logw, "Fallo realizarHandshake con %d", fd_proc);
		return -1;

	} else if (ctl == 0){
		log_trace(logw, "El proceso en socket %d se desconecto", fd_proc);
		return -1;

	} else if (verificarConexion(head, proc_expected, 0) < 0){
		log_trace(logw, "Fallo realizarHandshake con %d", fd_proc);
		return -1;
	}

	return 0;
}

void enviarResultado(int fd_m, int cod_rta){

	int ctl;
	header head = {.letra = 'W', .codigo = cod_rta, .sizeData = 1};
	message *msj = createMessage(&head, "");

	enviar_message(fd_m, msj, logw, &ctl);
}
