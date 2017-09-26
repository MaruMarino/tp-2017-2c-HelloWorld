#ifndef FUNCIONESCOMPARTIDAS_GENERALES_H_
#define FUNCIONESCOMPARTIDAS_GENERALES_H_

/* Libera los punteros pasados por parametro.
 * nptr indica la cantidad de punteros a liberar y
 * el resto de los parametros son los propios punteros.
 */
void liberador(int nptr, void *fst, ...);

/* Retorna el peso de todos los elementos de la lista de t_fname.
 */
size_t sizeOfFnames(t_list *fnames);

/* libera por completo los elementos de la lista de t_fname, incluido sus char*
 */
void liberarFnames(t_list *fnames);
#endif /* FUNCIONESCOMPARTIDAS_GENERALES_H_ */
