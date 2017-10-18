#ifndef FUNCIONESCOMPARTIDAS_GENERALES_H_
#define FUNCIONESCOMPARTIDAS_GENERALES_H_

#include <commons/collections/list.h>
#include "estructuras.h"

/* Libera los punteros pasados por parametro.
 * nptr indica la cantidad de punteros a liberar y
 * el resto de los parametros son los propios punteros.
 */
void liberador(int nptr, void *fst, ...);

/* Retorna el peso de todos los elementos de la lista de t_info_nodo.
 */
size_t sizeOfInfoNodos(t_list *nodos);

/* libera por completo los elementos de la lista de t_info_nodo
 */
void liberarInfoNodos(t_list *nodos);

/* Retorna el peso de todos los elementos de la lista de char *fname.
 */
size_t sizeOfFnames(t_list *fnames);

/* libera por completo los elementos de la lista de char *fname
 */
void liberarFnames(t_list *fnames);

size_t tamanio_bloque_archivo(bloqueArchivo *info);

#endif /* FUNCIONESCOMPARTIDAS_GENERALES_H_ */
