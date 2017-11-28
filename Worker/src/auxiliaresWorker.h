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

/* Dada una lista de nodos, se conecta a cada uno de ellos y carga en los
 * punteros *fds y **lns el file_descriptor y la primera linea de cada nodo.
 * Estos punteros son los parametros de retorno que interesan.
 * Ademas retorna -1 si ocurre algun fallo; retorna 0 en salida exitosa.
 */
int conectarYCargar(int nquant, t_list *nodos, int **fds, char ***lns);

/* Dado el IP y Puerto del FileSystem, le envia el archivo y espera respuesta
 * para el resultado del almacenamiento.
 * Retorna 0 en caso exitoso. Retorna != 0 en caso de fallo.
 */
int almacenarFileEnFilesystem(char *fs_ip, char *fs_port, t_file *file);

/* A partir de un path genera un t_file* con filename fn y sus datos binarios
 * Retorna NULL en caso de algun error. Sino retorna el t_file generado
 */
t_file *cargarFile(char *path, char *fn);

/* Establece conexion con IP y PUERTO (deberian ser del FileSystem),
 * realiza un handshake y le envia el file.
 * Retorna -1 en caso de fallo. Sino retorna el socket para el FileSystem.
 */
int enviarFile(char *fs_ip, char *fs_port, t_file *file);

off_t fsize(FILE *f);

void cleanWorkspaceFiles(int nfiles, char *fst, ...);

/* Envia al fd_m el cod_rta y llama exit() con ese mismo valor
 * Ademas limpia del entorno los archivos de datos y de ejecucion pertinentes
 */
void terminarEjecucion(int fd_m, int cod_rta, t_conf *conf, char *exe, char *dat);

#endif /* AUXILIARES_WORKER_H_ */
