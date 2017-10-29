#ifndef CLOCKS_H_
#define CLOCKS_H_

void armar_workers(char *);
void calcular_disponibilidad();
int get_maxima_carga();
int get_mayor_disponibilidad();
int get_menor_carga(t_list *lista_auxiliar);
void posicionar_clock();
void ejecutar_clock(t_list *archivo_bloques, int cant_bloques, int _socket);
int _get_index_clock();
void sumar_disponibilidad_base();
void enviar_reduccion_local(estado_tr, socket_);

#endif /* CLOCKS_H_ */
