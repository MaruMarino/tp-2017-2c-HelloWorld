/*
 * FS_conexiones.h
 *
 *  Created on: 8/9/2017
 *      Author: utnso
 */

#ifndef FS_CONEXIONES_H_
#define FS_CONEXIONES_H_

void manejo_conexiones();
int realizar_handshake(int nuevo_socket);
int direccionar(int socket_rec);

#endif /* FS_CONEXIONES_H_ */
