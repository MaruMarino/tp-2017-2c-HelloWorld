#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/log.h>
#include <funcionesCompartidas/mensaje.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "estructuras.h"
#include "conexion_master.h"
#include "conexion_fs.h"
#include "manejo_tabla_estados.h"

extern t_list *tabla_estado;
extern int job_id;

char *generar_nombre_temporal(int job, int nodo, int bloque)
{
	char *archivo_temporal = strdup("/temp/");
	char *sjob = string_itoa(job);
	char *snodo = string_itoa(nodo);
	char *sbloque = string_itoa(bloque);

	string_append(&archivo_temporal, "j");
	string_append(&archivo_temporal, sjob);
	string_append(&archivo_temporal, "n");
	string_append(&archivo_temporal, snodo);
	string_append(&archivo_temporal, "b");
	string_append(&archivo_temporal, sbloque);

	free(sjob);
	free(snodo);
	free(sbloque);

	return archivo_temporal;
}

void generar_estado(int master, int bloque, int nodo)
{
	t_estado *estado = malloc(sizeof (t_estado));
	char *arch_temp = generar_nombre_temporal(estado->job, nodo, bloque);

	estado->master = master;
	estado->job = job_id ++; //poner un flag no siempre va, pero no se cuando, dudo
	estado->etapa = TRANSFORMACION;
	estado->bloque = bloque;
	estado->nodo = nodo;
	estado->archivo_temporal = strdup("");
	string_append(&estado->archivo_temporal, arch_temp);
	estado->estado = EN_PROCESO;

	list_add(tabla_estado, estado);
	free(arch_temp);
}

void cambiar_estado(int master, int job, int nodo, int bloque, e_estado nuevo_estado)
{
	t_estado *estado = get_estado(master, job, nodo, bloque);
	estado->estado = nuevo_estado;

}

t_estado *get_estado(int master, int job, int nodo, int bloque)
{
	bool find_estado(t_estado *estado)
	{
		return (estado->master == master && estado->bloque == bloque
				&& estado->nodo == nodo && estado->job == job);
	}

	return (t_estado *)list_find(tabla_estado, (void *)find_estado);
}
