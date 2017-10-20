#ifndef CLOCKS_H_
#define CLOCKS_H_

void armar_workers();
void calcular_disponibilidad();
int get_maxima_carga();
int get_mayor_disponibilidad();
int get_menor_carga(t_list *lista_auxiliar);
void posicionar_clock();

#endif /* CLOCKS_H_ */
