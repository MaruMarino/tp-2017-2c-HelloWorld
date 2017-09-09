#ifndef AUXILIARES_WORKER_H_
#define AUXILIARES_WORKER_H_

#include <stdio.h>

/* Ordena lexicograficamente las entradas de un puntero a lineas,
 * dada la cantidad de lineas a las que apunta el puntero.
 */
void sort(char **lines[], int line_count);

/* Retorna la cantidad de lineas del FILE f
 * En error retorna -1
 *
 * ** NO produce efectos sobre f :)
 */
int countLines(FILE *f);

/* Crea un char**: un puntero donde cada puntero es una linea del FILE f
 * Retorna ademas la dimension del char* (las file_lines)
 * En error retorna NULL y setea file_lines en -1
 *
 * ** NO produce efectos sobre f :)
 */
char **readFileIntoArray(FILE *f, int *file_lines);

/* A partir de char**: un puntero donde cada puntero es un string,
 * sobreescribe linea por linea sobre el FILE f.
 *
 * ** SI produce efectos sobre f, lo trunca zarpado y le escribe :)
 */
int writeArrayIntoFile(char **lines, int dim, const char *path);


#endif /* AUXILIARES_WORKER_H_ */
