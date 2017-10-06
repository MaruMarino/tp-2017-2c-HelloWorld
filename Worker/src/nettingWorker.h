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
