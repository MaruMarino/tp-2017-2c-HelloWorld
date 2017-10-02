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

char *serializar_stream(char *bytes, size_t bytelen, size_t *len);
char *deserializar_stream(char *bytes_serial, size_t *bytelen);

char *serializar_FName(char *fn, size_t *len);
char *deserializar_FName(char *fname_serial);

#endif /* SERIALIZACION_H_ */
