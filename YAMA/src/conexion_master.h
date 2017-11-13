/*
 * conexion_master.h
 *
 *  Created on: 8/9/2017
 *      Author: utnso
 */

#ifndef CONEXION_MASTER_H_
#define CONEXION_MASTER_H_


void manejar_respuesta(int codigo);
void realizar_handshake_master(int socket_);
void manejo_conexiones();
void enviar_peticion_transformacion(int socket_);
char *generar_nombre_red_global(int mast, char* nod);
void matar_master(int socket_);

#endif /* CONEXION_MASTER_H_ */
