#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "estructuras.h"
#include "conexion_master.h"
#include "conexion_fs.h"

extern t_configuracion *config;
extern t_list *workers;

void armar_workers()
{
	//acá habría que deserializar lo que me mande maru en el handshake y ponerle en disponibilidad la base
}

void calcular_disponibilidad()
{
	void disponibilidad(t_worker *worker)
	{
		int pwl = 0;
		if(strcmp(config->algortimo_bal,"CLOCK"))
		{
			int maxima_carga = get_maxima_carga();
			pwl = maxima_carga - worker->carga_actual;
		}
		worker->disponibilidad = config->base + pwl;
	}
	list_iterate(workers, disponibilidad);
}

int get_maxima_carga()
{
	int maximo = 0;
	int i = 0;
	int cantidad_workers = list_size(workers);

	while(cantidad_workers > i)
	{
		t_worker *worker = list_get(workers, i);

		if(worker->carga_actual > maximo)
			maximo = worker->carga_actual;

		i++;
	}
	return maximo;
}

int get_mayor_disponibilidad()
{
	int mayor = -10;
	int i = 0;
	int size = list_size(workers);

	while(i < size)
	{
		t_worker *worker = list_get(workers, i);

		if (worker->disponibilidad > mayor)
			mayor = worker->disponibilidad;

		i++;
	}

	return mayor;
}

int get_menor_carga(t_list *lista_auxiliar)
{
	int menor_carga = 10000000;
	int i = 0;
	int size = list_size(lista_auxiliar);

	while(size<i)
	{
		t_worker *worker = list_get(lista_auxiliar, i);

		if(worker->carga_actual < menor_carga)
			menor_carga = worker->carga_actual;

		i++;
	}

	return menor_carga;
}

void posicionar_clock()
{
	int mayor_disponibilidad = get_mayor_disponibilidad();

	bool _mayor_disponibilidad(t_worker *worker)
	{
		return worker->disponibilidad == mayor_disponibilidad;
	}

	t_list *lista_auxiliar = list_filter(workers, _mayor_disponibilidad);

	int menor_carga = get_menor_carga(lista_auxiliar);

	bool _menor_disponibilidad(t_worker *worker)
	{
		return worker->carga_actual == menor_carga;
	}

	t_worker *worker_ = list_find(workers, _menor_disponibilidad);
	worker_->clock = true;

	list_destroy(lista_auxiliar);
}

