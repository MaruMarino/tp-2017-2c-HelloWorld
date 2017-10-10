#ifndef AUXILIARES_WORKER_H_
#define AUXILIARES_WORKER_H_

#include <stdio.h>

/* recibe una cant `nargs' de strings que concatena y retorna en un unico char*
 * Si falla retorna NULL.
 */
char *crearComando(int nargs, char *fst, ...);

/* Crea un nuevo archivo con nombre `fname' y le escribe los bytes pertinentes
 * de la estructura `info'
 * Retorna -1 si alguna operacion falla. Retorna 0 en caso exitoso.
 */
int crearArchivoBin(char *bin, size_t bin_sz, char *fname);

/* Crea un nuevo archivo con nombre `fname' y le escribe los `count' bytes del
 * bloque `blk'.
 * Retorna -1 si alguna operacion falla. Retorna 0 en caso exitoso.
 */
int crearArchivoData(size_t blk, size_t count, char *fname);

/* Aparea todos los files en la lista, fout es el filename del output.
 * Retorna la cantidad de lineas totales apareadas.
 * Retorna -1 en caso de fallos.
 */
int aparearFiles(t_list *fnames, char *fout);

/* Realiza el apareo efectivo de los FILES ya abiertos y escribe el resultado
 * en el ultimo de los FILES.
 * Retorna la cantidad de lineas totales apareadas.
 * Retorna -1 en caso de error de escritura/lectura a un FILE
 */
int realizarApareo(int nfiles, FILE *fs[nfiles]);

/* Crea y ejecuta un comando que pipea' los datos del archivo data_fname al
 * programa ejecutable exe_fname y vuelca el resultado en un archivo out_fname.
 * Retorna -1 si fallan o bien la creacion del comando o bien su ejecucion.
 * Retorna 0 en salida exitosa.
 */
int makeCommandAndExecute(char *data_fname, char *exe_fname, char *out_fname);

int conectarYCargar(int nquant, t_list *nodos, int **fds, char ***lns);
int apareoGlobal(t_list *nodos, char *fname);

void cleanWorkspaceFiles(int nfiles, char *fst, ...);

/* Recrea en el sistema local el archivo alojado en cada Nodo de la lista.
 * Retorna -1 si falla alguna operacion de la rutina. Sino retorna 0.
 */
int reproducirFiles(t_list *nodos);

/* Se conecta al nodo y obtiene y retorna los datos deserializados del FILE.
 * Retorna tambien el tamanio del file en *fsize.
 * Retorna NULL en caso del algun fallo.
 */
char *obtenerFileData(t_info_nodo *nodo, size_t *fsize);

/* Recibe varios filenames presuntamente ya ordenados y los aparea.
 * El primer parametro es la cantidad de files que se estan pasando.
 * El ultimo parametro si o si debe ser el filename de output del apareo.
 * Retorna la cantidad de lineas totales apareadas.
 * Retorna -1 si falla la apertura de alguno de los archivos.
 */
int aparearFiles_(int nfiles, char *fst, ...);

/* Ordena lexicograficamente las entradas de un puntero a lineas,
 * dada la cantidad de lineas a las que apunta el puntero.
 */
void sort(char **lines[], size_t line_count);

/* Retorna la cantidad de lineas del FILE f
 * En error retorna -1
 *
 * ** NO produce efectos sobre f :)
 */
size_t countLines(FILE *f);

/* Crea un char**: un puntero donde cada puntero es una linea del FILE f
 * Retorna ademas la dimension del char* (las file_lines)
 * En error retorna NULL y setea file_lines en 0
 *
 * ** NO produce efectos sobre f :)
 */
char **readFileIntoArray(FILE *f, size_t *file_lines);

/* A partir de char**: un puntero donde cada puntero es un string,
 * sobreescribe linea por linea sobre el FILE f.
 *
 * ** SI produce efectos sobre f, lo trunca zarpado y le escribe :)
 */
int writeArrayIntoFile(char **lines, int dim, const char *path);

#endif /* AUXILIARES_WORKER_H_ */
