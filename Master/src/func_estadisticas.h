#ifndef FUNC_ESTADISTICAS_H_
#define FUNC_ESTADISTICAS_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <commons/string.h>
#include "estructuras.h"

extern t_estadistica *estadistica;

void agregar_transformacion();
void agregar_reduccion();
void agregar_reduccion_global();
void agregar_almacenamiento();
void quitar_transformacion();
void quitar_reduccion_local();
void quitar_reduccion_global();
void quitar_almacenamiento();
void agregar_fallo_transf();
void agregar_fallo_reducc_local();
void agregar_fallo_reducc_global();
void agregar_fallo_almac();
void mostrar_estadisticas();

#endif /* FUNC_ESTADISTICAS_H_ */
