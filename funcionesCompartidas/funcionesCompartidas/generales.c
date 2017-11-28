#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

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


size_t sizeOfInfoNodos(t_list *nodos){

	int i;
	size_t total = 0;
	t_info_nodo *n;

	for (i = 0; i < nodos->elements_count; ++i){
		n = list_get(nodos, i);
		total += strlen(n->fname) + strlen(n->ip) + strlen(n->port) + 3 + 3 * sizeof(int);
	}

	return total;
}

void liberarInfoNodos(t_list *nodos){
	void freer(t_info_nodo *n){
		liberador(4, n->fname, n->ip, n->port, n);
	}
	list_destroy_and_destroy_elements(nodos, (void*) freer);
}

size_t sizeOfFnames(t_list *fnames){

	int i;
	size_t total = 0;
	char *fn;

	for (i = 0; i < fnames->elements_count; ++i){
		fn = list_get(fnames, i);
		total += sizeof(int) + strlen(fn) + 1;
	}

	return total;
}

void liberarFnames(t_list *fnames){
	list_destroy_and_destroy_elements(fnames, free);
}

size_t tamanio_bloque_archivo(bloqueArchivo *ba){

	size_t retorno=0;

	retorno += strlen(ba->nodo0);
	retorno += strlen(ba->nodo1);
	retorno += sizeof(bloqueArchivo);

	return retorno;
}

size_t tamanio_lista_t_nodo(t_list *nodis){

	int i;
	size_t tfinal = 0;
	t_nodo *nodi;

	for(i=0;i<nodis->elements_count;i++){

		nodi = list_get(nodis,i);
		tfinal += strlen(nodi->nodo)+2 + strlen(nodi->ip) + sizeof(t_nodo);
	}


		return tfinal;
}

void liberar_char_array(char **miarray) {

	int i = 0;
	while (miarray[i] != NULL) {
		free(miarray[i]);
		i++;
	}
	free(miarray);
}





