#ifndef FUNCIONES_NET_H_
#define FUNCIONES_NET_H_

#include <commons/log.h>
typedef struct {
    void * buffer;
    size_t sizeBuffer;
}message;

typedef struct {
    char letra;
    int codigo;
    size_t sizeData;
}header;

/* Dados IP y puerto de destino, se trata de conectar a un servidor...
 * Crea y retorna el socket que permite la comunicacion con el servidor.
 * En caso de error retona -1.
 */
int establecerConexion(char *ip_dest, char *port_dest, t_log *log, int *control);

/* Crea un socket, lo bindea a un puerto, y lo pone en listen() sobre el mismo.
 * En caso de error retorna -1. Sino retorna el socket creado
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

/* Envia una estructura de tipo menssage, usando el socket dado
 */
int enviar_message(int socket_emisor, message *message, t_log *log, int *control);

/* como enviar_message pero se banca interrupciones */
int enviar_messageIntr(int socket, message *message, t_log *log, int *control);

/* Recibe un mensaje, usando el socket dado - Respeta lo hablado ;)
 */
char *recibir(int socket_receptor, t_log *log, int *control);

/*
 * @return struct message
 * @param puntero de struct header
 * @param puntero de data a enviar
 * ejemeplo:
 *
 *  char * mjs = "El mensajito test"
 *
 *  header data;
    data.letra  = 'D';
    data.codigo = 1020;
    data.sizeData = strlen(mjs);

    message *buffer = createMessage(&data,mjs);

    send(socketCliente,buffer->buffer,buffer->sizeBuffer,0)

    *********************************************************
    ejemplo 2: structtura
 *
 *  x algunaStruct;
 *
 *  header data;
    data.letra  = 'D';
    data.codigo = 1020;
    data.sizeData = sizeof(algunaStruct);

    message *buffer = createMessage(&data,&algunaStruct);

    send(socketCliente,buffer->buffer,buffer->sizeBuffer,0)
 *
 * */
message * createMessage(header *head, void *data);


/*
 * @return puntero de buffer, NULL en caso de que fallo,
 * @param socket
 * @param puntero a una struct header al terminar este proceso la struct header va estar cargado con lo recibido
 * @param status : -1 error recibiendo/ 0 se desconecto/ else bytes recibidos
 * example:
 *
 *  Head header;
 *  int estado;
 *
    void * buffer = getMessage(socket,&header,&estado);
    if(buffer == NULL){
        printf("Error al recicbir el msj");
        return -1;
    }

    write(STDOUT_FILENO,buffer,header.sizeData);
 *
 * */
void *getMessage(int socket,header *head,int *status);

/* como getMessage pero se banca interrupciones
 * Retorna -1 si ocurrio algun otro problema inmanejable;
 */
char *getMessageIntr(int socket, header *head, int *status);

/* como getMessageIntr pero no bloquea en recv().
 * Retorna -2 si no hay nada que esperar en el buffer de recv();
 * Retorna -1 si ocurrio algun otro problema inmanejable;
 */
char *getMessageIntrNB(int socket, header *head, int *status);

/* recv con esteroides.
 * Recibe el *len completo y lo almacena en *buffer, incluso si ocurre alguna
 * interrupcion. Similar a usar WAITALL para el recv(), pero todavia mejor.
 * Aunque no es infalible... Retorna -1 si falla; *len en caso exitoso.
 * **buffer y *len son parametros de retorno.
 * Retorna -2 si falla por EWOULDBLOQ -> solo si en flags pasamos MSG_DONTWAIT
 */
int recvall_intr(int sock, char **buffer, size_t *len, int flags);

/* Envia todos los bytes del buffer que pueda, incluso si se interrumpe la
 * syscall.
 * Si falla por algun otro motivo, retorna -1; sino retorna 0.
 * len es variable de retorno para el size total enviado.
 */
int sendall_intr(int sock, char *buff, size_t *len, int flags);

#endif /* FUNCIONES_NET_H_ */
