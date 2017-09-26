#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "estructuras.h"
#include "generales.h"
#include "serializacion.h"


char *serializar_info_trans(t_info_trans *info, size_t *len){

	char *info_serial = malloc(sizeof *info + info->size_prog + (size_t) info->file_out.len);
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
	memcpy(info_serial + *len, &info->file_out.len, sizeof(int));
	*len += sizeof(int);
	memcpy(info_serial + *len, info->file_out.fname, (size_t) info->file_out.len);
	*len += (size_t) info->file_out.len;

	printf("Se serializaron %d bytes\n", *len);

	return info_serial;
}

t_info_trans *deserializar_info_trans(char *info_serial){

	t_info_trans *info = malloc(sizeof *info);

	size_t off = 0;
	memcpy(&info->size_prog, info_serial + off, sizeof(int));
	off += sizeof(int);

	if (info->size_prog <= 0){
		fprintf(stderr, "size_prog no es valido: %du\n", info->size_prog);
		free(info);
		return NULL;
	}

	info->prog = malloc(info->size_prog);
	memcpy(info->prog, info_serial + off, info->size_prog);
	off += info->size_prog;
	memcpy(&info->bloque, info_serial + off, sizeof(int));
	off += sizeof(int);
	memcpy(&info->bytes_ocup, info_serial + off, sizeof(int));
	off += sizeof(int);
	memcpy(&info->file_out.len, info_serial + off, sizeof(int));
	off += sizeof(int);

	if (info->file_out.len <= 0){
		fprintf(stderr, "file_out.len no es valido: %d\n", info->file_out.len);
		liberador(2, info->prog, info);
		return NULL;
	}

	info->file_out.fname = malloc((size_t) info->file_out.len);
	memcpy(info->file_out.fname, info_serial + off, (size_t) info->file_out.len);
	off += (size_t) info->file_out.len;

	printf("Se deserializaron %d bytes\n", off);

	return info;
}

char *serializar_info_redLocal(t_info_redLocal *info, size_t *len){

	int i;
	t_fname *fn;
	size_t size_fnames = sizeOfFnames(info->files);

	char *info_serial = malloc(sizeof(size_t) + info->size_prog + sizeof(int) +
			size_fnames + sizeof(size_t) + (size_t) info->file_out.len);
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
		memcpy(info_serial + *len, &fn->len, sizeof(int));
		*len += sizeof(int);
		memcpy(info_serial + *len, fn->fname, (size_t) fn->len);
		*len += (size_t) fn->len;
	}

	memcpy(info_serial + *len, &info->file_out.len, sizeof(int));
	*len += sizeof(int);
	memcpy(info_serial + *len, info->file_out.fname, (size_t) info->file_out.len);
	*len += (size_t) info->file_out.len;

	printf("Se serializaron %d bytes\n", *len);

	return info_serial;
}

t_info_redLocal *deserializar_info_redLocal(char *info_serial){

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
		t_fname *fn = malloc(sizeof *fn);
		memcpy(&fn->len, info_serial + off, sizeof(int));
		off += sizeof(int);
		fn->fname = malloc((size_t) fn->len);
		memcpy(fn->fname, info_serial + off, (size_t) fn->len);
		off += (size_t) fn->len;

		list_add(info->files, fn);
	}

	memcpy(&info->file_out.len, info_serial + off, sizeof(int));
	off += sizeof(int);
	info->file_out.fname = malloc((size_t) info->file_out.len);
	memcpy(info->file_out.fname, info_serial + off, (size_t) info->file_out.len);
	off += (size_t) info->file_out.len;

	printf("Se deserializaron %d bytes\n", off);

	return info;
}

char *serializar_info_redGlobal(t_info_redGlobal *info, size_t *len){
	return serializar_info_redLocal(info, len); // """polimorfismo""" en C :)
}

t_info_redGlobal *deserializar_info_redGlobal(char *info_serial){
	return deserializar_info_redLocal(info_serial);
}

char *serializar_info_redGlobalSub(t_info_redGlobalSub *info, size_t *len){

	char *info_serial = malloc(3 * sizeof(int) + (size_t) (info->iplen + info->portlen + info->file_in.len));
	if (info_serial == NULL)
		return NULL;

	*len = 0;
	memcpy(info_serial, &info->iplen, sizeof(int));
	*len += sizeof(int);
	memcpy(info_serial + *len, info->ip, (size_t) info->iplen);
	*len += (size_t) info->iplen;
	memcpy(info_serial + *len, &info->portlen, sizeof(int));
	*len += sizeof(int);
	memcpy(info_serial + *len, info->port, (size_t) info->portlen);
	*len += (size_t) info->portlen;
	memcpy(info_serial + *len, &info->file_in.len, sizeof(int));
	*len += sizeof(int);
	memcpy(info_serial + *len, info->file_in.fname, (size_t) info->file_in.len);
	*len += (size_t) info->file_in.len;

	printf("Se serializaron %d bytes\n", *len);

	return info_serial;
}

t_info_redGlobalSub *deserializar_info_redGlobalSub(char *info_serial){

	t_info_redGlobalSub *info = malloc(sizeof *info);

	size_t off = 0;
	memcpy(&info->iplen, info_serial, sizeof(int));
	off += sizeof(int);
	info->ip = malloc((size_t) info->iplen);
	memcpy(info->ip, info_serial + off, (size_t) info->iplen);
	off += (size_t) info->iplen;
	memcpy(&info->portlen, info_serial + off, sizeof(int));
	off += sizeof(int);
	info->port = malloc((size_t) info->portlen);
	memcpy(info->port, info_serial + off, (size_t) info->portlen);
	off += (size_t) info->portlen;
	memcpy(&info->file_in.len, info_serial + off, sizeof(int));
	off += sizeof(int);
	info->file_in.fname = malloc((size_t) info->file_in.len);
	memcpy(info->file_in.fname, info_serial + off, (size_t) info->file_in.len);
	off += (size_t) info->file_in.len;

	printf("Se deserializaron %du bytes\n", off);

	return info;
}
