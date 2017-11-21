#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/string.h>

t_log *crear_archivo_log(char *nombre_cabecera, int imprimir, char *file)
{
	t_log *log = log_create(file,nombre_cabecera,imprimir, LOG_LEVEL_TRACE);
	log_info(log, "Se crea el archivo de log");
	return log;
}

void escribir_log(t_log *log, char *mensaje)
{
	log_info(log, mensaje);
}

void escribir_error_log(t_log *log, char *mensaje)
{
	log_error(log, mensaje);
}

void escribir_log_con_numero(t_log *log, char *mensaje, int un_numero)
{
	char *final = strdup(mensaje);
	char *num = string_itoa(un_numero);
	string_append(&final, num);
	log_info(log, final);
	free(final);
	free(num);
}

void escribir_log_error_con_numero(t_log *log, char *mensaje, int un_numero)
{
	char *final = strdup(mensaje);
	char *num = string_itoa(un_numero);
	string_append(&final, num);
	log_error(log, final);
	free(final);
	free(num);
}

void escribir_log_compuesto(t_log *log, char *mensaje, char *otro_mensaje)
{
	char *final = strdup("");
	string_append(&final, mensaje);
	string_append(&final, otro_mensaje);
	log_info(log, final);
	free(final);
}

void escribir_log_error_compuesto(t_log *log, char *mensaje, char *otro_mensaje)
{
	char *final = strdup("");
	string_append(&final, mensaje);
	string_append(&final, otro_mensaje);
	log_error(log, final);
	free(final);
}

void liberar_log(t_log *log)
{
	log_destroy(log);
}
