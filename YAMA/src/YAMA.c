#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "estructuras.h"
#include "conexion_master.h"
#include "conexion_fs.h"
#include "clocks.h"

t_log *yama_log;
t_configuracion *config;
t_list *tabla_estado; //formado por t_estado
t_list *masters; //formado por t_master, o usar diccionario?
t_list *workers; //formado por t_worker
int master_id = 1;
int job_id = 1;
char *path;

void leer_configuracion();
void liberar_memoria();
void inicializar_variables();
int conectar_fs();
void crear_socket_servidor();
void reconfiguracion();

int main(int argc, char **argv)
{
	yama_log = crear_archivo_log("YAMA",true,"/home/utnso/conf/yama_log");

	path = argv[1];
	inicializar_variables();
	leer_configuracion();
	//free(path);

	signal(SIGUSR1, reconfiguracion);

	int cont = conectar_fs();
	if(cont == 0)
		return EXIT_FAILURE;
	crear_socket_servidor();
	manejo_conexiones();


	return EXIT_SUCCESS;
}

void inicializar_variables()
{
	config = malloc (sizeof (t_configuracion));
	config->algortimo_bal = strdup("");
	config->fs_ip = strdup("");
	config->fs_puerto = strdup("");
	config->yama_puerto = strdup("");

	tabla_estado = list_create();
	masters = list_create();
	workers = list_create();
}

void liberar_memoria()
{
	free(config->algortimo_bal);
	free(config->yama_puerto);
	free(config->fs_ip);
	free(config->fs_puerto);
	free(config);
	list_destroy(tabla_estado);
	list_destroy(masters);
	list_destroy(workers);
}

void leer_configuracion()
{
	escribir_log(yama_log, "Leyendo configuracion");

	t_config *configuracion = config_create(path);

	string_append(&config->yama_puerto, config_get_string_value(configuracion, "YAMA_PUERTO"));
	string_append(&config->algortimo_bal, config_get_string_value(configuracion, "ALGORITMO_BALANCEO"));
	config->retardo_plan = config_get_int_value(configuracion, "RETARDO_PLANIFICACION");
	config->base = config_get_int_value(configuracion, "BASE");
	string_append(&config->fs_ip, config_get_string_value(configuracion, "FS_IP"));
	string_append(&config->fs_puerto, config_get_string_value(configuracion, "FS_PUERTO"));

	config_destroy(configuracion);
}

int conectar_fs()
{
	int control = 0;
	int cont;
	config->socket_fs = establecerConexion(config->fs_ip, config->fs_puerto, yama_log, &control);
	if(control<0)
	{
		escribir_error_log(yama_log, "Error conectandose a FS");
	}
	else
	{
		escribir_log(yama_log, "Conectado a FS");
		header head, head2;
		head2.codigo = 0;
		head2.letra = 'Y';
		head2.sizeData = 0;

		message *mensaje = createMessage(&head2, "");
		enviar_messageIntr(config->socket_fs, mensaje, yama_log, &control);

		char *rta = getMessageIntr(config->socket_fs, &head, &control);
		if (head.codigo == 0)
		{
			escribir_log(yama_log, "YAMA rechazado por FileSystem :(");
			cont = 0;

		}else if(head.codigo == 2)
		{
			escribir_log(yama_log, "Conectado a File System :D");
			armar_workers(rta);
			cont = 1;
		}else
		{
			escribir_error_log(yama_log, "No comprendo el mensaje recibido");
			cont = 0;
		}
		free(mensaje->buffer);
		free(mensaje);
		free(rta);
	}
	return cont;
}

void crear_socket_servidor()
{
	int control = 0;
	config->server_ = makeListenSock(config->yama_puerto, yama_log, &control);
}

void reconfiguracion(int signal_)
{
	if (signal_ == SIGUSR1)
	{
		puts("empiezo!");
		t_config *configuracion = config_create(path);

		string_append(&config->algortimo_bal, config_get_string_value(configuracion, "ALGORITMO_BALANCEO"));
		config->retardo_plan = config_get_int_value(configuracion, "RETARDO_PLANIFICACION");

		config_destroy(configuracion);
		puts("termino!");
	}
}
