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
	size_t ipl, portl, fnl, flen;
	t_info_nodo *n;
	flen = strlen(info->file_out) + 1;
	size_t list_size = sizeOfInfoNodos(info->nodos);

	char *info_serial = malloc(3 * sizeof(int) + info->size_prog + list_size + flen);

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

	memcpy(info_serial + *len, &flen, sizeof(int));
	*len += sizeof(int);
	memcpy(info_serial + *len, info->file_out, flen);
	*len += flen;

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

	memcpy(&fnl, info_serial + off, sizeof(int));
	off += sizeof(int);
	info->file_out = malloc(fnl);
	memcpy(info->file_out, info_serial + off, fnl);
	off += fnl;

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

char *serializar_File(t_file *file, size_t *len){

	size_t flen = strlen(file->fname) + 1;
	char *file_serial = malloc(sizeof(off_t) + flen + sizeof(int) + (size_t) file->fsize);

	char *fname_serial = serializar_FName(file->fname, len);
	memcpy(file_serial, fname_serial, *len);
	free(fname_serial);
	memcpy(file_serial + *len, &file->fsize, sizeof(int));
	*len += sizeof(int);
	memcpy(file_serial + *len, file->data,(size_t) file->fsize);
	*len += (size_t) file->fsize;

	printf("Se serializaron %d bytes\n", *len);

	return file_serial;
}

t_file *deserializar_File(char *file_serial){

	size_t off = 0;
	t_file *file = malloc(sizeof *file);

	file->fname = deserializar_FName(file_serial);
	memcpy(&off, file_serial, sizeof off);
	memcpy(&file->fsize, file_serial + off, sizeof(int));
	off += sizeof(off_t);
	file->data = malloc((size_t) file->fsize);
	memcpy(file_serial + off, file->data, (size_t) file->fsize);
	off += (size_t) file->fsize;

	//printf("Se deserializaron %d bytes\n", off);

	return file;
}

//todo testear estas serializaciones
char *serializar_bloque_archivo(bloqueArchivo *inf,size_t *len){

	size_t desplazamiento = 0;
	size_t leng = tamanio_bloque_archivo(inf);
	size_t aux;
	char *buff = malloc(leng+sizeof(int));

	memcpy(buff+desplazamiento,&leng,sizeof(int));
	desplazamiento += sizeof(int);

	aux = strlen(inf->nodo0);
	memcpy(buff+desplazamiento,&aux,sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buff+desplazamiento,inf->nodo0,aux);
	desplazamiento += aux;

	aux = strlen(inf->nodo1);
	memcpy(buff+desplazamiento,&aux,sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buff+desplazamiento,inf->nodo1,aux);
	desplazamiento += aux;

	memcpy(buff+desplazamiento,&inf->bloquenodo0,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(buff+desplazamiento,&inf->bloquenodo1,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(buff+desplazamiento,&inf->bytesEnBloque,sizeof(int));
	desplazamiento += sizeof(int);

	*len = desplazamiento;

	return buff;
}
bloqueArchivo *deserializar_bloque_archivo(char *serba){

	bloqueArchivo *nuevo = malloc(sizeof(bloqueArchivo));
	size_t aux;
	size_t desplazamiento = 0;

	memcpy(&aux,serba+desplazamiento,sizeof(int));
	desplazamiento += sizeof(int);
	nuevo->nodo0 = malloc(aux+1); nuevo->nodo0[aux] = '\0';
	memcpy(nuevo->nodo0,serba+desplazamiento,aux);
	desplazamiento += aux;

	memcpy(&aux,serba+desplazamiento,sizeof(int));
	desplazamiento += sizeof(int);
	nuevo->nodo1 = malloc(aux+1); nuevo->nodo0[aux] = '\0';
	memcpy(nuevo->nodo1,serba+desplazamiento,aux);
	desplazamiento += aux;

	memcpy(&nuevo->bloquenodo0,serba+desplazamiento,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&nuevo->bloquenodo1,serba+desplazamiento,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&nuevo->bytesEnBloque,serba+desplazamiento,sizeof(int));
	desplazamiento += sizeof(int);

	return nuevo;
}

char *serializar_list_bloque_archivo(t_list *info_nodos_arc,size_t *leng){

	size_t lengtotal=0;
	size_t desplazamiento = 0;
	int cantnodos = list_size(info_nodos_arc);
	int i;
	bloqueArchivo *uno;
	for(i=0;i<cantnodos;i++){
		uno = list_get(info_nodos_arc,i);
		lengtotal += tamanio_bloque_archivo(uno);
	}
	char *buffer = malloc(lengtotal + sizeof(int));

	memcpy(buffer+desplazamiento,&cantnodos,sizeof(int));
	desplazamiento += sizeof(int);

	char *baux;
	size_t laux = 0;
	for(i=0;i<cantnodos;i++){
		uno =list_get(info_nodos_arc,i);
		baux = serializar_bloque_archivo(uno,&laux);
		memcpy(buffer+desplazamiento,baux,laux);
		desplazamiento += laux;
		free(baux);
	}

	*leng = desplazamiento;

	return buffer;
}

t_list *deserializar_lista_bloque_archivo(char *serializacion){

	t_list *final = list_create();
	size_t desplazamiento=0;

	int cantnodos,i;

	memcpy(&cantnodos,serializacion+desplazamiento,sizeof(int));
	desplazamiento += sizeof(int);

	size_t aux;
	for(i=0;i<cantnodos;i++){

		memcpy(&aux,serializacion+desplazamiento,sizeof(int));
		desplazamiento += sizeof(int);
		char *baux = malloc(aux);
		memcpy(baux,serializacion+desplazamiento,aux);
		bloqueArchivo *nuevito = deserializar_bloque_archivo(baux);
		list_add(final,nuevito);
		free(baux);
	}

	return final;
}
