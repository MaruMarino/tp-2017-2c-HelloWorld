#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <wait.h>
#include <unistd.h>

#include <commons/config.h>
#include <commons/log.h>

#include <funcionesNet/funcionesNet.h>
#include "configuracionWorker.h"

/* MAX(A, B) es un macro que retorna el mayor entre dos argumentos */
#define MAX(A, B) ((A) > (B) ? (A) : (B))


int main(int argc, char *argv[]){

	if (argc != 2){
		puts("Cantidad invalida de argumentos");
		return -1;
	}
	struct conf_worker *conf = cargarConfig(argv[1]);

	char buf_recv[13];
	int sock_master, lis_master, ready_fd, fd, status;
	int max_fd = -1;

	fd_set masters_set, read_set;
	FD_ZERO(&masters_set);
	FD_ZERO(&read_set);

	if ((max_fd = lis_master = makeListenSock(conf->puerto_worker)) < 0){
		printf("No se logro bindear sobre puerto %s\n", conf->puerto_worker);
		return -1;

	} else if (listen(lis_master, 20) == -1){
		perror("Fallo listen. error");
		return -1;
	}

	FD_SET(lis_master, &masters_set);
	while(1){
		read_set = masters_set;

		if ((ready_fd = select(max_fd + 1, &read_set, NULL, NULL, NULL)) == -1){
			perror("Fallo de select(). error");
			break;
		}

		for (fd = 0; fd < max_fd + 1 && FD_ISSET(fd, &read_set); ++fd){

			if (fd == lis_master){
				puts("Un Master quiere conectarse!");
				if ((sock_master = makeCommSock(lis_master)) == -1){
					puts("No se pudo establecer la conexion con un Master!");
					continue;
				}
				max_fd = MAX(max_fd, sock_master);

				printf("Se conecto un Master, su socket: %d\n", sock_master);

			} else {

				puts("Un Master previamente conectado quiere decirnos algo!");
				if ((status = recv(fd, buf_recv, 13, 0)) == -1){
					perror("Fallo recepcion de mensaje! error:");
					break;

				} else if (status == 0){
					printf("Se desconecto el Master del socket %d\n", fd);
					FD_CLR(fd, &masters_set);
					continue;
				}

				printf("El mensaje recibido es: %s\n", buf_recv);
			}
		}
	}

	liberarConfig(conf);
	close(lis_master);
	return 0;
}
