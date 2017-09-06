#ifndef FUNCSOCKS_FUNCSOCKS_H_
#define FUNCSOCKS_FUNCSOCKS_H_

/* Dados IP y puerto de destino, se trata de conectar a un servidor...
 * Crea y retorna el socket que permite la comunicacion con el servidor.
 * En caso de error retona -1.
 */
int establecerConexion(char *ip_destino, char *port_destino);

/* Crea un socket y lo bindea a un puerto para escuchar clientes entrantes,
 * Retorna el socket pero sin haberle hecho listen()!
 * En caso de error retorna -1
 */
int makeListenSock(char *port_listen);

/* Bloquea hasta lograr accept() con una conexion entrante.
 * Retorna el nuevo socket hecho para con el cliente.
 * En caso de error retorna -1
 */
int makeCommSock(int socket_in);


#endif /* FUNCSOCKS_FUNCSOCKS_H_ */
