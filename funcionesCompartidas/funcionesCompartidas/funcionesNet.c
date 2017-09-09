#include "funcionesNet.h"

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <commons/string.h>

/* Auxiliar para configurar la conexion en red, sea para cliente o servidor */
void setupHints(struct addrinfo *hints, int flags){
    memset(hints, 0, sizeof *hints);
	hints->ai_family = AF_INET;
	hints->ai_socktype = SOCK_STREAM;
	hints->ai_flags = flags;
}

int establecerConexion(char *ip_dest, char *port_dest){

	int stat, sock_dest;
	struct addrinfo hints, *destInfo;
	setupHints(&hints, 0);

	if ((stat = getaddrinfo(ip_dest, port_dest, &hints, &destInfo)) < 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(stat));
		return -1;
	}

	if ((sock_dest = socket(destInfo->ai_family, destInfo->ai_socktype, destInfo->ai_protocol)) == -1){
		perror("No se pudo crear socket. error.");
		return -1;
	}

	if (connect(sock_dest, destInfo->ai_addr, destInfo->ai_addrlen) == -1){
		perror("No se pudo establecer conexion, fallo connect(). error");
		return -1;
	}

	freeaddrinfo(destInfo);

	return sock_dest;
}

int makeListenSock(char *port_listen){

	int stat, sock_listen;
	struct addrinfo hints, *serverInfo;
	setupHints(&hints, AI_PASSIVE);
	int BACKLOG = 20; //Cantidad de conexiones maximas

	if ((stat = getaddrinfo(NULL, port_listen, &hints, &serverInfo)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(stat));
		return -1;
	}

	if ((sock_listen = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol)) == -1){
		perror("No se pudo crear socket. error.");
		return -1;
	}
	int yes = 1;
	setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	printf("HOLA\n");
	if (bind(sock_listen, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1){
		perror("Fallo binding con socket. error");
		return -1;
	}

	//Listening socket
	if (listen(sock_listen, BACKLOG) != 0) {
	//	*controlador = 5;
	//	error_sockets(controlador, "");
	}

	freeaddrinfo(serverInfo);
	return sock_listen;
}

int aceptar_conexion(int socket_in){

	int sock_comm;
	struct sockaddr_in clientAddr;
	socklen_t clientSize = sizeof(clientAddr);

	if ((sock_comm = accept(socket_in, (struct sockaddr*) &clientAddr, &clientSize)) == -1){
		perror("Fallo accept del socket entrada. error");
		return -1;
	}

	return sock_comm;
}

int enviar(int socket_emisor, char *mensaje_a_enviar)
{
	int ret;
	size_t sbuffer = (size_t)string_length(mensaje_a_enviar);

	char *buffer = string_substring(mensaje_a_enviar, 0, (int)sbuffer);

	if ((ret = send(socket_emisor, buffer, sbuffer, MSG_NOSIGNAL)) < 0) {
		perror("No se pudo enviar el mensaje");
		return -1;
	}

	free(buffer);
	return ret;
}

char *recibir(int socket_receptor)
{
	int ret;
	char *buffer = malloc(13);

	if((ret = recv(socket_receptor, buffer, 13, MSG_WAITALL)) <= 0)
	{
		perror("No se pudo recibir el mensaje");
	}

	char *str_size = string_substring(buffer, 3, 10);

	int size = atoi(str_size);
	free(str_size);
	char *resto_mensaje;

	if(size>0)
	{
		resto_mensaje = malloc((size_t)size);
		recv(socket_receptor, resto_mensaje, (size_t)size, 0);
	}

	char *buffer_aux = string_substring(buffer,0,13);
	char *resto_sub = string_substring(resto_mensaje,0,size);
	string_append(&buffer_aux, resto_sub);

	free(resto_sub);
	free(resto_mensaje);
	free(buffer);

	return buffer_aux;
}
