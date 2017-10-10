#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>

#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/mensaje.h>
#include <funcionesCompartidas/log.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/serializacion.h>
#include <funcionesCompartidas/generales.h>

#include "configuracionWorker.h"
#include "auxiliaresWorker.h"
#include "estructurasLocales.h"
#include "nettingWorker.h"
#include "rutinasChild.h"

#define maxline 0x100000 // 1 MiB

t_log *logw;

/* El unico proposito de este handler es llamar waitpid() pa matar zombies */
void handleWorkerRet(int sig){
	while(waitpid(-1, &sig, WNOHANG) != -1) ; // no-op
}

int main(int argc, char *argv[]){

	if (argc != 2){
		puts("Cantidad invalida de argumentos");
		return -1;
	}
	logw = crear_archivo_log("Worker", true, "/home/utnso/conf/worker_log");

	struct conf_worker *conf = cargarConfig(argv[1]);
	mostrarConfig(conf);

	pid_t mpid;
	header head;
	char *msj;
	int fd_proc, lis_fd, status;
	bool masterQuery = false;
	
	fd_set masters_set, read_set;
	FD_ZERO(&masters_set);
	FD_ZERO(&read_set);

	signal(SIGCHLD, handleWorkerRet);

	if ((lis_fd = makeListenSock(conf->puerto_worker, logw, &status)) < 0){
		log_error(logw, "No se logro bindear sobre puerto %s\n", conf->puerto_worker);
		return -1;
	}

	FD_SET(lis_fd, &masters_set);
	while(1){
		read_set = masters_set;

		try_select:
		if (select(lis_fd + 1, &read_set, NULL, NULL, NULL) == -1){
			if (errno == EINTR) goto try_select;
			perror("Fallo de select(). error");
			log_error(logw, "Fallo select()");
			break;
		}

		log_info(logw, "Un Proceso quiere conectarse!");
		if ((fd_proc = aceptar_conexion(lis_fd, logw, &status)) == -1){
			log_error(logw, "No se pudo establecer una conexion!");
			continue;
		}
		if ((msj = getMessageIntr(fd_proc, &head, &status)) == NULL){
			log_error(logw, "No se pudo recibir mensaje de %c", head.letra);
			close(fd_proc);
			continue;
		}
		free(msj); // no nos interesa el mensaje

		if (verificarConexion(head, 'M', 0) == 0) // es un Master
			masterQuery = true;
		else if(verificarConexion(head, 'W', 0) == 0) // es un Worker
			masterQuery = false;
		else {
			log_info(logw, "Se trato de conectar un proceso invalido");
			close(fd_proc);
			continue;
		}

		if ((mpid = fork()) == 0){ // soy proceso hijo
			if (masterQuery) // un Master quiere que le procese algo
				subrutinaEjecutor(fd_proc);

			// un Worker quiere que le sirva alguna informacion
			subrutinaServidor(fd_proc);

		} else if (mpid > 0){
			close(fd_proc);

		} else {
			perror("Fallo fork(). error");
			log_error(logw, "Fallo llamada a fork! Esto es un error fatal!");
			break;
		}
	}

	liberarConfig(conf);
	close(lis_fd);
	return 0;
}
