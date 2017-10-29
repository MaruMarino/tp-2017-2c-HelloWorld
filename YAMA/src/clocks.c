#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/serializacion.h>
#include <funcionesCompartidas/serializacion_yama_master.h>
#include <funcionesCompartidas/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "estructuras.h"
#include "conexion_master.h"
#include "conexion_fs.h"
#include "clocks.h"

extern t_configuracion *config;
extern t_list *workers;
extern t_log *yama_log;

void armar_workers(char *rta)
{
	t_list *nodos_aux = deserializar_lista_nodos(rta);

	void _armar_workers(t_nodo *nodo)
	{
		t_worker *worker = malloc(sizeof(t_worker));
		worker->bloques = list_create();
		worker->carga_actual = 0;
		worker->disponibilidad = config->base;
		worker->clock = false;
		worker->nodo = nodo;

		list_add(workers, worker);
	}
	list_iterate(nodos_aux, (void *)_armar_workers);
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
	list_iterate(workers,(void *) disponibilidad);
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

	while(size > 0)
	{
		t_worker *worker = list_get(lista_auxiliar, i);

		if(worker->carga_actual < menor_carga)
			menor_carga = worker->carga_actual;

		i++;
		size--;
	}

	return menor_carga;
}

void posicionar_clock()
{
	int mayor_disponibilidad = get_mayor_disponibilidad();

	t_list *lista_auxiliar = list_create();
	int size_l = list_size(workers);
	int i;

	for(i = 0 ; i < size_l ; i++)
	{
		t_worker *worker = list_get(workers, i);
		if(worker->disponibilidad == mayor_disponibilidad)
		{
			list_add(lista_auxiliar, worker);
		}
	}

	int menor_carga = get_menor_carga(lista_auxiliar);

	bool _menor_carga(t_worker *worker)
	{
		return worker->carga_actual == menor_carga;
	}

	t_worker *worker_ = list_find(workers, (void *)_menor_carga);
	worker_->clock = true;

	list_destroy(lista_auxiliar);
}

int _get_index_clock()
{
	int index_ = 0;
	int i = 1;

	while(i)
	{
		t_worker *worker = list_get(workers, index_);

		if(!worker->clock)
			index_ ++;
		else
			i = 0;
	}

	return index_;
}

void sumar_disponibilidad_base()
{
	int w_size = list_size(workers);
	int i;
	for(i = 0; i < w_size; i++)
	{
		t_worker *wk = list_get(workers, i);
		wk->disponibilidad += config->base;
	}
}

t_worker *get_worker(t_list *archivo, int n_bloque)
{
	t_worker *worker = NULL;
	int clock;

	int l_size = list_size(archivo);

	clock = _get_index_clock();

	t_list *lista_aux = list_create();
	int i;
	for(i = 0; i < l_size; i++)
	{
		t_bloque *bl = list_get(archivo, i);

		if(bl->n_bloque_archivo == n_bloque)
			list_add(lista_aux, bl);
	}

	bool _nodo_bloque(t_worker *worker_aux)
	{
		t_bloque *bl1 = list_get(lista_aux, 0);
		t_bloque *bl2 = list_get(lista_aux, 1);
		int resultado_parcial = strcmp(bl1->nodo, worker_aux->nodo->nodo) || strcmp(bl2->nodo, worker_aux->nodo->nodo);
		int otro_resultado = worker_aux->clock == true;

		return (resultado_parcial && otro_resultado);
	}

	worker = list_find(workers,(void *) _nodo_bloque);

	int w_size = list_size(workers);
	if(worker != NULL)
	{
		int next_index;
		bool _bloque_archivo(t_bloque *bl2)
		{
			return !strcmp(bl2->nodo, worker->nodo->nodo);
		}
		t_bloque *bl = list_find(lista_aux, (void *)_bloque_archivo);

		list_add(worker->bloques, bl);
		worker->clock = false;
		worker->carga_actual++;

		//if(worker->disponibilidad > 0)
			worker->disponibilidad--;
		//else
			//worker->disponibilidad = config->base;
		//if ((clock + 1) == w_size)
			//next_index = 0;
		//else
			//next_index = clock + 1;
		next_index = (clock + 1) % w_size;

		t_worker *w_next_clock = list_get(workers, next_index);

		if(worker->disponibilidad > 0)
			w_next_clock->clock = true;
		else
		{
			w_next_clock->disponibilidad += config->base;
			int next_index2 = (next_index + 1) % w_size;
			int encontrado = 0;
			while(!encontrado)
			{
				t_worker * worker2 = list_get(workers, next_index2);

				if (worker2->disponibilidad > 0)
				{
					encontrado = 1;
					worker2->clock = true;
				}
				else
					next_index2 = (clock + 1) % w_size;
			}
		}
	}else
	{
		int next_index = (clock + 1) % w_size;
		int encontrado = 0;
		int vuelta_completa = 0;
		int cant = 0;
		while(!encontrado && !vuelta_completa)
		{
			t_worker * worker2 = list_get(workers, next_index);

			bool _bloque(t_bloque *bl)
			{
				return bl->n_bloque_archivo == n_bloque;
			}
			if (list_any_satisfy(worker2->bloques, _bloque) && worker2->disponibilidad > 0)
			{
				encontrado = 1;
				worker2->disponibilidad --;
				worker2->carga_actual++;
			}
			else
			{
				next_index = (clock + 1) % w_size;
				cant ++;
				if(cant == w_size)
					vuelta_completa = 1;
			}
		}
		if(vuelta_completa)
		{
			sumar_disponibilidad_base();
			return get_worker(archivo, n_bloque);
		}
	}

	list_destroy(lista_aux);

	return worker;
}

void obtener_nodo_transformacion(t_list *archivo, t_list *transformaciones, int bloque)
{
	t_worker *worker = get_worker(archivo, bloque);
	int ind = list_size(worker->bloques) - 1;
	t_bloque *bloque_ = list_get(worker->bloques, ind);
	t_transformacion *transf = malloc(sizeof(t_transformacion));
	transf->bloque = bloque_->n_bloque_archivo;
	transf->bytes = bloque_->bytes;
	transf->nodo = worker->nodo;
	transf->temporal = "prueba" ;

	list_add(transformaciones, transf);
}

void ejecutar_clock(t_list *archivo_bloques, int cant_bloques, int _socket)
{
	escribir_log(yama_log, "Empieza el algoritmo de pre-planificación");
	int i = 0;
	calcular_disponibilidad();
	posicionar_clock();

	t_list *transformaciones = list_create();
	while(cant_bloques > 0)
	{
		obtener_nodo_transformacion(archivo_bloques, transformaciones, i);

		i++;
		cant_bloques--;
	}

	header head;
	head.codigo = 1;
	head.letra = 'Y';
	int control;

	char *transformaciones_ser = serializar_lista_transformacion(transformaciones, &head.sizeData);

	message *mensaje = createMessage(&head, transformaciones_ser);
	enviar_message(_socket, mensaje, yama_log, &control);
}

typedef struct
{
	t_nodo *nodo;
	t_list *archivos_temp;
	char *temp_red_local;
}t_redLocal;

t_master *find_master(int sockt)
{
	int l_size = list_size(masters);
	int i = 0;
	int encontrado = 0;
	t_master *master_ ;

	while (!encontrado)
	{
		master_ = list_get(masters, i);
		if(master_->socket_ == sockt)
			encontrado = 1;
		else
			i++;
	}

	return master_;
}

t_worker *find_worker(char *nodo)
{
	int l_size = list_size(workers);
	int i = 0;
	int encontrado = 0;
	t_worker * wk;

	while(!encontrado)
	{
		wk = list_get(workers, i);
		if(!strcmp(nodo, wk->nodo->nodo))
			encontrado = 1;
		else
			i++;
	}

	return wk;
}

void enviar_reduccion_local(t_estado_master *estado_tr, int socket_)
{
	t_redLocal *red_l = malloc(sizeof(t_redLocal));
	t_master *master_ = find_master(socket_);
	t_worker *wk = find_worker(estado_tr->nodo);
	t_list *lista_auxiliar = list_create();
	list_add(lista_auxiliar, "prueba2");

	red_l->nodo = wk->nodo;
	red_l->temp_red_local = "prueba";
	red_l->archivos_temp = lista_auxiliar;

	size_t len;
	char *red_local_ser = serializar_redLocal(red_l, &len);

	int control;

	header head;
	head.codigo = 2;
	head.letra = 'Y';
	head.sizeData = len;

	message *mensaje = createMessage(head, (void *)red_local_ser);
	enviar_message(socket_, mensaje, yama_log, &control);
	//cambiar_estado(master_->master,estado_tr->nodo, estado_tr->bloque)
	list_destroy(lista_auxiliar);
	free(red_l);
}

//todo cuando se desconecte un master poner en null el socket
