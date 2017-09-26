#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>

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

char *serializar_redLocal(t_redLocal *red_local, size_t *len)
{
	size_t len_nodo;
	size_t len_temp_transformacion = (size_t) string_length(red_local->temp_transformacion) + 1;
	size_t len_temp_red_local = (size_t) string_length(red_local->temp_red_local) + 1;
	char *nodo = serializar_nodo(red_local->nodo, &len_nodo);
	char *redLocal_ser = malloc(sizeof(t_redLocal) + len_temp_transformacion + len_temp_red_local + len_nodo);


	*len = 0;
	memcpy(redLocal_ser, &len_nodo, 4);
	*len += 4;
	memcpy(redLocal_ser + *len, nodo, len_nodo);
	*len += len_nodo;
	memcpy(redLocal_ser + *len, &len_temp_transformacion, 4);
	*len += 4;
	memcpy(redLocal_ser + *len, red_local->temp_transformacion, len_temp_transformacion);
	*len += len_temp_transformacion;
	memcpy(redLocal_ser + *len, &len_temp_red_local, 4);
	*len += 4;
	memcpy(redLocal_ser + *len, red_local->temp_red_local, len_temp_red_local);
	*len += len_temp_red_local;

	return redLocal_ser;
}

t_redLocal *deserializar_redLocal (char *red_local_ser)
{
	t_redLocal *red_local_des = malloc(sizeof(t_redLocal));

	size_t len_temp;
	size_t len_local;
	size_t len_nodo;
	char *nodo_ser;
	size_t len;

	len = 0;
	memcpy(&len_nodo, red_local_ser, 4);
	len += 4;
	nodo_ser = malloc(len_nodo);
	memcpy(nodo_ser, red_local_ser + len, len_nodo);
	red_local_des->nodo = deserializar_nodo(nodo_ser, &len_nodo);
	len += len_nodo;
	memcpy(&len_temp, red_local_ser + len, 4);
	len += 4;
	red_local_des->temp_transformacion = malloc(len_temp);
	memcpy(red_local_des->temp_transformacion, red_local_ser + len, len_temp);
	len += len_temp;
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
