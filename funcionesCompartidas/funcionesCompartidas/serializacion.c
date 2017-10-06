#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "estructuras.h"
#include "generales.h"
#include "serializacion.h"


char *serializar_info_trans(t_info_trans *info, size_t *len){

	size_t fnlen = strlen(info->file_out) + 1;
	char *info_serial = malloc(4 * sizeof(int) + info->size_prog + fnlen);
	if (info_serial == NULL)
		return NULL;

	*len = 0;
	memcpy(info_serial, &info->size_prog, sizeof(int));
	*len += sizeof(int);
	memcpy(info_serial + *len, info->prog, info->size_prog);
	*len += info->size_prog;
	memcpy(info_serial + *len, &info->bloque, sizeof(int));
	*len += sizeof(int);
	memcpy(info_serial + *len, &info->bytes_ocup, sizeof(int));
	*len += sizeof(int);
	memcpy(info_serial + *len, &fnlen, sizeof(int));
	*len += sizeof(int);
	memcpy(info_serial + *len, info->file_out, fnlen);
	*len += fnlen;

	printf("Se serializaron %d bytes\n", *len);

	return info_serial;
}

t_info_trans *deserializar_info_trans(char *info_serial){

	size_t fnlen;
	t_info_trans *info = malloc(sizeof *info);

	size_t off = 0;
	memcpy(&info->size_prog, info_serial + off, sizeof(int));
	off += sizeof(int);
	info->prog = malloc(info->size_prog);
	memcpy(info->prog, info_serial + off, info->size_prog);
	off += info->size_prog;
	memcpy(&info->bloque, info_serial + off, sizeof(int));
	off += sizeof(int);
	memcpy(&info->bytes_ocup, info_serial + off, sizeof(int));
	off += sizeof(int);
	memcpy(&fnlen, info_serial + off, sizeof(int));
	off += sizeof(int);
	info->file_out = malloc(fnlen);
	memcpy(info->file_out, info_serial + off, fnlen);
	off += fnlen;

	printf("Se deserializaron %d bytes\n", off);

	return info;
}

char *serializar_info_redLocal(t_info_redLocal *info, size_t *len){

	int i;
	char *fn;
	size_t auxlen;
	size_t fnlen = strlen(info->file_out) + 1;
	size_t size_fnames = sizeOfFnames(info->files);

	char *info_serial = malloc(sizeof(size_t) + info->size_prog + sizeof(int) +
			size_fnames + sizeof(size_t) + fnlen);
	if (info_serial == NULL)
		return NULL;

	*len = 0;
	memcpy(info_serial, &info->size_prog, sizeof(size_t));
	*len += sizeof(size_t);
	memcpy(info_serial + *len, info->prog, info->size_prog);
	*len += info->size_prog;
	memcpy(info_serial + *len, &info->files->elements_count, sizeof(int));
	*len += sizeof(int);
	for (i = 0; i < info->files->elements_count; ++i){
		fn = list_get(info->files, i);
		auxlen = strlen(fn) + 1;
		memcpy(info_serial + *len, &auxlen, sizeof(int));
		*len += sizeof(int);
		memcpy(info_serial + *len, fn, auxlen);
		*len += auxlen;
	}

	memcpy(info_serial + *len, &fnlen, sizeof(int));
	*len += sizeof(int);
	memcpy(info_serial + *len, info->file_out, fnlen);
	*len += fnlen;

	printf("Se serializaron %d bytes\n", *len);

	return info_serial;
}

t_info_redLocal *deserializar_info_redLocal(char *info_serial){

	size_t fnlen;
	int len_list, i;
	t_info_redLocal *info = malloc(sizeof *info);

	size_t off = 0;
	memcpy(&info->size_prog, info_serial, sizeof(int));
	off += sizeof(int);

	info->prog = malloc(info->size_prog);
	memcpy(info->prog, info_serial + off, info->size_prog);
	off += info->size_prog;
	memcpy(&len_list, info_serial + off, sizeof(int));
	off += sizeof(int);

	info->files = list_create();
	for (i = 0; i < len_list; ++i){
		char *fn;
		memcpy(&fnlen, info_serial + off, sizeof(int));
		off += sizeof(int);
		fn = malloc(fnlen);
		memcpy(fn, info_serial + off, fnlen);
		off += fnlen;

		list_add(info->files, fn);
	}

	memcpy(&fnlen, info_serial + off, sizeof(int));
	off += sizeof(int);
	info->file_out = malloc(fnlen);
	memcpy(info->file_out, info_serial + off, fnlen);
	off += fnlen;

	printf("Se deserializaron %d bytes\n", off);

	return info;
}

char *serializar_info_redGlobal(t_info_redGlobal *info, size_t *len){

	int i;
	size_t ipl, portl, fnl;
	t_info_nodo *n;
	size_t list_size = sizeOfInfoNodos(info->nodos);

	char *info_serial = malloc(sizeof(int) + info->size_prog + sizeof(int) + list_size);

	*len = 0;
	memcpy(info_serial + *len, &info->size_prog, sizeof(int));
	*len += sizeof(int);
	memcpy(info_serial + *len, info->prog, info->size_prog);
	*len += info->size_prog;

	memcpy(info_serial + *len, &info->nodos->elements_count, sizeof(int));
	*len += sizeof(int);
	for (i = 0; i < info->nodos->elements_count; ++i){
		n = list_get(info->nodos, i);
		ipl = strlen(n->ip) + 1;
		portl = strlen(n->port) + 1;
		fnl = strlen(n->fname) + 1;

		memcpy(info_serial + *len, &ipl, sizeof(int));
		*len += sizeof(int);
		memcpy(info_serial + *len, n->ip, ipl);
		*len += ipl;
		memcpy(info_serial + *len, &portl, sizeof(int));
		*len += sizeof(int);
		memcpy(info_serial + *len, n->port, portl);
		*len += portl;
		memcpy(info_serial + *len, &fnl, sizeof(int));
		*len += sizeof(int);
		memcpy(info_serial + *len, n->fname, fnl);
		*len += fnl;
	}

	printf("Se serializaron %d bytes\n", *len);

	return info_serial;
}

t_info_redGlobal *deserializar_info_redGlobal(char *info_serial){

	size_t elems, i, ipl, portl, fnl;
	t_info_redGlobal *info = malloc(sizeof *info);

	size_t off = 0;
	memcpy(&info->size_prog, info_serial, sizeof(int));
	off += sizeof(int);
	info->prog = malloc(info->size_prog);
	memcpy(info->prog, info_serial + off, info->size_prog);
	off += info->size_prog;

	memcpy(&elems, info_serial + off, sizeof(int));
	off += sizeof(int);
	info->nodos = list_create();
	for (i = 0; i < elems; ++i){
		t_info_nodo *n = malloc(sizeof *n);

		memcpy(&ipl, info_serial + off, sizeof(int));
		off += sizeof(int);
		n->ip = malloc(ipl);
		memcpy(n->ip, info_serial + off, ipl);
		off += ipl;
		memcpy(&portl, info_serial + off, sizeof(int));
		off += sizeof(int);
		n->port = malloc(portl);
		memcpy(n->port, info_serial + off, portl);
		off += portl;
		memcpy(&fnl, info_serial + off, sizeof(int));
		off += sizeof(int);
		n->fname = malloc(fnl);
		memcpy(n->fname, info_serial + off, fnl);
		off += fnl;

		list_add(info->nodos, n);
	}

	printf("Se deserializaron %d bytes\n", off);

	return info;
}

char *serializar_stream(char *bytes, size_t bytelen, size_t *len){

	char *bytes_serial = malloc(sizeof(int) + bytelen);

	*len = 0;
	memcpy(bytes_serial, &bytelen, sizeof(int));
	*len += sizeof(int);
	memcpy(bytes_serial + *len, bytes, bytelen);
	*len += bytelen;

	printf("Se serializaron %d bytes\n", *len);

	return bytes_serial;
}

char *deserializar_stream(char *bytes_serial, size_t *bytelen){

	char *bytes;
	size_t off;

	off = 0;
	memcpy(bytelen, bytes_serial, sizeof(int));
	off += sizeof(int);
	bytes = malloc(*bytelen);
	memcpy(bytes, bytes_serial + off, *bytelen);
	off += *bytelen;

	printf("Se deserializaron %d bytes\n", off);

	return bytes;
}

char *serializar_FName(char *fn, size_t *len){

	size_t fnlen = strlen(fn) + 1;
	char *fname_serial = malloc(sizeof(int) + fnlen);

	*len = 0;
	memcpy(fname_serial, &fnlen, sizeof(int));
	*len += sizeof(int);
	memcpy(fname_serial + *len, fn, fnlen);
	*len += fnlen;

	printf("Se serializaron %d bytes\n", *len);

	return fname_serial;
}

char *deserializar_FName(char *fname_serial){

	char *fn;
	size_t off, fnlen;

	off = 0;
	memcpy(&fnlen, fname_serial, sizeof(int));
	off += sizeof(int);
	fn = malloc(fnlen);
	memcpy(fn, fname_serial + off, fnlen);
	off += fnlen;

	printf("Se deserializaron %d bytes\n", off);

	return fn;
}
