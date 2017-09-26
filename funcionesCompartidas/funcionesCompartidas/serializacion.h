#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

/* A partir de 'info' arma un char* con una longitud de len bytes */
char *serializar_info_trans(t_info_trans *info, size_t *len);

/* A partir de un char* lo traduce y crea en un t_info_trans */
t_info_trans *deserializar_info_trans(char *info_serial);

char *serializar_info_redLocal(t_info_redLocal *info, size_t *len);
t_info_redLocal *deserializar_info_redLocal(char *info_serial);

char *serializar_info_redGlobal(t_info_redGlobal *info, size_t *len);
t_info_redGlobal *deserializar_info_redGlobal(char *info_serial);

char *serializar_info_redGlobalSub(t_info_redGlobalSub *info, size_t *len);
t_info_redGlobalSub *deserializar_info_redGlobalSub(char *info_serial);


#endif /* SERIALIZACION_H_ */
