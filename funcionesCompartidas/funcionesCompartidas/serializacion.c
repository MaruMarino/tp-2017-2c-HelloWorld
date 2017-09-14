#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "estructuras.h"

char *serializar_info_trans(t_info_trans *info, size_t *len){

	char *info_serial = malloc(sizeof *info + info->size_prog + (size_t) info->len_out);
	if (info_serial == NULL)
		return NULL;

	*len = 0;
	memcpy(info_serial, &info->size_prog, sizeof(int));
	len += sizeof(int);
	memcpy(info_serial, info->prog, info->size_prog);
	len += info->size_prog;
	memcpy(info_serial, &info->bloque, sizeof(int));
	len += sizeof(int);
	memcpy(info_serial, &info->bytes_ocup, sizeof(int));
	len += sizeof(int);
	memcpy(info_serial, &info->len_out, sizeof(int));
	len += sizeof(int);
	memcpy(info_serial, info->file_out, (size_t) info->len_out);
	len += info->len_out;

	printf("Se serializaron %du bytes\n", *len);

	return info_serial;
}

t_info_trans *deserializar_info_trans(char *info_serial){

	t_info_trans *info = malloc(sizeof *info);

	size_t off = 0;
	memcpy(&info->size_prog, info_serial + off, sizeof(int));
	off += sizeof(int);
	memcpy(&info->prog, info_serial + off, info->size_prog);
	off += info->size_prog;
	memcpy(&info->bloque, info_serial + off, sizeof(int));
	off += sizeof(int);
	memcpy(&info->bytes_ocup, info_serial + off, sizeof(int));
	off += sizeof(int);
	memcpy(&info->len_out, info_serial + off, sizeof(int));
	off += sizeof(int);
	memcpy(&info->file_out, info_serial + off, (size_t) info->len_out);
	off += (size_t) info->len_out;

	printf("Se deserializaron %d bytes\n", off);

	return info;
}
