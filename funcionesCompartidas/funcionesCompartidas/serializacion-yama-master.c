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

	return nodo_ser;
}

t_nodo *deserializar_nodo (char *nodo_ser)
{
	t_nodo *nodo = malloc (sizeof (t_nodo));
	size_t len_nodo;
	size_t len_ip;


	size_t len = 0;
	memcpy(&len_nodo, nodo_ser, 4);
	len += 4;
	nodo->nodo = malloc(len_nodo);
	memcpy(nodo->nodo, nodo_ser + len, len_nodo);
	len += len_nodo;
	memcpy(&len_ip, nodo_ser + len, 4);
	len += 4;
	nodo->ip = malloc(len_ip);
	memcpy(nodo->ip, nodo_ser + len, len_ip);
	len += len_ip;
	memcpy(&nodo->puerto, nodo_ser + len, 4);

	return nodo;
}
