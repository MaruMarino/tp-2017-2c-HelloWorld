#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <commons/string.h>
#include <pthread.h>
#include "estructuras.h"

extern t_estadistica *estadistica;
extern pthread_mutex_t mutex_estadistica;

void imprimir_tiempo(char *mensaje, int tiempo);

void agregar_transformacion()
{
	pthread_mutex_lock(&mutex_estadistica);

	if(estadistica->transf_total == 0)
		*estadistica->inicio_trans = time(NULL);

	estadistica->transf_total ++;

	if(estadistica->transf_ejecutando > 0)
	{
		if(estadistica->transf_ejecutando == 1)
			estadistica->transf_paralelo = 2;
		else
			estadistica->transf_paralelo ++;
	}

	estadistica->transf_ejecutando ++;

	pthread_mutex_unlock(&mutex_estadistica);
}

void agregar_reduccion()
{
	pthread_mutex_lock(&mutex_estadistica);

	if(estadistica->reduc_total == 0)
		*estadistica->inicio_reduc_local = time(NULL);

	estadistica->reduc_total ++;

	if(estadistica->reduc_ejecutando > 0)
	{
		if(estadistica->reduc_ejecutando == 1)
			estadistica->reduc_paralelo = 2;
		else
			estadistica->reduc_paralelo ++;
	}

	estadistica->reduc_ejecutando ++;

	pthread_mutex_unlock(&mutex_estadistica);
}

void agregar_reduccion_global()
{
	pthread_mutex_lock(&mutex_estadistica);

	estadistica->reduc_glo_total ++;
	*estadistica->inicio_reduc_global = time(NULL);

	pthread_mutex_unlock(&mutex_estadistica);
}

void agregar_almacenamiento()
{
	pthread_mutex_lock(&mutex_estadistica);

	estadistica->alm_total ++;
	*estadistica->inicio_alm = time(NULL);

	pthread_mutex_unlock(&mutex_estadistica);
}

void quitar_transformacion()
{
	pthread_mutex_lock(&mutex_estadistica);

	estadistica->transf_ejecutando --;
	*estadistica->fin_trans = time(NULL);

	pthread_mutex_unlock(&mutex_estadistica);
}

void quitar_reduccion_local()
{
	pthread_mutex_lock(&mutex_estadistica);

	estadistica->reduc_ejecutando --;
	*estadistica->fin_reduc_local = time(NULL);

	pthread_mutex_unlock(&mutex_estadistica);
}

void quitar_reduccion_global()
{
	pthread_mutex_lock(&mutex_estadistica);

	*estadistica->fin_reduc_global = time(NULL);

	pthread_mutex_unlock(&mutex_estadistica);
}

void quitar_almacenamiento()
{
	pthread_mutex_lock(&mutex_estadistica);

	*estadistica->fin_alm = time(NULL);

	pthread_mutex_unlock(&mutex_estadistica);
}

void agregar_fallo_transf()
{
	pthread_mutex_lock(&mutex_estadistica);

	estadistica->transf_ejecutando --;
	*estadistica->fin_trans = time(NULL);
	estadistica->fallo_transf ++;

	pthread_mutex_unlock(&mutex_estadistica);
}

void agregar_fallo_reducc_local()
{
	pthread_mutex_lock(&mutex_estadistica);

	estadistica->reduc_ejecutando --;
	*estadistica->fin_reduc_local = time(NULL);
	estadistica->fallo_reduc_local ++;

	pthread_mutex_unlock(&mutex_estadistica);
}

void agregar_fallo_reducc_global()
{
	pthread_mutex_lock(&mutex_estadistica);

	*estadistica->fin_reduc_global = time(NULL);
	estadistica->fallo_reduc_global ++;

	pthread_mutex_unlock(&mutex_estadistica);
}

void agregar_fallo_almac()
{
	pthread_mutex_lock(&mutex_estadistica);

	*estadistica->fin_alm = time(NULL);
	estadistica->fallo_almacenamiento ++;

	pthread_mutex_unlock(&mutex_estadistica);
}

void mostrar_estadisticas()
{
	printf("Ejecucion finalizada\n");

	int tiempo_total_job = *estadistica->fin_alm - *estadistica->inicio_trans;
	imprimir_tiempo("Tiempo total de job:", tiempo_total_job);

	printf("\n");
	printf("Etapa transformacion\n");
	printf("	Total procesados: %d\n", estadistica->transf_total);
	printf("	Total procesados con error: %d\n", estadistica->fallo_transf);
	printf("	Maximo ejecutados en paralelo: %d\n", estadistica->transf_paralelo);
	int tiempo_prom_transf = (*estadistica->fin_trans - *estadistica->inicio_trans) / estadistica->transf_total;
	imprimir_tiempo("	Tiempo promedio procesamiento:", tiempo_prom_transf);

	printf("\n");
	printf("Etapa reduccion local\n");
	printf("	Total procesados: %d\n", estadistica->reduc_total);
	printf("	Total procesados con error: %d\n", estadistica->fallo_reduc_local);
	printf("	Maximo ejecutados en paralelo: %d\n", estadistica->reduc_paralelo);
	int tiempo_prom_red = (*estadistica->fin_reduc_local - *estadistica->inicio_reduc_local) / estadistica->reduc_total;
	imprimir_tiempo("	Tiempo promedio procesamiento:", tiempo_prom_red);

	printf("\n");
	printf("Etapa reduccion global\n");
	printf("	Total procesados: %d\n", estadistica->reduc_glo_total);
	printf("	Total procesados con error: %d\n", estadistica->fallo_reduc_global);
	int tiempo_prom_red_glo = (*estadistica->fin_reduc_global - *estadistica->inicio_reduc_global);
	imprimir_tiempo("	Tiempo promedio procesamiento:", tiempo_prom_red_glo);

	printf("\n");
	printf("Etapa almacenamiento\n");
	printf("	Total procesados: %d\n", estadistica->alm_total);
	printf("	Total procesados con error: %d\n", estadistica->fallo_almacenamiento);
	int tiempo_prom_alm = (*estadistica->fin_alm - *estadistica->inicio_alm);
	imprimir_tiempo("	Tiempo promedio procesamiento:", tiempo_prom_alm);

	printf("\n");
}

void imprimir_tiempo(char *mensaje, int tiempo)
{
	int minutos = tiempo/60;
	int segundos = tiempo%60;

	printf("%s %d min %d seg\n", mensaje, minutos, segundos );
}
