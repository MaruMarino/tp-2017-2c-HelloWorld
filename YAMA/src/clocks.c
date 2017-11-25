#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/serializacion.h>
#include <funcionesCompartidas/serializacion_yama_master.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "estructuras.h"
#include "conexion_master.h"
#include "conexion_fs.h"
#include "manejo_tabla_estados.h"
#include "clocks.h"

extern t_configuracion *config;
extern t_list *workers;
extern t_list *masters;
extern t_log *yama_log;
extern t_list *tabla_estado;

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
		worker->carga_historica = 0;

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
int get_menor_carga2(t_list *lista_auxiliar)
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

int get_menor_carga(t_list *lista_auxiliar)
{
	int menor_carga = 10000000;
	int i = 0;
	int size = list_size(lista_auxiliar);

	while(size > 0)
	{
		t_worker *worker = list_get(lista_auxiliar, i);

		if(worker->carga_historica < menor_carga)
			menor_carga = worker->carga_historica;

		i++;
		size--;
	}

	return menor_carga;
}

void posicionar_clock()
{
	escribir_log(yama_log, "Comenzando posicionamiento de clock");
	int mayor_disponibilidad = get_mayor_disponibilidad();

	t_list *lista_auxiliar = list_create();
	int size_l = list_size(workers);
	int i;

	for(i = 0 ; i < size_l ; i++)
	{
		t_worker *worker = list_get(workers, i);
		worker->clock = false;
		if(worker->disponibilidad == mayor_disponibilidad)
		{
			list_add(lista_auxiliar, worker);
		}
	}

	int menor_carga = get_menor_carga(lista_auxiliar);

	bool _menor_carga(t_worker *worker)
	{
		return worker->carga_historica == menor_carga;
	}

	t_worker *worker_ = list_find(workers, (void *)_menor_carga);
	worker_->clock = true;

	escribir_log_compuesto(yama_log, "Se posicionó el clock en el Worker asociado al: ", worker_->nodo->nodo);

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
	escribir_log(yama_log, "Buscando Worker para trabajar");
	t_worker *worker = NULL;
	t_worker * worker2;
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
		int resultado_parcial = (!strcmp(bl1->nodo, worker_aux->nodo->nodo)) || (!strcmp(bl2->nodo, worker_aux->nodo->nodo));
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
			return (!strcmp(bl2->nodo, worker->nodo->nodo));
		}
		t_bloque *bl = list_find(lista_aux, (void *)_bloque_archivo);

		list_add(worker->bloques, bl);
		worker->clock = false;
		worker->carga_actual++;
		worker->carga_historica++;

		if(worker->disponibilidad > 0)
			worker->disponibilidad--;
		else
			worker->disponibilidad = config->base;
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
			worker2 = list_get(workers, next_index);

			t_list *lista43 = list_create();
			for(i = 0; i < l_size; i++)
			{
				t_bloque *bl = list_get(archivo, i);

				if(bl->n_bloque_archivo == n_bloque)
					list_add(lista43, bl);
			}

			bool _bloque(t_bloque *bl)
			{
				return bl->n_bloque_archivo == n_bloque && (!strcmp(bl->nodo,worker2->nodo->nodo));
			}
			bool any = list_any_satisfy(lista43,(void *) _bloque);

			if ( any && worker2->disponibilidad > 0)
			{
				bool _bloque_archivo(t_bloque *bl2)
				{
					return (!strcmp(bl2->nodo, worker2->nodo->nodo));
				}
				t_bloque *bl = list_find(lista_aux, (void *)_bloque_archivo);

				list_add(worker2->bloques, bl);
				encontrado = 1;
				worker2->disponibilidad --;
				worker2->carga_actual++;
				worker2->carga_historica++;
			}
			else
			{
				next_index = (next_index + 1) % w_size;
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
		if(encontrado)
			return worker2;
	}

	list_destroy(lista_aux);

	return worker;
}

t_worker *worker_copia(t_list *archivo, int n_bloque, char *nodo)
{
	t_worker *worker = NULL;

	int l_size = list_size(archivo);

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
		int resultado_parcial = (!(strcmp(bl1->nodo, worker_aux->nodo->nodo))) || (!(strcmp(bl2->nodo, worker_aux->nodo->nodo)));
		int otro_resultado = strcmp(worker_aux->nodo->nodo, nodo);

		return (resultado_parcial && otro_resultado);
	}

	worker = list_find(workers,(void *) _nodo_bloque);

	bool _bloque_archivo(t_bloque *bl2)
	{
		return (!strcmp(bl2->nodo, worker->nodo->nodo));
	}
	t_bloque *bl = list_find(lista_aux, (void *)_bloque_archivo);

	list_add(worker->bloques, bl);

	return worker;
}

void obtener_nodo_transformacion(t_list *archivo, t_list *transformaciones, int bloque, int master)
{
	escribir_log(yama_log, "Armado de transformación");
	t_worker *worker0 = get_worker(archivo, bloque);
	t_worker *worker_cop = worker_copia(archivo, bloque, worker0->nodo->nodo);
	int ind_cop = list_size(worker_cop->bloques) - 1;
	t_bloque *b_cop = list_get(worker_cop->bloques, ind_cop);
	int ind = list_size(worker0->bloques) - 1;
	t_bloque *bloque_ = list_get(worker0->bloques, ind);
	t_transformacion *transf = malloc(sizeof(t_transformacion));
	transf->bloque = bloque_->n_bloque;
	transf->bytes = bloque_->bytes;
	transf->nodo = worker0->nodo;
	t_estado *est = generar_estado(master, bloque_->n_bloque, worker0->nodo->nodo, worker_cop->nodo->nodo, b_cop->n_bloque, bloque_->bytes, bloque);
	transf->temporal = est->archivo_temporal;

	list_add(transformaciones, transf);
}

void ejecutar_clock(t_list *archivo_bloques, int cant_bloques, int _socket)
{
	escribir_log(yama_log, "Empieza el algoritmo de pre-planificación");
	int i = 0;
	calcular_disponibilidad();
	posicionar_clock();

	t_master *ms  = find_master(_socket);
	t_list *transformaciones = list_create();
	while(cant_bloques > 0)
	{
		obtener_nodo_transformacion(archivo_bloques, transformaciones, i, ms->master);

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

t_master *find_master(int sockt)
{
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

char *generar_nombre_red_local(int mast, char* nod)
{
	char *nombre = strdup("/tmp/rl_m");
	char *masterrr = string_itoa(mast);

	string_append(&nombre, masterrr);
	string_append(&nombre, nod);
	free(masterrr);
	return nombre;
}

void armar_reduccion_local(int sz, t_master *master_, t_estado *est, t_estado_master *estado_tr)
{
	escribir_log(yama_log, "Comenzando a armar reduccion local");
	t_redLocal *red_l = malloc(sizeof(t_redLocal));

	int i;
	t_worker *wk = find_worker(estado_tr->nodo);
	t_list *lista_auxiliar = list_create();

	for(i=0; i < sz; i++)
	{
		t_estado * est2 = list_get(tabla_estado, i);
		if((!strcmp(est2->nodo,estado_tr->nodo)) && est2->master == master_->master
				&& est2->etapa == TRANSFORMACION && est2->estado == FINALIZADO_OK/*&& est2->etapa == ESPERA_REDUCCION_LOCAL*/)
		{
			list_add(lista_auxiliar, est2->archivo_temporal);
		}
	}
	red_l->nodo = wk->nodo;
	red_l->temp_red_local = generar_nombre_red_local(master_->master, estado_tr->nodo);
	red_l->archivos_temp = lista_auxiliar;
	red_l->bloque = estado_tr->bloque;

	size_t len;
	char *red_local_ser = serializar_redLocal(red_l, &len);

	int control;

	header head;
	head.codigo = 2;
	head.letra = 'Y';
	head.sizeData = len;

	message *mensaje = createMessage(&head, (void *)red_local_ser);
	enviar_message(master_->socket_, mensaje, yama_log, &control);
	t_estado *es_rl = generar_estado(master_->master, -10, estado_tr->nodo, NULL, -10, -10, -10);
	es_rl->archivo_temporal = red_l->temp_red_local;
	es_rl->etapa = REDUCCION_LOCAL;
	list_destroy(lista_auxiliar);
	free(red_l);
}

void enviar_reduccion_local(t_estado_master *estado_tr, int socket_)
{
	t_master *master_ = find_master(socket_);
	t_estado *est = get_estado(master_->master, estado_tr->nodo, estado_tr->bloque, TRANSFORMACION);
	int i;
	int sz = list_size(tabla_estado);
	est->cant_bloques_nodo--;
	//est->etapa = ESPERA_REDUCCION_LOCAL;
	est->estado = FINALIZADO_OK;

	for(i=0; i < sz; i++)
	{
		t_estado * est2 = list_get(tabla_estado, i);
		if((!strcmp(est2->nodo,estado_tr->nodo)) && est2->master == master_->master
				&& est2->etapa == TRANSFORMACION && est2->estado == EN_PROCESO)
		{
			est2->cant_bloques_nodo = est->cant_bloques_nodo;
		}
	}

	if(est->cant_bloques_nodo == 0)
	{
		armar_reduccion_local(sz, master_, est, estado_tr);
	}
	else
		escribir_log(yama_log, "Esperando a que se terminen todas las transformaciones para el nodo");
}

void armar_transformacion_replanificada(t_estado *estado, int socket_)
{
	escribir_log(yama_log, "Generando transformacion replanificada");
	t_transformacion *tr = malloc(sizeof(t_transformacion));
	t_worker *wk = find_worker(estado->nodo_copia);
	t_list *transformaciones = list_create();

	tr->bloque = estado->bloque_copia;
	tr->nodo = wk->nodo;
	t_estado * est = generar_estado(estado->master, tr->bloque, wk->nodo->nodo, NULL, 0, estado->bytes,22);
	est->copia_disponible = false;
	tr->temporal = est->archivo_temporal;
	tr->bytes = est->bytes;

	list_add(transformaciones, tr);

	header head;
	head.codigo = 1;
	head.letra = 'Y';
	int control;

	char *transformaciones_ser = serializar_lista_transformacion(transformaciones, &head.sizeData);

	message *mensaje = createMessage(&head, transformaciones_ser);
	enviar_message(socket_, mensaje, yama_log, &control);

	free(transformaciones);
}

void replanificar(t_estado_master *estado_tr, int socket_)
{
	escribir_log(yama_log, "Chequeando replanificación");
	t_master *master_ = find_master(socket_);
	t_estado *est = get_estado(master_->master, estado_tr->nodo, estado_tr->bloque, TRANSFORMACION);
	int sz = list_size(tabla_estado);
	est->cant_bloques_nodo--;
	est->estado = ERROR;

	if(est->cant_bloques_nodo == 0 && est->etapa == TRANSFORMACION && est->estado != ERROR)
	{
		armar_reduccion_local(sz, master_, est, estado_tr);
	}

	if(est->copia_disponible)
	{
		armar_transformacion_replanificada(est, socket_);
	}
	else
	{
		escribir_error_log(yama_log, "No hay copia disponible para replanificar, lo 100to");
		matar_master(socket_);
	}

	est->copia_disponible = false;

}

void reduccion_global(int socket_, t_estado_master *estado_tr)
{
	t_master *master_ = find_master(socket_);
	t_estado *est = get_estado(master_->master, estado_tr->nodo, -10, REDUCCION_LOCAL);
	int i;
	bool reducir;
	int sz = list_size(tabla_estado);
	est->estado = FINALIZADO_OK;


	for(i=0; i < sz; i++)
	{
		t_estado * est2 = list_get(tabla_estado, i);
		bool transformacion_finalizada = (est2->etapa == TRANSFORMACION && est2->estado != EN_PROCESO);
		bool reduccion_finalizada = (est2->etapa == REDUCCION_LOCAL && est2->estado == FINALIZADO_OK);

		if((est2->master == master_->master && (transformacion_finalizada || reduccion_finalizada))
			|| est2->master != master_->master)
		reducir = true;
		else
		{
			reducir = false;
			break;
		}
	}

	if(reducir)
		armar_reduccion_global(sz, master_, est, estado_tr);
	else
		escribir_log(yama_log, "Todavía no se puede comenzar con la Reducción Global");


}

char * generar_nombre_red_global(int mast, char* nod)
{
	char *nombre = strdup("/tmp/rg_m");
	char *masterrr = string_itoa(mast);

	string_append(&nombre, masterrr);
	string_append(&nombre, nod);
	free(masterrr);
	return nombre;
}

void armar_reduccion_global(int sz, t_master *master_, t_estado *est, t_estado_master *estado_tr)
{
	escribir_log(yama_log, "Comenzando a armar reducción global");
	int en_worker_trabajado = 0;
	int i;
	t_list *lista_auxiliar = list_create();
	int cant_temporales = 0;

	t_redGlobal *red_g2;
	for(i=0; i < sz; i++)
	{
		t_estado * est2 = list_get(tabla_estado, i);
		if(est2->master == master_->master
			&& est2->etapa == REDUCCION_LOCAL && est2->estado == FINALIZADO_OK)
		{
			t_redGlobal *red_g = malloc(sizeof(t_redGlobal));
			red_g->encargado = 0;
			t_worker *wkk = find_worker(est2->nodo);
			red_g->nodo = wkk->nodo;
			red_g->temp_red_local = est2->archivo_temporal;
			red_g->red_global = strdup("");

			cant_temporales++;
			list_add(lista_auxiliar, red_g);
		}
	}


	int menor_carga = get_menor_carga(workers);

	bool menor_carga_(t_worker * wr)
	{
		return wr->carga_historica == menor_carga;
	}

	t_worker *wk = list_find(workers, (void *)menor_carga_);

	wk->carga_actual +=  ((cant_temporales + 1) / 2);

	bool _worker_encargado(t_redGlobal *rg)
	{
		return (!strcmp(rg->nodo->nodo, wk->nodo->nodo));
	}

	t_redGlobal *red_g = list_find(lista_auxiliar, (void *)_worker_encargado);

	if (red_g != NULL)
	{
		red_g->encargado = 1;
		free(red_g->red_global);
		red_g->red_global = generar_nombre_red_global(master_->master, red_g->nodo->nodo);
		en_worker_trabajado = 1;
	}else
	{
		red_g2 = malloc(sizeof(t_redGlobal));
		red_g2->encargado = 1;
		red_g2->nodo = wk->nodo;
		red_g2->temp_red_local = strdup("");
		red_g2->red_global =generar_nombre_red_global(master_->master, red_g2->nodo->nodo) ;

		list_add(lista_auxiliar, red_g2);
	}

	size_t len;
	char *red_global_ser = serializar_lista_redGlobal(lista_auxiliar, &len);
	int control;

	header head;
	head.codigo = 3;
	head.letra = 'Y';
	head.sizeData = len;

	message *mensaje = createMessage(&head, (void *)red_global_ser);
	enviar_message(master_->socket_, mensaje, yama_log, &control);
	t_estado *estt = generar_estado(master_->master,-10,wk->nodo->nodo,NULL,cant_temporales,-10, -10);
	free(estt->archivo_temporal);
	if(en_worker_trabajado)
		estt->archivo_temporal = red_g->red_global;
	else
		estt->archivo_temporal = red_g2->red_global;
	estt->etapa = REDUCCION_GLOBAL;
	//cambiar_estado(master_->master,estado_tr->nodo, estado_tr->bloque, REDUCCION_GLOBAL, red_g->red_global);
	list_destroy(lista_auxiliar);
	free(red_g);
}

void recalcular_cargas(int socket_)
{
	int i;
	t_worker *wk;
	t_master *master_ = find_master(socket_);
	int sz_ = list_size(tabla_estado);
	for(i=0; i < sz_; i++)
	{
		t_estado *est = list_get(tabla_estado, i);
		if(est->master == master_->master)
		{
			if(est->copia_disponible && est->etapa == TRANSFORMACION)
			{
				wk = find_worker(est->nodo);
				wk->carga_actual--;
				break;
			}
			if(!(est->copia_disponible) && est->etapa == TRANSFORMACION)
			{
				wk = find_worker(est->nodo_copia);
				wk->carga_actual--;
				break;
			}
			if(est->etapa == REDUCCION_GLOBAL)
			{
				wk = find_worker(est->nodo_copia);
				wk->carga_actual += (- est->bloque_copia);
				break;
			}
		}
	}
}
