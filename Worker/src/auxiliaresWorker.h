#ifndef AUXILIARES_WORKER_H_
#define AUXILIARES_WORKER_H_

#include <stdio.h>

#include "configuracionWorker.h"

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

/* Se conecta a todos los nodos y va recibiendo de a una linea de cada uno.
 * Aparea todos los inputs recibidos en el FILE de salida fname.
 * Retorna -1 si ocurre algun fallo; retorna 0 en salida exitosa.
 */
int apareoGlobal(t_list *nodos, char *fname);

/* Dado el IP y Puerto del FileSystem, se le conecta y le envia el archivo
 * del tipo t_file que corresponde con el filename fname.
 * Retorna -1 si falla el proceso. Retorna 0 en caso exitoso.
 */
int almacenarFileEnFilesystem(char *fs_ip, char *fs_port, char *fname);

/* Dada una lista de nodos, se conecta a cada uno de ellos y carga en los
 * punteros *fds y **lns el file_descriptor y la primera linea de cada nodo.
 * Estos punteros son los parametros de retorno que interesan.
 * Ademas retorna -1 si ocurre algun fallo; retorna 0 en salida exitosa.
 */
int conectarYCargar(int nquant, t_list *nodos, int **fds, char ***lns);

/* A partir de un path genera un t_file* con los datos pertinentes */
t_file *cargarFile(char *fname);

off_t fsize(FILE* f);

void cleanWorkspaceFiles(int nfiles, char *fst, ...);

/* Envia al fd_m el cod_rta y llama exit() con ese mismo valor */
void terminarEjecucion(int fd_m, int cod_rta, t_conf *conf);

#endif /* AUXILIARES_WORKER_H_ */
