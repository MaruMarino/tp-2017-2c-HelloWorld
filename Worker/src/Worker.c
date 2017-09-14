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
#include <funcionesCompartidas/mensaje.h>
#include <funcionesCompartidas/log.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/serializacion.h>

#include "configuracionWorker.h"
#include "auxiliaresWorker.h"
#include "estructurasLocales.h"

/* MAX(A, B) es un macro que retorna el mayor entre dos argumentos */
#define MAX(A, B) ((A) > (B) ? (A) : (B))

void subrutinaHijo(int sock_m);

t_log *logger;
int const headsize = 13;
char* const sort_cmd = "sort -dib";

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

	if ((max_fd = lis_master = makeListenSock(conf->puerto_worker, logger, &status)) < 0){
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
				if ((sock_master = aceptar_conexion(lis_master, logger, &status)) == -1){
					log_error(logger, "No se pudo establecer la conexion con un Master!");
					continue;
				}

				// verificarConexionMaster() --> lo dejo o lo desecho

				if ((mpid = fork()) == 0){
					subrutinaHijo(sock_master);

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

void subrutinaHijo(int sock_m){

	int status;
	char *cmd;
	char *msj = recibir(sock_m, logger, &status);
	char *exe_fname = string_itoa(getpid()); // filename temporario para prog
	const char *pipeSortAndOut = "| sort -dib >";

	switch(get_codigo(msj)){
	case TRANSF:
		log_trace(logger, "Un Master pide transformacion");

		t_info_trans *info = deserializar_info_trans(msj + 13);

		if (!crearArchivoBin(info, exe_fname)){
			log_error(logger, "No se pudo crear el transformador. Se aborta!");
			exit(-1);
		}

		//obtenerBloque(info->bloque, info->bytes_ocup) -> getBloque()?

		cmd = crearComando(3, exe_fname, pipeSortAndOut, info->file_out);
		if (!cmd){
			log_error(logger, "Fallo creacion del comando. Detenemos proceso");
			exit(-1);
		}

		if (!system(cmd)){
			log_error(logger, "Llamada a system() con comando %s fallo!", cmd);
			exit(-1);
		}

		break;

	case RED_L:
		log_trace(logger, "Un Master pide reduccion local");
		break;

	case RED_G:
		log_trace(logger, "Un Master pide reduccion global");
		break;
	}

	free(msj);
	free(exe_fname);
	exit(0);
}
