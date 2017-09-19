/*
 * serializacion-yama-master.h
 *
 *  Created on: 17/9/2017
 *      Author: utnso
 */

#ifndef FUNCIONESCOMPARTIDAS_SERIALIZACION_YAMA_MASTER_H_
#define FUNCIONESCOMPARTIDAS_SERIALIZACION_YAMA_MASTER_H_

char *serializar_nodo(t_nodo *nodo, size_t *len);
t_nodo *deserializar_nodo (char *nodo_ser, size_t *len);
char *serializar_transformacion(t_transformacion *tran, size_t *len);
t_transformacion *deserializar_transformacion(char *tran);
char *serializar_redLocal(t_redLocal *red_local, size_t *len);
t_redLocal *deserializar_redLocal (char *red_local_ser);
char *serializar_redGlobal(t_redGlobal *red_global, size_t *len);
t_redGlobal *deserializar_redGlobal(char *red_ser);
char *serializar_almacenado(t_almacenado *almacenado, size_t *len);
t_almacenado *deserializar_almacenado(char *alm_ser);
char *serializar_estado_master(t_estado_master *estado_master, size_t *len);
t_estado_master *deserializar_estado_master(char *em_ser);

#endif /* FUNCIONESCOMPARTIDAS_SERIALIZACION_YAMA_MASTER_H_ */
