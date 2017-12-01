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
void enviar_reduccion_local(t_estado_master *estado_tr, int socket_);
t_worker *find_worker(char *nodo);
t_master *find_master(int sockt);
void replanificar(t_estado_master *estado_tr, int socket_);
void reduccion_global(int socket_, t_estado_master *estado_tr);
void armar_reduccion_local(int sz, t_master *master_, t_estado *est, t_estado_master *estado_tr);
void armar_reduccion_global(int sz, t_master *master_, t_estado *est, t_estado_master *estado_tr);
void recalcular_cargas(int master);
void armar_transformacion_replanificada2(t_estado *estado, int socket_, t_list *transformaciones);

#endif /* CLOCKS_H_ */
