#include "funcionesNet.h"

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "mensaje.h"
#include "log.h"
#include <commons/string.h>
#include <commons/log.h>

void error_sockets(t_log *log, int *controlador, char *proceso);

/* Auxiliar para configurar la conexion en red, sea para cliente o servidor */
void setupHints(struct addrinfo *hints, int flags) {
    memset(hints, 0, sizeof *hints);
    hints->ai_family = AF_INET;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = flags;
}

int establecerConexion(char *ip_dest, char *port_dest, t_log *log, int *control) {
    int stat, sock_dest;
    struct addrinfo hints, *destInfo;
    setupHints(&hints, 0);
    *control = 0;

    if ((stat = getaddrinfo(ip_dest, port_dest, &hints, &destInfo)) < 0) {
    	*control = 11;
    	error_sockets(log, control, "");
        return -1;
    }

    if ((sock_dest = socket(destInfo->ai_family, destInfo->ai_socktype, destInfo->ai_protocol)) == -1) {
        *control = 1;
        error_sockets(log, control, "");
    }

    retry_connect:
    if (connect(sock_dest, destInfo->ai_addr, destInfo->ai_addrlen) == -1) {
    	if (errno == EINTR) goto retry_connect;
        *control = 2;
        error_sockets(log, control, "");
    }

    freeaddrinfo(destInfo);
    return sock_dest;
}

int makeListenSock(char *port_listen, t_log *log, int *control) {
    int stat, sock_listen;
    struct addrinfo hints, *serverInfo;
    setupHints(&hints, AI_PASSIVE);
    int BACKLOG = 20; //Cantidad de conexiones maximas
    int yes = 1;
    *control = 0;

    if ((stat = getaddrinfo(NULL, port_listen, &hints, &serverInfo)) != 0) {
    	*control = 11;
    	error_sockets(log, control, port_listen);
        return -1;
    }

    if ((sock_listen = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol)) == -1) {
        *control = 1;
        error_sockets(log, control, "");
        return -1;
    }
    if (setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        *control = 10;
        error_sockets(log, control, "");
        return -1;
    }

    if (bind(sock_listen, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) {
        *control = 4;
        error_sockets(log, control, "");
        return -1;
    }

    //Listening socket
    if (listen(sock_listen, BACKLOG) != 0) {
        *control = 5;
        error_sockets(log, control, "");
        return -1;
    }

    freeaddrinfo(serverInfo);
    return sock_listen;
}

int aceptar_conexion(int socket_in, t_log *log, int *control) {
    int sock_comm;
    struct sockaddr_in clientAddr;
    socklen_t clientSize = sizeof(clientAddr);
    *control = 0;

    retry_accept:
    if ((sock_comm = accept(socket_in, (struct sockaddr *) &clientAddr, &clientSize)) == -1) {
    	if (errno == EINTR) goto retry_accept;
        *control = 6;
        error_sockets(log, control, "");
    }

    return sock_comm;
}

int enviar(int socket_emisor, char *mensaje_a_enviar, t_log *log, int *control) {
    int ret;
    *control = 0;
    size_t sbuffer = (size_t) string_length(mensaje_a_enviar);

    char *buffer = string_substring(mensaje_a_enviar, 0, (int) sbuffer);

    if ((ret = send(socket_emisor, buffer, sbuffer, MSG_NOSIGNAL)) < 0) {
        *control = 7;
        char *emisor = string_itoa(socket_emisor);
        error_sockets(log, control, emisor);
        free(emisor);
    }

    free(buffer);
    return ret;
}

int enviar_message(int socket_emisor, message *message, t_log *log, int *control)
{
    int ret;
    *control = 0;

    if ((ret = send(socket_emisor, message->buffer, message->sizeBuffer, MSG_NOSIGNAL)) < 0) {
        *control = 7;
        char *emisor = string_itoa(socket_emisor);
        error_sockets(log, control, emisor);
        free(emisor);
    }

    return ret;
}

int enviar_messageIntr(int socket, message *message, t_log *log, int *control){

    int ret;
    *control = 0;

    if ((ret = sendall_intr(socket, message->buffer, &message->sizeBuffer, MSG_NOSIGNAL)) == -1){
        *control = 7;
        char *emisor = string_itoa(socket);
        error_sockets(log, control, emisor);
        free(emisor);
    }

    return ret;
}


char *recibir(int socket_receptor, t_log *log, int *control) {
    int ret;
    *control = 0;
    char *buffer = malloc(13);

    if ((ret = recv(socket_receptor, buffer, 13, MSG_WAITALL)) <= 0) {
        if (ret == 0) {
            *control = 8;
            char *receptor = string_itoa(socket_receptor);
            error_sockets(log, control, receptor);
            free(receptor);
        } else {
            *control = 1;
            error_sockets(log, control, "");
        }
    }

    char *str_size = string_substring(buffer, 3, 10);

    int size = atoi(str_size);
    free(str_size);
    char *resto_mensaje;

    if (size > 0) {
        resto_mensaje = malloc((size_t) size);

        if ((ret = recv(socket_receptor, resto_mensaje, (size_t) size, 0)) <= 0) {
            if (ret == 0) {
                *control = 8;
                char *receptor = string_itoa(socket_receptor);
                error_sockets(log, control, receptor);
                free(receptor);
            } else {
                *control = 1;
                error_sockets(log, control, "");
            }
        }
    }

    char *buffer_aux = string_substring(buffer, 0, 13);
    char *resto_sub = string_substring(resto_mensaje, 0, size);
    string_append(&buffer_aux, resto_sub);

    free(resto_sub);
    free(resto_mensaje);
    free(buffer);

    return buffer_aux;
}

void error_sockets(t_log *log, int *controlador, char *proceso) {
    switch (*controlador) {
        case 1:
            escribir_error_log(log, "Error creando socket");
            break;
        case 2:
            escribir_error_log(log, "Error conectando socket");
            break;
        case 3:
            escribir_error_log(log, "Error creando socket server");
            break;
        case 4:
            escribir_error_log(log, "Error bindeando socket server");
            break;
        case 5:
            escribir_error_log(log, "Socket server, error escuchando");
            break;
        case 6:
            escribir_error_log(log, "Error aceptando conexion");
            break;
        case 7:
            escribir_log_error_compuesto(log, "Error al enviar mensaje a: ", proceso);
            break;
        case 8:
            escribir_log_error_compuesto(log, "Error, socket desconectado: ", proceso);
            break;
        case 9:
            escribir_log_error_compuesto(log, "Error recibiendo mensaje de: ", proceso);
            break;
        case 10:
            escribir_error_log(log, "No se pudieron setear opciones a socket");
            break;
        case 11:
            escribir_log_error_compuesto(log, "Fallo getaddrinfo() sobre puerto", proceso);
            break;
    }
}

message *createMessage(header *head, void *data) {
    message *ElMensaje = malloc(sizeof(message));
    ElMensaje->sizeBuffer = (sizeof(header) + head->sizeData);
    ElMensaje->buffer = malloc(ElMensaje->sizeBuffer);
    memcpy(ElMensaje->buffer, head, sizeof(header));
    memcpy((ElMensaje->buffer + sizeof(header)), data, head->sizeData);
    return ElMensaje;
}

void *getMessage(int socket, header *head,int *status) {

	if ((*status = recv(socket, head, sizeof(header), 0)) <= 0)
        return NULL;

    void *buffer = malloc(head->sizeData);
    if(head->sizeData > 0){
    	if ((*status = recv(socket, buffer, head->sizeData, 0)) <= 0) {
    		free(buffer);
			return NULL;
    	}
	}

    return buffer;
}

char *getMessageIntrNB(int socket, header *head, int *status){

	char *buffer;
	size_t len = sizeof *head;

	*status = recvall_intr(socket, (char **) &head, &len, MSG_DONTWAIT);
	if (*status == -1 || *status == -2 || *status == 0)
        return NULL;

    buffer = malloc(head->sizeData);
    if ((*status = recvall_intr(socket, &buffer, &head->sizeData, 0)) == -1 ||
    	 *status == 0){
    	free(buffer);
    	return NULL;
    }

    return buffer;
}

char *getMessageIntr(int socket, header *head, int *status){

	char *buffer;
	size_t len = sizeof *head;

	if ((*status = recvall_intr(socket, (char **) &head, &len, 0)) == -1 ||
		 *status == 0)
		return NULL;

	buffer = malloc(head->sizeData);
	if ((*status = recvall_intr(socket, &buffer, &head->sizeData, 0)) == -1 ||
			*status == 0){
		free(buffer);
		return NULL;
	}

    return buffer;
}

int recvall_intr(int sock, char **buffer, size_t *len, int flags){

	int status = 0;
	size_t total = 0;
	size_t left = *len;

	while (total < *len){

		if ((status = recv(sock, *buffer + total, left, flags)) == -1){
			if (errno == EWOULDBLOCK)
				return -2;
			if (errno != EINTR)
				break;
		}else if(!status)
			return 0;

		total += (size_t) status;
		left -= (size_t) status;
	}

	*len = total;
	return (status == -1)? -1 : (int) total;
}

int sendall_intr(int sock, char *buff, size_t *len, int flags){

	int stat;
	size_t total = 0;
	size_t left  = *len;

	while (total < *len){

		try_send:
		if ((stat = send(sock, buff + total, left, flags)) == -1){
			if (errno == EINTR) goto try_send;
			//perror("No se pudo sendall'ear el paquete. error");
			break;
		}
		total += (size_t) stat;
		left  -= (size_t) stat;
	}

	*len = total;
	return (stat == -1)? -1 : 0;
}
