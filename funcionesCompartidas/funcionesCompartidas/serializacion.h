#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

/* A partir de 'info' arma un char* con una longitud de len bytes */
char *serializar_info_trans(t_info_trans *info, size_t *len);

/* A partir de un char* lo traduce y crea en un t_info_trans */
t_info_trans *deserializar_info_trans(char *info_serial);

#endif /* SERIALIZACION_H_ */
