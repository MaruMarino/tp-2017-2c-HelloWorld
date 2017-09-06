#include "funcionesNet.h"

#include <stdio.h>
#include <netdb.h>
#include <string.h>

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

	if ((stat = getaddrinfo(NULL, port_listen, &hints, &serverInfo)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(stat));
		return -1;
	}

	if ((sock_listen = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol)) == -1){
		perror("No se pudo crear socket. error.");
		return -1;
	}

	if (bind(sock_listen, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1){
		perror("Fallo binding con socket. error");
		return -1;
	}

	freeaddrinfo(serverInfo);
	return sock_listen;
}

int makeCommSock(int socket_in){

	int sock_comm;
	struct sockaddr_in clientAddr;
	socklen_t clientSize = sizeof(clientAddr);

	if ((sock_comm = accept(socket_in, (struct sockaddr*) &clientAddr, &clientSize)) == -1){
		perror("Fallo accept del socket entrada. error");
		return -1;
	}

	return sock_comm;
}
