#include <stdarg.h>
#include <stdlib.h>

#include <commons/collections/list.h>

#include "estructuras.h"

void liberador(int nptr, void *fst, ...){

	void *p;
	va_list ptrp;
	va_start(ptrp, fst);

	free(fst);
	for (nptr--; nptr > 0; nptr--){
		p = va_arg(ptrp, void*);
		free(p);
	}
}

size_t sizeOfFnames(t_list *fnames){

	int i;
	size_t total = 0;
	t_fname *fn;

	for (i = 0; i < fnames->elements_count; ++i){
		fn = list_get(fnames, i);
		total += sizeof fn->len + (size_t) fn->len;
	}

	return total;
}

void liberarFnames(t_list *fnames){

	int i;
	t_fname *fn;

	for (i = 0; i < fnames->elements_count; ++i){
		fn = list_get(fnames, i);
		free(fn->fname);
		free(fn);
	}
}
