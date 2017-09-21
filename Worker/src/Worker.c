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
#include <funcionesCompartidas/generales.h>

#include "configuracionWorker.h"
#include "auxiliaresWorker.h"
#include "estructurasLocales.h"

/* MAX(A, B) es un macro que retorna el mayor entre dos argumentos */
#define MAX(A, B) ((A) > (B) ? (A) : (B))

void subrutinaHijo(int sock_m);
int responderConexionMaster(int fd_proc);
int verificarConexionMaster(int fd_proc);

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
	int fd_proc, lis_fd, ready_fd, fd, status;
	int max_fd = -1;
	
	fd_set masters_set, read_set;
	FD_ZERO(&masters_set);
	FD_ZERO(&read_set);

	if ((max_fd = lis_fd = makeListenSock(conf->puerto_worker, logger, &status)) < 0){
		log_error(logger, "No se logro bindear sobre puerto %s\n", conf->puerto_worker);
		return -1;
	}

	FD_SET(lis_fd, &masters_set);
	while(1){
		read_set = masters_set;

		if ((ready_fd = select(max_fd + 1, &read_set, NULL, NULL, NULL)) == -1){
			perror("Fallo de select(). error");
			log_error(logger, "Fallo select()");
			break;
		}

		for (fd = 0; fd < max_fd + 1; ++fd){
		if (FD_ISSET(fd, &read_set)){

			if (fd == lis_fd){

				log_info(logger, "Un Proceso quiere conectarse!");
				if ((fd_proc = aceptar_conexion(lis_fd, logger, &status)) == -1){
					log_error(logger, "No se pudo establecer la conexion con un Master!");
					continue;
				}

				if ((status = verificarConexionMaster(fd_proc)) < 0){
					log_info(logger, "Se rechaza la conexion en %d", fd_proc);
					close(fd_proc);
					continue;

				} else if (responderConexionMaster(fd_proc) < 0){
					log_info(logger, "No se logra responder al Master");
					close(fd_proc);
					continue;
				}

				if ((mpid = fork()) == 0){
					subrutinaHijo(fd_proc);

				} else if (mpid > 0){
					close(fd_proc);
					// funcion_padre

				} else {
					perror("Fallo fork(). error");
					log_error(logger, "Fallo llamada a fork! Esto es un error fatal!");
					return EXIT_FAILURE;
				}

				log_info(logger, "Se conecto un Master, su socket: %d\n", fd_proc);

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

		}} // cierro for() y FD_ISSET
	}

	liberarConfig(conf);
	close(lis_fd);
	return 0;
}

void subrutinaHijo(int sock_m){

	header head;
	char *cmd, *msj, *exe_fname, *data_fname;

	msj = getMessage(sock_m, &head);
	switch(head.codigo){
	case TRANSF:
		log_trace(logger, "Un Master pide transformacion");

		t_info_trans *info = deserializar_info_trans(msj);

		// filenames temporarios para programa y bloque de datos
		exe_fname  = string_itoa(getpid()); string_append(&exe_fname, ".exec");
		data_fname = string_itoa(getpid()); string_append(&data_fname, ".dat");

		if (!crearArchivoBin(info, exe_fname) ||
			!crearArchivoData(0, 0, data_fname)){

			log_error(logger, "No se pudieron crear los archivos de trabajo.");
			liberador(4, msj, info, exe_fname, data_fname);
			exit(-1);
		}

		cmd = crearComando(6, "cat ", data_fname, "|", exe_fname,
				" | sort -dib > ", info->file_out);
		if (!cmd){
			log_error(logger, "Fallo la creacion del comando a ejecutar.");
			liberador(5, msj, info, exe_fname, data_fname, cmd);
			exit(-1);
		}

		if (!system(cmd)){
			log_error(logger, "Llamada a system() con comando %s fallo.", cmd);
			liberador(5, msj, info, exe_fname, data_fname, cmd);
			exit(-1);
		}

		// todo: que responda al Master
		break;

	case RED_L:
		log_trace(logger, "Un Master pide reduccion local");

		char **lista_de_temporales = NULL; // se recibe del Master
		aparearFiles(3, lista_de_temporales);

		break;

	case RED_G:
		log_trace(logger, "Un Master pide reduccion global");

		break;
	}

	exit(0);
}

int responderConexionMaster(int fd_proc){

	int ctl;
	message *msj;
	char *data = "Recibido - forkeo un Worker nuevo";
	header head = {.letra = 'W', .codigo = 0, .sizeData = strlen(data)};

	msj = createMessage(&head, data);
	if (enviar_message(fd_proc, msj, logger, &ctl) < 0){
		free(msj);
		return -1;
	}

	free(msj);
	return 0;
}

int verificarConexionMaster(int fd_proc){

	header head;
	char *data;

	if ((data = getMessage(fd_proc, &head)) == NULL){
		log_error(logger, "Fallo recepcion de handshake");
		return -1;
	}

	if (head.letra != 'M'){
		log_info(logger, "La conexion recibida no es de Master: %c", head.letra);
		return -2;

	} else if (head.codigo != 0){
		log_info(logger, "La conexion recibida no es handshake: %d", head.codigo);
		return -3;
	}

	return 0;
}
