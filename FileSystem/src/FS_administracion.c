/*
 * FS_administracion.c
 *
 *  Created on: 19/9/2017
 *      Author: utnso
 */

#include "FS_administracion.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/bitarray.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/generales.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "estructurasfs.h"

#define Mib 1048576

extern yamafs_config *configuracion;
extern t_log *logi;
extern t_directory directorios[100];
extern t_list *nodos;
extern t_list *archivos;

// Funciones Auxiliares
static char *armar_string_nombres_nodos();

static char *completar_path_metadata(char *);

static char *nombres_subdirectorios(char *);

static char *nombres_archivos(char *);

//static int indice_padre_archivo(char *);


//todo: Funciones de recuperacion de estructuras administrativas de un estado anterior //

int recuperar_estructuras_administrativas(void) {

    int status;
    // recuperar arbol de directorios
    status = recuperar_arbol_directorios();
    if (status != 1) {
        log_error(logi, "Error en la recuperacion del arbol de directorios, bai");
        return status;
    }
    // recuperar NODOS
    status = recuperar_nodos();
    if (status != 1) {
        log_error(logi, "Error en la recuperacion de nodos, bai");
        return status;
    }
    // Recuperar bitmaps nodos
    int cantidad_nodos = list_size(nodos);
    int i = 0;
    while (i < cantidad_nodos) {

        NODO *unodo = list_get(nodos, i);
        status = recuperar_bitmap_nodo(unodo);
        if (status != 1) {
            log_error(logi, "Error en la recuperacion de bitmaps, bai");
            return status;
        }
        i++;
    }

    status = recuperar_metadata_archivos();
    if (status != 1) {
        log_error(logi, "Error en la recuperacion de metadata de archivos, bai");
        return status;
    }

    return 1;
}

int recuperar_arbol_directorios(void) {

    char *path_armado;
    // recuperar arbol de directorios
    path_armado = completar_path_metadata("directorios.dat");
    FILE *filedir = fopen(path_armado, "r");

    if (filedir == NULL) {
        log_error(logi, "Error abriendo archivo 'directorios.dat'");
        free(path_armado);
        return -1;
    } else {
        fread(directorios, sizeof(t_directory), 100, filedir);
        fclose(filedir);
    }

    free(path_armado);
    return 1;
}

int recuperar_nodos(void) {

    char *path_armado;
    path_armado = completar_path_metadata("nodos.bin");

    struct stat data;
    int res = stat(path_armado, &data);
    if (res != 0) {
        log_error(logi, "Error abriendo Nodos.bin");
        free(path_armado);
        return -1;
    }

    t_config *config_nodos = config_create(path_armado);

    configuracion->espacio_total = (config_get_int_value(config_nodos, "TAMANIO"))*Mib;
    configuracion->espacio_libre = (config_get_int_value(config_nodos, "LIBRE"))*Mib;
    char **nombres_nodos = config_get_array_value(config_nodos, "NODOS");

    int i = 0;
    char *key_aux;

    while (nombres_nodos[i] != NULL) {

        NODO *nodo_recuperado = malloc(sizeof(NODO));

        nodo_recuperado->estado = no_disponible;

        nodo_recuperado->nombre = malloc(strlen(nombres_nodos[i]));
        strcpy(nodo_recuperado->nombre, nombres_nodos[i]);

        key_aux = string_from_format("%sTotal", nombres_nodos[i]);
        nodo_recuperado->espacio_total = (config_get_int_value(config_nodos, key_aux))*Mib;
        free(key_aux);

        key_aux = string_from_format("%sLibre", nombres_nodos[i]);
        nodo_recuperado->espacio_libre = (config_get_int_value(config_nodos, key_aux))*Mib;
        free(key_aux);

        list_add(nodos, nodo_recuperado);
        i++;
    }

    free(path_armado);
    config_destroy(config_nodos);
    liberar_char_array(nombres_nodos);

    return 1;
}

int recuperar_bitmap_nodo(NODO *unodo) {

    struct stat mystat;

    char *aux_nombre_archivo = string_from_format("bitmaps/%s.dat", unodo->nombre);
    char *path_armado = completar_path_metadata(aux_nombre_archivo);

    int fdbitmap = open(path_armado, O_RDWR);
    if (fdbitmap <= 0) {
        log_error(logi, "Error abriendo %s \n", aux_nombre_archivo);
        free(path_armado);
        return -1;
    }
    fstat(fdbitmap, &mystat);
    char *bitarray = mmap(0, mystat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fdbitmap, 0);
    if (bitarray == MAP_FAILED) {
        log_error(logi, "Error en mmap :%s\n", strerror(errno));
        close(fdbitmap);
        free(path_armado);
        return -1;

    }
    unodo->bitmapNodo = bitarray_create_with_mode(bitarray, mystat.st_size, LSB_FIRST);
    free(aux_nombre_archivo);
    free(path_armado);
    close(fdbitmap);
    return 1;
}

int recuperar_metadata_archivos(void) {

    char *nombres_sub = nombres_subdirectorios("archivos");
    if (nombres_sub == NULL) {
        return -1;
    } else if (!strncmp(nombres_sub, "NADA", 4)) {
        return 1;
    }
    char **aux_split = string_split(nombres_sub, "-");

    int i = 0;
    char *subdirectorio;
    char *fullpath;
    char *archivoss;

    while (aux_split[i] != NULL) {

        subdirectorio = string_from_format("archivos/%s", aux_split[i]);
        fullpath = completar_path_metadata(subdirectorio);
        archivoss = nombres_archivos(fullpath);

        if (archivoss == NULL) {
            free(subdirectorio);
            free(fullpath);
            liberar_char_array(aux_split);
            return -1;
        } else if (strncmp(archivoss, "NADA", 4) != 0) {
            char **aux_split_archs = string_split(archivoss, "-");
            int c = 0;
            char *fullpath_archivo;
            while (aux_split_archs[c] != NULL) {
                fullpath_archivo = string_from_format("%s/%s", fullpath, aux_split_archs[c]);
                recuperar_metadata_un_arhcivo(fullpath_archivo,atoi(aux_split[i]));
                free(fullpath_archivo);
                c++;
            }

            liberar_char_array(aux_split_archs);
            free(archivoss);
        }

        free(fullpath);
        free(subdirectorio);

        i++;
    }

    liberar_char_array(aux_split);
    free(nombres_sub);

    return 1;
}

int recuperar_metadata_un_arhcivo(char *fullpath, int padre) {

    t_archivo *archi = malloc(sizeof(t_archivo));
    t_config *info = config_create(fullpath);

    archi->tamanio = config_get_int_value(info, "TAMANIO");
    archi->tipo = strdup(config_get_string_value(info, "TIPO"));
    char **pathnombre = sacar_archivo(fullpath);
    archi->index_padre = padre;
    archi->nombre = strdup(pathnombre[1]);

    archi->estado = no_disponible;

    archi->bloques = list_create();
    int i = 0;
    int limite = config_keys_amount(info)-2;
    int cantidad_leidas=0;

    char *key_bloque_copia0;
    char *key_bloque_copia1;
    char *key_bloque_bytes;

    while (cantidad_leidas < limite) {


        bloqueArchivo *nuevobloque = malloc(sizeof(bloqueArchivo));
        key_bloque_copia0 = string_from_format("BLOQUE%dCOPIA0", i);
        key_bloque_copia1 = string_from_format("BLOQUE%dCOPIA1", i);
        key_bloque_bytes = string_from_format("BLOQUE%dBYTES", i);

        if(config_has_property(info,key_bloque_copia0)){
        	char **copia0 = config_get_array_value(info, key_bloque_copia0);
        	nuevobloque->nodo0 = strdup(copia0[0]);
        	nuevobloque->bloquenodo0 = atoi(copia0[1]);
        	liberar_char_array(copia0);
        	cantidad_leidas++;
        }else{
        	nuevobloque->nodo0 = strdup("");
        	nuevobloque->bloquenodo0 = -1;
        }

        if(config_has_property(info,key_bloque_copia1)){
        	char **copia1 = config_get_array_value(info, key_bloque_copia1);
        	nuevobloque->nodo1 = strdup(copia1[0]);
        	nuevobloque->bloquenodo1 = atoi(copia1[1]);
        	liberar_char_array(copia1);
        	cantidad_leidas++;
        }else{
        	nuevobloque->nodo1 = strdup("");
        	nuevobloque->bloquenodo1 = -1;
        }


        nuevobloque->bytesEnBloque = config_get_int_value(info, key_bloque_bytes);
        cantidad_leidas++;

        list_add(archi->bloques, nuevobloque);



        free(key_bloque_copia0);
        free(key_bloque_copia1);
        free(key_bloque_bytes);

        i++;
    }

    liberar_char_array(pathnombre);
    archi->cantbloques = i;
    list_add(archivos, archi);
    config_destroy(info);
    return 1;
}
//todo: Funciones de creacion de estructuras administrativas en un inicio limpio //

int iniciar_arbol_directorios(void) {

    int i;
    for (i = 0; i < 100; i++) {
        directorios[i].index = i;
        memset(directorios[i].nombre, 0, 255);
        directorios[i].padre = -9; // Flag de Entrada Directorio Vacía
    }
    //Directorio Padre, seria la "/"
    memcpy(directorios[0].nombre, configuracion->dir_estructuras, strlen(configuracion->dir_estructuras));
    directorios[0].padre = -1; // Dir Root no tiene padre

    char *path_armado = completar_path_metadata("directorios.dat");

    FILE *filedir = fopen(path_armado, "w");
    fwrite(directorios, sizeof(t_directory), 100, filedir);
    fclose(filedir);

    free(path_armado);
    return 1;

}

int iniciar_nodos(void) {

    char *path_armado = completar_path_metadata("nodos.bin");
    t_config *tabla_nodos = malloc(sizeof(t_config));
    tabla_nodos->path = path_armado;

    t_dictionary *elementos = dictionary_create();
    tabla_nodos->properties = elementos;

    char *espacio_total = string_itoa((configuracion->espacio_total)/Mib);
    char *espacio_libre = string_itoa((configuracion->espacio_libre)/Mib);

    config_set_value(tabla_nodos,"TAMANIO",espacio_total);
    config_set_value(tabla_nodos,"LIBRE",espacio_libre);

    free(espacio_total);
    free(espacio_libre);

    char *nomnodos = armar_string_nombres_nodos();
    config_set_value(tabla_nodos, "NODOS", nomnodos);

    void _agregar_info_nodo(NODO *self) {

        char *total = string_from_format("%sTotal", self->nombre);
        char *libre = string_from_format("%sLibre", self->nombre);
        espacio_total = string_itoa((self->espacio_total)/Mib);
        espacio_libre = string_itoa((self->espacio_libre)/Mib);

        config_set_value(tabla_nodos, total, espacio_total);
        config_set_value(tabla_nodos, libre, espacio_libre);

        free(total);
        free(libre);

        free(espacio_total);
        free(espacio_libre);
    }

    list_iterate(nodos, (void *) _agregar_info_nodo);

    config_save(tabla_nodos);

    config_destroy(tabla_nodos);
    free(nomnodos);

    return 1;
}


int iniciar_bitmaps_nodos(void) {

    void _create_bitmap(NODO *self) {

        int cantBloques = self->espacio_total / Mib;
        char *bitarray = malloc((size_t) cantBloques);
        memset(bitarray, 0, (size_t) cantBloques);
        self->bitmapNodo = bitarray_create_with_mode(bitarray, (size_t) cantBloques, LSB_FIRST);
    }

    list_iterate(nodos, (void *) _create_bitmap);

    void _save_bitmap_infile(NODO *self) {

        char *aux_nombre_archivo = string_from_format("bitmaps/%s.dat", self->nombre);
        char *path_armado = completar_path_metadata(aux_nombre_archivo);

        FILE *filedir = fopen(path_armado, "w+");
        fwrite(self->bitmapNodo->bitarray, self->bitmapNodo->size, 1, filedir);
        fclose(filedir);

        free(aux_nombre_archivo);
        free(path_armado);
    }

    list_iterate(nodos, (void *) _save_bitmap_infile);

    void _mmap_bitmap(NODO *self) {

        free(self->bitmapNodo->bitarray);
        bitarray_destroy(self->bitmapNodo);
        recuperar_bitmap_nodo(self);

    }

    list_iterate(nodos, (void *) _mmap_bitmap);


    return 1;
}

void crear_subdirectorios(void) {

	struct stat info_dir;
	char *syscmd;

	char *path_armado = completar_path_metadata("archivos");
    if(stat(path_armado,&info_dir) == 0){
    	syscmd = string_from_format("rm -r %s",path_armado);
    	system(syscmd);
    	free(syscmd);
    }
    mkdir(path_armado, 0775);
    free(path_armado);

    path_armado = completar_path_metadata("bitmaps");
    if(stat(path_armado,&info_dir) == 0){
    	syscmd = string_from_format("rm -r %s",path_armado);
    	system(syscmd);
    	free(syscmd);
    }
    mkdir(path_armado, 0775);
    free(path_armado);

}
//todo: Funciones para manipular/operar/etc elementos de las diferentes estructuras ya creadas

t_archivo *get_metadata_archivo(char *path) {

    t_archivo *res = NULL;
    char **dirNom = sacar_archivo(path);

    int padre = existe_ruta_directorios(dirNom[0]);
    if (padre == -9) {
        liberar_char_array(dirNom);
        return res;
    }

    bool existe = existe_archivo(dirNom[1], padre);
    if (!existe) {
        liberar_char_array(dirNom);
        return res;
    }

    int _get_archivo(t_archivo *self) {

        return (!strcmp(self->nombre, dirNom[1]) && self->index_padre == padre);
    }
    res = list_find(archivos, (void *) _get_archivo);

    liberar_char_array(dirNom);
    return res;

}


t_archivo *get_metadata_archivo_sinvalidar(char *nombre, int padre) {

    t_archivo *res = NULL;
    int _get_archivo(t_archivo *self) {

        return (!strcmp(self->nombre, nombre) && self->index_padre == padre);
    }
    res = list_find(archivos, (void *) _get_archivo);
    return res;
}

NODO *get_NODO(char *nombre) {

    NODO *res = NULL;
    int _buscar(NODO *self) {
        return (!strcmp(self->nombre, nombre));
    }
    res = list_find(nodos, (void *) _buscar);
    return res;
}

int existe_dir_en_padre(char *nombre, int padre) {
    int i;
    for (i = 0; i < 100; i++) {
        if (!strcmp(directorios[i].nombre, nombre) && directorios[i].padre == padre) {
            return i;
        }
    }
    return -9;
}

int existe_ruta_directorios(char *path) {

    int padre = 0;
    if (!string_contains(path, "/")) {
        padre = existe_dir_en_padre(path, padre);
    } else {
        char **dirs = string_split(path, "/");
        int i = 0;
        while (dirs[i] != NULL) {
            padre = existe_dir_en_padre(dirs[i], padre);
            i++;
        }
        liberar_char_array(dirs);
    }
    return padre;
}

bool existe_archivo(char *nombre, int padre) {

    bool res;
    int _buscador(t_archivo *self) {

        return (!strcmp(self->nombre, nombre) && self->index_padre == padre);
    }
    t_archivo *arch = list_find(archivos, (void *) _buscador);

    res = (arch == NULL) ? false : true;
    return res;
}

int agregar_directorio(char *nombre, int padre) {

    int i;
    int indice = -1;
    for (i = 0; i < 100; i++) {
        if (directorios[i].padre == -9) {
            indice = i;
            break;
        }
    }
    if (indice != -1) {
        memcpy(directorios[indice].nombre, nombre, strlen(nombre));
        directorios[indice].padre = padre;
    }
    return indice;
}

int liberarBloqueNodo(char *nameNodo, unsigned int numBlock) {
    int i;
    NODO *fetchNodo;
    for (i = 0; i < nodos->elements_count; ++i) {
        fetchNodo = list_get(nodos, i);
        if (strcmp(fetchNodo->nombre, nameNodo) == 0) {
            bitarray_clean_bit(fetchNodo->bitmapNodo, (off_t) numBlock);
            fetchNodo->espacio_libre += Mib;
            return 1;
        }
    }
    return 0;
}

bool containsDirOtherDir(int index) {
    int i;
    for (i = 0; i < 100; i++) {
        if (directorios[i].padre != -9 && directorios[i].padre == index) {
            return true;
        }
    }
    return false;
}

bool directoryEmpty(int index) {

	char indexString[14] = "";
    sprintf(indexString, "archivos/%d", index);
    char *pathReal = completar_path_metadata(indexString);
    char *files = nombres_archivos(pathReal);
    if (files == NULL) {
    	free(pathReal);
    	free(files);
        log_error(logi, "No se pudo abrir directorio a verificar");
        return false;
    }
    if (strcmp(files, "NADA") != 0) {
    	free(pathReal);
    	free(files);
        log_error(logi, "El directorio contiene archivos");
        return false;
    }
    if (containsDirOtherDir(index)) {
    	free(pathReal);
    	free(files);
        log_error(logi, "El directorio contiene otro directorios");
        return false;
    }
    return true;
}
void actualizar_arbol_directorios(void){

	char *path_armado = completar_path_metadata("directorios.dat");

	FILE *filedir = fopen(path_armado, "w");
	fwrite(directorios, sizeof(t_directory), 100, filedir);
	fclose(filedir);

	free(path_armado);

}
void actualizar_FS_free(void){

	int libre = 0;

	void _sumar_totalesylibres_nodos(NODO *self){

		if(self->estado == disponible){
			libre += self->espacio_libre;


		}
	}
	list_iterate(nodos,(void *) _sumar_totalesylibres_nodos);

	configuracion->espacio_libre = libre;


}

void sincronizar_bitmaps(){

	void _sincronizar(NODO* self){
		msync(self->bitmapNodo->bitarray, self->bitmapNodo->size, MS_SYNC);
	}
	list_iterate(nodos,(void *) _sincronizar);





}

void actualizar_tabla_nodos(void){

    char *path_armado = completar_path_metadata("nodos.bin");
    t_config *tabla_nodos =config_create(path_armado);

    actualizar_FS_free();
    sincronizar_bitmaps();

    char *espacio_total = string_itoa((configuracion->espacio_total)/Mib);
    char *espacio_libre = string_itoa((configuracion->espacio_libre)/Mib);
    config_set_value(tabla_nodos,"TAMANIO", espacio_total);
    config_set_value(tabla_nodos,"LIBRE", espacio_libre);
    free(espacio_libre);
    free(espacio_total);

    void _actualizar_info_nodo(NODO *self) {

        char *total = string_from_format("%sTotal", self->nombre);
        char *libre = string_from_format("%sLibre", self->nombre);
        espacio_total = string_itoa((self->espacio_total)/Mib);
        espacio_libre = string_itoa((self->espacio_libre)/Mib);

        config_set_value(tabla_nodos,total, espacio_total);
        config_set_value(tabla_nodos,libre, espacio_libre);

        free(total);
        free(libre);
        free(espacio_total);
        free(espacio_libre);
    }

    list_iterate(nodos, (void *) _actualizar_info_nodo);

    config_save(tabla_nodos);

    config_destroy(tabla_nodos);
    free(path_armado);

}

// Crear estructuras Fisicas en FS-local para un archivo
void crear_metadata_archivo(t_archivo *arch){

	char *aux_path = string_from_format("archivos/%d",arch->index_padre);
	char *path_armado = completar_path_metadata(aux_path);

	struct stat info;
	if(stat(path_armado,&info)!=0){
		mkdir(path_armado,0775);
	}
	char *path_armado_conNombre = string_from_format("%s/%s",path_armado,arch->nombre);

	t_config *metadata = malloc(sizeof(t_config));
	metadata->path = path_armado_conNombre;
	metadata->properties = dictionary_create();

	char *aux = string_from_format("%d",arch->tamanio);
	config_set_value(metadata,"TAMANIO",aux);
	free(aux);

	aux = strdup(arch->tipo);
	config_set_value(metadata,"TIPO",aux);
	free(aux);

	char *key0,*key1,*key3,*copia0,*copia1,*bytesEnB;
	int block_count = 0;

	void _agregar_info_bloques(bloqueArchivo *self){

		if(self->bloquenodo0 != -1){
			key0= string_from_format("BLOQUE%dCOPIA0",block_count);
			copia0 = string_from_format("[%s,%d]",self->nodo0,self->bloquenodo0);
			config_set_value(metadata,key0,copia0);
			free(key0);
			free(copia0);
		}
		if(self->bloquenodo1 != -1){
			key1= string_from_format("BLOQUE%dCOPIA1",block_count);
			copia1 = string_from_format("[%s,%d]",self->nodo1,self->bloquenodo1);
			config_set_value(metadata,key1,copia1);
			free(key1);
			free(copia1);
		}
		key3 = string_from_format("BLOQUE%dBYTES",block_count);
		bytesEnB = string_from_format("%d",self->bytesEnBloque);
		config_set_value(metadata,key3,bytesEnB);
		free(key3);
		free(bytesEnB);

		block_count++;
	}
	list_iterate(arch->bloques,(void *)_agregar_info_bloques);

	config_save(metadata);

	config_destroy(metadata);
	free(path_armado);
	free(aux_path);
}


void eliminar_metadata_archivo(t_archivo *arch){

	char *aux_path = string_from_format("archivos/%d",arch->index_padre);
	char *path_armado = completar_path_metadata(aux_path);
	char *path_armado_conNombre = string_from_format("%s/%s",path_armado,arch->nombre);

	unlink(path_armado_conNombre);
}

void eliminar_directorio(int index){

	char *aux_path = string_from_format("archivos/%d",index);
	char *path_armado = completar_path_metadata(aux_path);

	rmdir(path_armado);

	free(aux_path);
	free(path_armado);
}

//todo: Funciones Auxiliares

static char *completar_path_metadata(char *archivo) {

    char *pmetadata = strdup(configuracion->dir_estructuras);
    char *pmdirectorios = strdup("");

    string_append(&pmdirectorios, pmetadata);
    if (string_ends_with(pmdirectorios, "/")) {
        string_append(&pmdirectorios, archivo);
    } else {
        string_append(&pmdirectorios, "/");
        string_append(&pmdirectorios, archivo);
    }

    free(pmetadata);
    return pmdirectorios;
}

static char *armar_string_nombres_nodos() {

    char *aux_string = strdup("");
    int i;
    int cantnodos = list_size(nodos);

    for (i = 0; i < (cantnodos - 1); i++) {

        NODO *aux_nodo = list_get(nodos, i);
        string_append(&aux_string, aux_nodo->nombre);
        string_append(&aux_string, ",");
    }
    if (i == cantnodos - 1) {
        NODO *aux_nodo = list_get(nodos, i);
        string_append(&aux_string, aux_nodo->nombre);
    }


    char *string_final = string_from_format("[%s]", aux_string);

    free(aux_string);

    return string_final;

}

static char *nombres_subdirectorios(char *donde) {

    DIR *directorio;
    struct dirent *entry;
    int cantidad_subdirectorios;
    char *path_armado = completar_path_metadata(donde);
    char *archivos = strdup("");
    directorio = opendir(path_armado);

    if (directorio == NULL) {
        perror("Error abriendo directorio de archivos");
        free(path_armado);
        free(archivos);
        return NULL;
    }

    cantidad_subdirectorios = 0;

    for (entry = readdir(directorio); entry != NULL; entry = readdir(directorio)) {

        if (((strncmp(entry->d_name, ".", 1)) || (strncmp(entry->d_name, "..", 1))) && entry->d_type == DT_DIR) {

            cantidad_subdirectorios++;

            string_append(&archivos, "-");
            string_append(&archivos, entry->d_name);

        }
    }
    if (cantidad_subdirectorios == 0) {
        free(archivos);
        return "NADA";
    }
    char *subnombres = string_substring_from(archivos, 1);
    free(path_armado);
    free(archivos);
    closedir(directorio);

    return subnombres;
}

static char *nombres_archivos(char *donde) {

    DIR *directorio;
    struct dirent *entry;
    int cantidad_archivos;
    char *path_armado = strdup(donde);
    char *archivos = strdup("");
    directorio = opendir(path_armado);

    if (directorio == NULL) {
        perror("Error abriendo directorio de archivos");
        free(path_armado);
        free(archivos);
        return NULL;
    }

    cantidad_archivos = 0;

    for (entry = readdir(directorio); entry != NULL; entry = readdir(directorio)) {

        if (((strncmp(entry->d_name, ".", 1)) || (strncmp(entry->d_name, "..", 1))) && entry->d_type == DT_REG) {

            cantidad_archivos++;

            string_append(&archivos, "-");
            string_append(&archivos, entry->d_name);

        }
    }
    if (cantidad_archivos == 0) {
        free(archivos);
        return "NADA";
    }
    char *subnombres = string_substring_from(archivos, 1);
    free(path_armado);
    free(archivos);
    closedir(directorio);

    return subnombres;

}

void liberar_char_array(char **miarray) {

    int i = 0;
    while (miarray[i] != NULL) {
        free(miarray[i]);
        i++;
    }
    free(miarray);
}

char **sacar_archivo(char *fullpath1) {

    char **path = malloc(sizeof(char *) * 3);
    path[2] = NULL;
    int i = 0, ii = 0, j = 0;
    char *fullpath = strdup(fullpath1);


    if (!string_contains(fullpath, "/")) {
        path[1] = strdup(fullpath);
        path[0] = strdup("/");
        free(fullpath);
        return path;
    }

    path[0] = strdup("");
    path[1] = strdup("");
    char **split = string_split(fullpath, "/");

    while (split[ii] != NULL) ii++;
    i = ii - 2;

    string_append(&path[0], "/");
    while (j <= i) {
        string_append(&path[0], split[j]);
        string_append(&path[0], "/");
        j++;
    }
    if(ii == 0){
        path[1] = strdup("");

        liberar_char_array(split);
        free(fullpath);

        return path;
    }
    string_append(&path[1], split[ii - 1]);

    liberar_char_array(split);
    free(fullpath);

    return path;
}
