#include <funcionesCompartidas/funcionesNet.h>

/* Envia un header minimal ('W' y codigo 0, sin datos) al fd_proc
 * En caso de fallo retorna -1; sino retorna 0.
 */
int responderHandshake(int fd_proc);

/* Compara los datos del header contra el proc y cod pasados por parametro.
 * Retorna 0 si coinciden, -1 si al menos proc discrepa, -2 si cod discrepa.
 */
int verificarConexion(header head, char proc, int cod);

/* Envia un header minimal ('W' y codigo 0, sin datos) al fd_proc y espera a
 * recibir una respuesta del fd_proc; por ultimo compara la respuesta contra el
 * header esperado.
 * Retorna negativo si falla alguna parte del procedimiento. Sino retorna 0.
 */
int realizarHandshake(int fd_proc, char proc);

/* Avisa al Master en fd_m el resultado de la operacion a traves del cod_rta */
void enviarResultado(int fd_m, int cod_rta);

/* Bloquea hasta lograr accept() con una conexion entrante u ocurra una INTR
 * En caso de INTRE, setea EINTR en *control y retorna -1,
 * En caso de otros errores retorna -1,
 * Retorna el nuevo socket hecho para con el cliente en caso exitoso.
 */
int aceptar_conexion_intr(int socket_in, int *control);
