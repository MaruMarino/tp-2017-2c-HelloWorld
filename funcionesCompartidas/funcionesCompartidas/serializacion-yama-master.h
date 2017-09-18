/*
 * serializacion-yama-master.h
 *
 *  Created on: 17/9/2017
 *      Author: utnso
 */

#ifndef FUNCIONESCOMPARTIDAS_SERIALIZACION_YAMA_MASTER_H_
#define FUNCIONESCOMPARTIDAS_SERIALIZACION_YAMA_MASTER_H_

char *serializar_nodo(t_nodo *nodo, size_t *len);
t_nodo *deserializar_nodo (char *nodo_ser);

#endif /* FUNCIONESCOMPARTIDAS_SERIALIZACION_YAMA_MASTER_H_ */
