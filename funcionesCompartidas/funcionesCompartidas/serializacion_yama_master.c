#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>

#include "serializacion_yama_master.h"
#include "estructuras.h"

char *serializar_nodo(t_nodo *nodo, size_t *len)
{
	size_t len_nodo =(size_t) string_length(nodo->nodo) + 1;
	size_t len_ip =(size_t) string_length(nodo->ip) + 1;
	char *nodo_ser = malloc(sizeof(t_nodo) + (size_t) len_nodo + (size_t) len_ip);

	*len = 0;
	memcpy(nodo_ser, &len_nodo, 4);
	*len += 4;
	memcpy(nodo_ser + *len, nodo->nodo, len_nodo);
	*len += len_nodo;
	memcpy(nodo_ser + *len, &len_ip, 4);
	*len += 4;
	memcpy(nodo_ser + *len, nodo->ip, len_ip);
	*len += len_ip;
	memcpy(nodo_ser + *len, &nodo->puerto, 4);
	*len += 4;
	memcpy(nodo_ser + *len, &nodo->sizeDatabin, sizeof(int));
	*len += sizeof(int);

	return nodo_ser;
}

t_nodo *deserializar_nodo (char *nodo_ser, size_t *len)
{
	t_nodo *nodo = malloc (sizeof (t_nodo));
	size_t len_nodo;
	size_t len_ip;

	*len = 0;
	memcpy(&len_nodo, nodo_ser, 4);
	*len += 4;
	nodo->nodo = malloc(len_nodo);
	memcpy(nodo->nodo, nodo_ser + *len, len_nodo);
	*len += len_nodo;
	memcpy(&len_ip, nodo_ser + *len, 4);
	*len += 4;
	nodo->ip = malloc(len_ip);
	memcpy(nodo->ip, nodo_ser + *len, len_ip);
	*len += len_ip;
	memcpy(&nodo->puerto, nodo_ser + *len, 4);
	*len += 4;
	memcpy(&nodo->sizeDatabin,nodo_ser + *len, sizeof(int));
	*len += sizeof(int);

	return nodo;
}

char *serializar_transformacion(t_transformacion *tran, size_t *len)
{
	size_t len_nodo;
	size_t len_temporal =(size_t) string_length(tran->temporal) + 1;
	char *nodo = serializar_nodo(tran->nodo, &len_nodo);
	char *tran_ser = malloc(sizeof(t_transformacion) + len_temporal + len_nodo);

	*len = 0;
	memcpy(tran_ser, &len_nodo, 4);
	*len += 4;
	memcpy(tran_ser + *len, nodo, len_nodo);
	*len += len_nodo;
	memcpy(tran_ser + *len, &tran->bloque, 4);
	*len += 4;
	memcpy(tran_ser + *len, &tran->bytes, 4);
	*len += 4;
	memcpy(tran_ser + *len, &len_temporal, 4);
	*len += 4;
	memcpy(tran_ser + *len, tran->temporal, len_temporal);
	*len += len_temporal;

	return tran_ser;
}

t_transformacion *deserializar_transformacion(char *tran)
{
	t_transformacion *tran_des = malloc(sizeof (t_transformacion));
	size_t len_temp;
	size_t len_nodo;
	char *nodo_ser;
	size_t len;

	len = 0;
	memcpy(&len_nodo, tran, 4);
	len += 4;
	nodo_ser = malloc(len_nodo);
	memcpy(nodo_ser, tran + len, len_nodo);
	tran_des->nodo = deserializar_nodo(nodo_ser, &len_nodo);
	len += len_nodo;
	memcpy(&tran_des->bloque, tran + len, 4);
	len += 4;
	memcpy(&tran_des->bytes, tran + len, 4);
	len += 4;
	memcpy(&len_temp, tran + len, 4);
	len += 4;
	tran_des->temporal = malloc(len_temp);
	memcpy(tran_des->temporal, tran + len, len_temp);
	len += len_temp;

	return tran_des;
}

char *serializar_archivo_temporal(t_list *archivos, size_t *len)
{
	*len = 0;
	void _get_strings_length(char *archivo)
	{
		*len += (size_t)string_length(archivo) +1;
	}
	list_iterate(archivos, (void *)_get_strings_length);

	char *archivos_serializados = malloc(*len);
	*len = 0;
	size_t size_l = (size_t)list_size(archivos);
	memcpy(archivos_serializados, &size_l , 4);
	*len += 4;

	void _serializar_archivos(char *archivo)
	{
		size_t tam_arch =(size_t)string_length(archivo) + 1;
		memcpy(archivos_serializados + *len, &tam_arch, 4);
		*len += 4;
		memcpy(archivos_serializados + *len, archivo, tam_arch);
		*len += tam_arch;
	}

	list_iterate(archivos,(void *) _serializar_archivos);

	return archivos_serializados;
}

char *serializar_redLocal(t_redLocal *red_local, size_t *len)
{
	size_t len_nodo;
	size_t len_archivos;

	char *lista_archivos = serializar_archivo_temporal(red_local->archivos_temp, &len_archivos);
	size_t len_temp_red_local = (size_t) string_length(red_local->temp_red_local) + 1;
	char *nodo = serializar_nodo(red_local->nodo, &len_nodo);
	char *redLocal_ser = malloc(sizeof(t_redLocal) + len_archivos + len_temp_red_local + len_nodo);

	*len = 0;
	memcpy(redLocal_ser, lista_archivos, len_archivos);
	*len += len_archivos;
	memcpy(redLocal_ser + *len, &len_nodo, 4);
	*len += 4;
	memcpy(redLocal_ser + *len, nodo, len_nodo);
	*len += len_nodo;
	memcpy(redLocal_ser + *len, &len_temp_red_local, 4);
	*len += 4;
	memcpy(redLocal_ser + *len, red_local->temp_red_local, len_temp_red_local);
	*len += len_temp_red_local;

	return redLocal_ser;
}

t_list *deserializar_archivo_temporal(char *lista_archivos, size_t *len)
{
	t_list *archivos = list_create();
	size_t size_list;

	memcpy(&size_list, lista_archivos, 4);
	*len = 4;

	while(size_list > 0)
	{
		size_t len2 = 0;
		memcpy(&len2, lista_archivos + *len, 4);
		*len += 4;
		char *archivo = malloc(len2);
		memcpy(archivo, lista_archivos + *len, len2);
		*len += len2;

		list_add(archivos, archivo);

		size_list--;
	}

	return archivos;
}

t_redLocal *deserializar_redLocal (char *red_local_ser)
{
	t_redLocal *red_local_des = malloc(sizeof(t_redLocal));

	size_t len_local;
	size_t len_nodo;
	char *nodo_ser;
	size_t len;

	red_local_des->archivos_temp = deserializar_archivo_temporal(red_local_ser, &len);

	memcpy(&len_nodo, red_local_ser, 4);
	len += 4;
	nodo_ser = malloc(len_nodo);
	memcpy(nodo_ser, red_local_ser + len, len_nodo);
	red_local_des->nodo = deserializar_nodo(nodo_ser, &len_nodo);
	len += len_nodo;
	memcpy(&len_local, red_local_ser + len, 4);
	red_local_des->temp_red_local = malloc(len_local);
	len += 4;
	memcpy(red_local_des->temp_red_local, red_local_ser + len, len_local);
	len += len_local;

	return red_local_des;
}

char *serializar_redGlobal(t_redGlobal *red_global, size_t *len)
{
	size_t len_nodo;
	size_t len_red_global = (size_t) string_length(red_global->red_global) + 1;
	size_t len_red_local = (size_t) string_length(red_global->temp_red_local) + 1;
	char *nodo = serializar_nodo(red_global->nodo, &len_nodo);
	char *red_ser = malloc(sizeof(t_redGlobal) + len_red_global + len_red_local + len_nodo);

	*len = 0;
	memcpy(red_ser, &len_nodo, 4);
	*len += 4;
	memcpy(red_ser + *len, nodo, len_nodo);
	*len += len_nodo;
	memcpy(red_ser + *len, &len_red_local, 4);
	*len += 4;
	memcpy(red_ser + *len, red_global->temp_red_local, len_red_local);
	*len += len_red_local;
	memcpy(red_ser + *len, &len_red_global, 4);
	*len += 4;
	memcpy(red_ser + *len, red_global->red_global, len_red_global);
	*len += len_red_global;
	memcpy(red_ser + *len, &red_global->encargado, 4);
	*len += 4;

	return red_ser;
}

t_redGlobal *deserializar_redGlobal(char *red_ser)
{
	t_redGlobal *red_des = malloc(sizeof(t_redGlobal));

	size_t len_global;
	size_t len_local;
	size_t len_nodo;
	char *nodo_ser;
	size_t len;

	len = 0;
	memcpy(&len_nodo, red_ser, 4);
	len += 4;
	nodo_ser = malloc(len_nodo);
	memcpy(nodo_ser, red_ser + len, len_nodo);
	red_des->nodo = deserializar_nodo(nodo_ser, &len_nodo);
	len += len_nodo;
	memcpy(&len_local, red_ser + len, 4);
	len += 4;
	red_des->temp_red_local = malloc(len_local);
	memcpy(red_des->temp_red_local, red_ser + len, len_local);
	len += len_local;
	memcpy(&len_global, red_ser + len, 4);
	len += 4;
	red_des->red_global = malloc(len_global);
	memcpy(red_des->red_global, red_ser + len, len_global);
	len += len_global;
	memcpy(&red_des->encargado, red_ser + len, 4);
	len += 4;

	return red_des;
}

char *serializar_almacenado(t_almacenado *almacenado, size_t *len)
{
	size_t len_nodo;
	size_t len_red_global = (size_t) string_length(almacenado->red_global) + 1;
	char *nodo = serializar_nodo(almacenado->nodo, &len_nodo);
	char *alm_ser = malloc(sizeof(t_almacenado) + len_red_global + len_nodo);

	*len = 0;
	memcpy(alm_ser, &len_nodo, 4);
	*len += 4;
	memcpy(alm_ser + *len, nodo, len_nodo);
	*len += len_nodo;
	memcpy(alm_ser + *len, &len_red_global, 4);
	*len += 4;
	memcpy(alm_ser + *len, almacenado->red_global, len_red_global);
	*len += len_red_global;

	return alm_ser;
}

t_almacenado *deserializar_almacenado(char *alm_ser)
{
	t_almacenado *alm_des = malloc(sizeof(t_almacenado));

	size_t len_global;
	size_t len_nodo;
	char *nodo_ser;
	size_t len;

	len = 0;
	memcpy(&len_nodo, alm_ser, 4);
	len += 4;
	nodo_ser = malloc(len_nodo);
	memcpy(nodo_ser, alm_ser + len, len_nodo);
	alm_des->nodo = deserializar_nodo(nodo_ser, &len_nodo);
	len += len_nodo;
	memcpy(&len_global, alm_ser + len, 4);
	len += 4;
	alm_des->red_global = malloc(len_global);
	memcpy(alm_des->red_global, alm_ser + len, len_global);
	len += len_global;

	return alm_des;
}

char *serializar_estado_master(t_estado_master *estado_master, size_t *len)
{
	size_t len_nodo = (size_t) string_length(estado_master->nodo) + 1;
	char *alm_ser = malloc(sizeof(t_estado_master) + len_nodo);

	*len = 0;
	memcpy(alm_ser, &len_nodo, 4);
	*len += 4;
	memcpy(alm_ser + *len, estado_master->nodo, len_nodo);
	*len += len_nodo;
	memcpy(alm_ser + *len, &estado_master->bloque, 4);
	*len += 4;
	memcpy(alm_ser + *len, &estado_master->estado, 4);
	*len += 4;

	return alm_ser;
}

t_estado_master *deserializar_estado_master(char *em_ser)
{
	t_estado_master *em_des = malloc(sizeof(t_estado_master));

	size_t len_nodo;
	size_t len;

	len = 0;
	memcpy(&len_nodo, em_ser, 4);
	len += 4;
	em_des->nodo = malloc(len_nodo);
	memcpy(em_des->nodo, em_ser + len, len_nodo);
	len += len_nodo;
	memcpy(&em_des->bloque, em_ser + len, 4);
	len += 4;
	memcpy(&em_des->estado, em_ser + len, 4);
	len += 4;

	return em_des;
}

int tamanio_nodo(t_nodo *nodo)
{
	int tam_nodo = sizeof(t_nodo);
	tam_nodo += string_length(nodo->ip) +1;
	tam_nodo += string_length(nodo->nodo) +1;

	return tam_nodo;
}

int tamanio_transformacion(t_transformacion *transformacion)
{
	int tam_transf = sizeof(t_transformacion);
	tam_transf += string_length(transformacion->temporal) +1;
	tam_transf += tamanio_nodo(transformacion->nodo);

	return tam_transf;
}

char *serializar_lista_transformacion(t_list *l_transformacion, size_t *len)
{
	char *transfs_serializadas;
	int tam_malloc = sizeof(t_list);
	int tam_lista = list_size(l_transformacion);
	int i = 0;
	*len = 0;

	while(tam_lista > i)
	{
		t_transformacion *transf = list_get(l_transformacion, i);
		tam_malloc += tamanio_transformacion(transf);
		i++;
	}

	i = 0;
	transfs_serializadas = malloc((size_t)tam_malloc);
	memcpy(transfs_serializadas, &tam_lista, 4);
	*len += 4;

	while(tam_lista > i)
	{
		size_t len2 = 0;
		t_transformacion *transf = list_get(l_transformacion, i);
		int tr_size = tamanio_transformacion(transf);
		char *tr_ser = serializar_transformacion(transf, &len2);

		memcpy(transfs_serializadas + *len, &tr_size, 4);
		*len += 4;
		memcpy(transfs_serializadas + *len, tr_ser, len2);
		*len += len2;

		i++;
	}

	return transfs_serializadas;
}

t_list *deserializar_lista_transformacion(char *lista_ser)
{
	t_list *lista_des = list_create();
	size_t len = 0;
	int size_list;

	memcpy(&size_list, lista_ser, 4);
	len += 4;

	while(size_list > 0)
	{
		size_t len2 = 0;
		memcpy(&len2, lista_ser + len, 4);
		len += 4;
		char *tr_ser = malloc(len2);
		memcpy(tr_ser, lista_ser + len, len2);
		len += len2;

		t_transformacion *transf = deserializar_transformacion(tr_ser);

		list_add(lista_des, transf);

		size_list--;
	}

	return lista_des;
}

int tamanio_redGlobal(t_redGlobal *redGlobal)
{
	int tam_redGlobal = sizeof(t_redGlobal);
	tam_redGlobal += string_length(redGlobal->red_global) +1;
	tam_redGlobal += string_length(redGlobal->temp_red_local) +1;
	tam_redGlobal += tamanio_nodo(redGlobal->nodo);

	return tam_redGlobal;
}

char *serializar_lista_redGlobal(t_list *l_redGlobal, size_t *len)
{
	char *redGlobs_serializadas;
	int tam_malloc = sizeof(t_list);
	int tam_lista = list_size(l_redGlobal);
	int i = 0;
	*len = 0;

	while(tam_lista > i)
	{
		t_redGlobal *redG = list_get(l_redGlobal, i);
		tam_malloc += tamanio_redGlobal(redG);
		i++;
	}

	i = 0;
	redGlobs_serializadas = malloc((size_t)tam_malloc);
	memcpy(redGlobs_serializadas, &tam_lista, 4);
	*len += 4;

	while(tam_lista > i)
	{
		size_t len2 = 0;
		t_redGlobal *redG = list_get(l_redGlobal, i);
		int tr_size = tamanio_redGlobal(redG);
		char *tr_ser = serializar_redGlobal(redG, &len2);

		memcpy(redGlobs_serializadas + *len, &tr_size, 4);
		*len += 4;
		memcpy(redGlobs_serializadas + *len, tr_ser, len2);
		*len += len2;

		i++;
	}

	return redGlobs_serializadas;
}

t_list *deserializar_lista_redGlobal(char *lista_ser)
{
	t_list *lista_des = list_create();
	size_t len = 0;
	int size_list;

	memcpy(&size_list, lista_ser, 4);
	len += 4;

	while(size_list > 0)
	{
		size_t len2 = 0;
		memcpy(&len2, lista_ser + len, 4);
		len += 4;
		char *redG_ser = malloc(len2);
		memcpy(redG_ser, lista_ser + len, len2);
		len += len2;

		t_redGlobal *redG = deserializar_redGlobal(redG_ser);

		list_add(lista_des, redG);

		size_list--;
	}

	return lista_des;
}
