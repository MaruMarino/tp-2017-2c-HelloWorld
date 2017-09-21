#ifndef FUNCIONESCOMPARTIDAS_GENERALES_H_
#define FUNCIONESCOMPARTIDAS_GENERALES_H_

/* Libera los punteros pasados por parametro.
 * nptr indica la cantidad de punteros a liberar y
 * el resto de los parametros son los propios punteros.
 */
void liberador(int nptr, void *fst, ...);

#endif /* FUNCIONESCOMPARTIDAS_GENERALES_H_ */
