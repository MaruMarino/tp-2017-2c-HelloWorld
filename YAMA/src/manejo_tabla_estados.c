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
//extern int job_id;

char *generar_nombre_temporal(int job, char *nodo, int bloque)
{
	char *archivo_temporal = strdup("/tmp/tl_");
	char *sjob = string_itoa(job);
	char *sbloque = string_itoa(bloque);

	string_append(&archivo_temporal, "m");
	string_append(&archivo_temporal, sjob);
	string_append(&archivo_temporal, "n");
	string_append(&archivo_temporal, nodo);
	string_append(&archivo_temporal, "b");
	string_append(&archivo_temporal, sbloque);

	free(sjob);
	free(sbloque);

	return archivo_temporal;
}

int calcular_cantidad(int master, char *nodo)
{
	int cant = 1;
	int i;
	int sz = list_size(tabla_estado);

	for(i=0; i < sz; i++)
	{
		t_estado * est = list_get(tabla_estado, i);
		if(!strcmp(est->nodo,nodo) && est->master == master && est->etapa == TRANSFORMACION)
		{
			cant ++;
		}
	}
	for(i=0; i < sz; i++)
	{
		t_estado * est = list_get(tabla_estado, i);
		if(!strcmp(est->nodo,nodo) && est->master == master && est->etapa == TRANSFORMACION)
		{
			est->cant_bloques_nodo = cant;
		}
	}

	return cant;
}

t_estado *generar_estado(int master, int bloque, char *nodo, char *nodo2, int bloque2, int bytes)
{
	t_estado *estado = malloc(sizeof (t_estado));
	char *arch_temp = generar_nombre_temporal(master, nodo, bloque);

	estado->master = master;
	estado->job = master; //chequear
	estado->etapa = TRANSFORMACION;
	estado->bloque = bloque;
	estado->nodo = strdup(nodo);
	estado->archivo_temporal = strdup("");
	string_append(&estado->archivo_temporal, arch_temp);
	estado->estado = EN_PROCESO;
	estado->cant_bloques_nodo = calcular_cantidad(master, nodo);
	estado->copia_disponible = true;
	estado->nodo_copia = nodo2;
	estado->bloque_copia = bloque2;
	estado->bytes = bytes;

	list_add(tabla_estado, estado);
	free(arch_temp);
	return estado;
}

void cambiar_estado(int master, char *nodo, int bloque, e_estado nuevo_estado, char *nom)
{
	t_estado *estado = get_estado(master, nodo, bloque);
	estado->estado = nuevo_estado;
	estado->archivo_temporal = nom;
}

t_estado *get_estado(int master, char *nodo, int bloque)
{
	bool find_estado(t_estado *estado)
	{
		return (estado->master == master && estado->bloque == bloque
				&& !(strcmp(estado->nodo, nodo)));
	}

	return (t_estado *)list_find(tabla_estado, (void *)find_estado);
}
