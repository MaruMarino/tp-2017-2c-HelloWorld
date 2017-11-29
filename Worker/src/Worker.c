#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>
#include <sys/mman.h>

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>

#include <funcionesCompartidas/logicaNodo.h>
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

t_log *logw;
t_conf *conf;
char *databin;
size_t dsize;
bool die = false;
struct sigaction sa;

/* El unico proposito de este handler es llamar waitpid() pa matar zombies */
void handleWorkerRet(int sig){
	int r;
	if (sig == SIGCHLD)
		while((r = waitpid(-1, &sig, WNOHANG)) != -1 && r != 0) ; // no-op
	else if (sig == SIGUSR1) die = true;
}

int main(int argc, char *argv[]){

	if (argc != 2){
		puts("Cantidad invalida de argumentos");
		return -1;
	}
	puts("Se crea archivo de log en /home/utnso/worker_log");
	logw = log_create("/home/utnso/worker_log", "Worker", true, LOG_LEVEL_TRACE);
	conf = cargarConfig(argv[1]);
	mostrarConfig(conf);

	pid_t mpid;
	header head;
	char *msj;
	int fd_proc, lis_fd, status;
	bool masterQuery = false;

	if ((databin = openDataBin(conf->ruta_databin, &dsize, 0)) == NULL){
		log_error(logw, "Fallo abrir el archivo databin. No se incia el Nodo");
		log_destroy(logw);
		liberarConfig(conf);
		return -1;
	}

	if ((lis_fd = makeListenSock(conf->puerto_worker, logw, &status)) < 0){
		log_error(logw, "No se logro bindear sobre puerto %s\n", conf->puerto_worker);
		log_destroy(logw);
		liberarConfig(conf);
		munmap(databin, dsize);
		return -1;
	}

	/* Seteo signal handler. Uso este especial por temas del accept() */
	memset(&sa, 0, sizeof sa);
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = handleWorkerRet;
	sigaction(SIGCHLD, &sa, 0);
	sigaction(SIGUSR1, &sa, 0);
	while(1){

		if (die) break;
		if ((fd_proc = aceptar_conexion_intr(lis_fd, &status)) == -1){
			if (status == EINTR) continue;
			log_error(logw, "No se pudo establecer una conexion!");
			continue;
		}
		log_info(logw, "Un Proceso quiere conectarse!");

		if ((msj = getMessageIntr(fd_proc, &head, &status)) == NULL){
			if (head.codigo == 0) goto verif;
			log_error(logw, "No se pudo recibir mensaje de %c", head.letra);
			close(fd_proc);
			continue;
		}
		free(msj); // no nos interesa el mensaje

		verif:
		if (verificarConexion(head, 'M', 0) == 0) // es un Master
			masterQuery = true;
		else if(verificarConexion(head, 'W', 0) == 0) // es un Worker
			masterQuery = false;
		else {
			log_info(logw, "Se trato de conectar un proceso invalido");
			close(fd_proc);
			continue;
		}

		log_trace(logw, "Se procede a forkear el proceso...");
		if ((mpid = fork()) == 0){ // soy proceso hijo
			close(lis_fd);
			if (masterQuery) // un Master quiere que le procese algo
				subrutinaEjecutor(fd_proc);

			// un Worker quiere que le sirva alguna informacion
			subrutinaServidor(fd_proc);

		} else if (mpid > 0){
			log_trace(logw, "Main Worker cierra socket contra Master...");
			close(fd_proc);

		} else {
			perror("Fallo fork(). error");
			log_error(logw, "Fallo llamada a fork! Esto es un error fatal!");
			close(fd_proc);
			break;
		}
	}

	log_trace(logw, "Se procede a liberar los ultimos recursos...");
	log_destroy(logw);
	liberarConfig(conf);
	munmap(databin, dsize);
	close(lis_fd);
	return 0;
}
