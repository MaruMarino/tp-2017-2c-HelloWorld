#ifndef FUNCIONES_NET_H_
#define FUNCIONES_NET_H_

#include <commons/log.h>

/* Dados IP y puerto de destino, se trata de conectar a un servidor...
 * Crea y retorna el socket que permite la comunicacion con el servidor.
 * En caso de error retona -1.
 */
int establecerConexion(char *ip_dest, char *port_dest, t_log *log, int *control);

/* Crea un socket y lo bindea a un puerto para escuchar clientes entrantes,
 * Retorna el socket pero sin haberle hecho listen()!
 * En caso de error retorna -1
 */
int makeListenSock(char *port_listen, t_log *log, int *control);

/* Bloquea hasta lograr accept() con una conexion entrante.
 * Retorna el nuevo socket hecho para con el cliente.
 * En caso de error retorna -1
 */
int aceptar_conexion(int socket_in, t_log *log, int *control);

/* Envia un mensaje dado, usando un socket dado
 * En caso de error retorna -1
 */
int enviar(int socket_emisor, char *mensaje_a_enviar, t_log *log, int *control);

/* Recibe un mensaje, usando el socket dado - Respeta lo hablado ;)
 */
char *recibir(int socket_receptor, t_log *log, int *control);

#endif /* FUNCIONES_NET_H_ */
