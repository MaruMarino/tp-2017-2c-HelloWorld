#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <wait.h>
#include <unistd.h>

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>

#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/log.h>
#include "configuracionWorker.h"
#include "auxiliaresWorker.h"

/* MAX(A, B) es un macro que retorna el mayor entre dos argumentos */
#define MAX(A, B) ((A) > (B) ? (A) : (B))

t_log *logger;

int main(int argc, char *argv[]){

	if (argc != 2){
		puts("Cantidad invalida de argumentos");
		return -1;
	}
	logger = crear_archivo_log("Worker", true, "/home/utnso/conf/worker_log");

	struct conf_worker *conf = cargarConfig(argv[1]);
	mostrarConfig(conf);

	pid_t mpid;
	char buf_recv[13];
	int sock_master, lis_master, ready_fd, fd, status;
	int max_fd = -1;
	
	fd_set masters_set, read_set;
	FD_ZERO(&masters_set);
	FD_ZERO(&read_set);

	if ((max_fd = lis_master = makeListenSock(conf->puerto_worker)) < 0){
		log_error(logger, "No se logro bindear sobre puerto %s\n", conf->puerto_worker);
		return -1;

	} else if (listen(lis_master, 20) == -1){
		perror("Fallo listen. error");
		log_error(logger, "Fallo listen() sobre socket %d", lis_master);
		return -1;
	}

	FD_SET(lis_master, &masters_set);
	while(1){
		read_set = masters_set;

		if ((ready_fd = select(max_fd + 1, &read_set, NULL, NULL, NULL)) == -1){
			perror("Fallo de select(). error");
			log_error(logger, "Fallo select()");
			break;
		}

		for (fd = 0; fd < max_fd + 1 && FD_ISSET(fd, &read_set); ++fd){

			if (fd == lis_master){

				log_info(logger, "Un Master quiere conectarse!");
				if ((sock_master = makeCommSock(lis_master)) == -1){
					log_error(logger, "No se pudo establecer la conexion con un Master!");
					continue;
				}
				max_fd = MAX(max_fd, sock_master);

				if ((mpid = fork()) == 0){
					// funcion_hijo
				} else if (mpid > 0){
					// funcion_padre

				} else {
					perror("Fallo fork(). error");
					log_error(logger, "Fallo llamada a fork! Esto es un error fatal!");
					return EXIT_FAILURE;
				}

				log_info(logger, "Se conecto un Master, su socket: %d\n", sock_master);

			} else {

				log_info(logger, "Un Master previamente conectado quiere decirnos algo!");
				if ((status = recv(fd, buf_recv, 13, 0)) == -1){
					perror("Fallo recepcion de mensaje! error:");
					log_error(logger, "Fallo recepcion de mensaje!");
					break;

				} else if (status == 0){
					log_info(logger, "Se desconecto el Master del socket %d\n", fd);
					FD_CLR(fd, &masters_set);
					continue;
				}

				log_info(logger, "El mensaje recibido es: %s\n", buf_recv);
			}
		}
	}

	liberarConfig(conf);
	close(lis_master);
	return 0;
}
