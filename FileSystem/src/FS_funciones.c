/*
 * FS_funciones.c
 *
 *  Created on: 12/9/2017
 *      Author: utnso
 */


#include "FS_funciones.h"

#include <commons/string.h>
#include <pthread.h>
#include <funcionesCompartidas/generales.h>
#include <commons/log.h>
#include <string.h>
#include <stdlib.h>
#include "FS_administracion.h"
#include "FS_interfaz_nodos.h"
#include "estructurasfs.h"
#include "showState.h"
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/funcionesNet.h>
#include "FS_conexiones.h"

extern comando commands[];
extern yamafs_config *configuracion;
extern t_log *logi;
extern t_list *archivos;
extern t_directory directorios[100];
extern t_list *nodos;
extern pthread_mutex_t mutex_socket;


#define cyan  "\x1B[36m"
#define sin "\x1B[0m"
#define Mib 0x100000

comando *buscar_comando(char *nombre);

void deleteArchive(char *path);

void deleteDirectory(char *path);

void deleteBlocksCpArchive(char *path, char *numBlock, char *numCopy);

int fs_ls(char *h) {

	char **split = string_split(h," ");
	int i = 0;
	while(split[i]!=NULL) i++;
	if(i== 2){

		int padre =existe_ruta_directorios(split[2]);
		if(padre == -9){
			printf("No existe ruta directorio\n");
			log_info(logi,"Usuario pidio archivos de un path no existente %s ",split[1]);
			liberar_char_array(split);
			return 0;
		}

		void _imprimir_nombre(t_archivo *self){

			if(self->index_padre == padre) printf("%s\n",self->nombre);
		}
		list_iterate(archivos,(void *) _imprimir_nombre);
		liberar_char_array(split);

	}else{
		printf("La cantidad de parámetros es incorrecta, ingrese '%s? ls%s' para más información\n", cyan, sin);
		liberar_char_array(split);
		log_info(logi,"Usuario ingresó mal el comando ls");
	}

    return 0;
}

int fs_rename(char *i) {

	char **split = string_split(i," ");
	int ii=0;
	while(split[ii]!= NULL) ii++;

	if(ii == 4){

		if( !strcmp(split[3],"-A") || !strcmp(split[3],"-a")){

			t_archivo *archivo = get_metadata_archivo(split[1]);
			if(archivo == NULL){
				printf("No existe archivo original\n");
				liberar_char_array(split);
				log_info(logi,"Usuario ingresó path original de archivo inexistente");
				return 0;
			}
			char **nuevo = sacar_archivo(split[1]);
			int padre = existe_ruta_directorios(nuevo[0]);
			bool existe = existe_archivo(split[2],padre);
			if(existe){
				printf("Ya existe un archivo con ese nombre en el directorio\n");
				liberar_char_array(split);
				liberar_char_array(nuevo);
				log_info(logi,"Usuario ingresó nombre que ya existe");
				return 0;
			}
			char *pathNuevo =string_from_format("archivos/%d/%s",padre,split[2]);
			char *completo = completar_path_metadata(pathNuevo);
			char *pathViejo = string_from_format("archivos/%d/%s",padre,nuevo[1]);
			char *completo_viejo = completar_path_metadata(pathViejo);

			if (rename(completo_viejo,completo) == -1){
				printf("No se pudo renombrar\n");
				liberar_char_array(split);
				liberar_char_array(nuevo);
				free(pathNuevo);
				free(completo);
				free(completo_viejo);
				free(pathViejo);
				log_info(logi,"Falló rename");
				return 0;
			}

			free(archivo->nombre);
			archivo->nombre = strdup(split[2]);
			printf("Exitos: %s -> %s \n",nuevo[1],split[2]);
			log_info(logi,"Exitos rename: %s -> %s",nuevo[1],split[2]);
			liberar_char_array(split);
			liberar_char_array(nuevo);
			free(pathNuevo);
			free(completo);
			free(completo_viejo);
			free(pathViejo);

		}else if( !strcmp(split[3],"-D") || !strcmp(split[3],"-d")){

			int padre = existe_ruta_directorios(split[1]);
			if(padre == -9){
				printf("No existe path original\n");
				log_info(logi,"Usuario quisó cambiar nombre de un directorio inexistente: %s",split[1]);
				liberar_char_array(split);
				return 0;
			}
			int existe = existe_dir_en_padre(split[2],padre);
			if(existe == -9){
				printf("Ya existe un directorio con ese nombre \n");
				log_info(logi,"Usuario quisó cambiar nombre a uno que ya existe");
				liberar_char_array(split);
				return 0;
			}

			memset(directorios[padre].nombre,'\0',255);
			memcpy(directorios[padre].nombre,split[2],strlen(split[2]));
			actualizar_arbol_directorios();
			printf("Exitos: %s -> %s \n",split[1],split[2]);
			log_info(logi,"Exitos rename: %s -> %s \n",split[1],split[2]);
			liberar_char_array(split);
			return 0;
		}else{
			printf("Ingrese '%s? rename%s' para información sobre su sintaxis \n", cyan, sin);
			liberar_char_array(split);
			log_info(logi,"Usuario ingresó mal el comando rename");
		}

	}else{
		printf("La cantidad de parámetros es incorrecta, ingrese '%s? rename%s' para más información\n", cyan, sin);
		liberar_char_array(split);
		log_info(logi,"Usuario ingresó mal el comando renme");
	}

    return 0;
}

int fs_format(char *j) {

    char **split = string_split(j, " ");
    int i = 0;
    while (split[i] != NULL) i++;

    if (i == 1) {
        if (configuracion->inicio_limpio) {

            if (nodos->elements_count < 2) {

                log_info(logi, "No se puede Formatear, se nesecitan al menos 2 Nodos");

            } else if (!configuracion->estado_estable) {

                crear_subdirectorios();
                iniciar_arbol_directorios();
                iniciar_nodos();
                iniciar_bitmaps_nodos();

                configuracion->estado_estable = 1;
            } else {

                log_info(logi, "El File System ya fue fomateado \n");
            }
        } else {
            log_info(logi, "El File System ya fue fomateado \n");
        }

    } else {
        printf("La cantidad de parámetros es incorrecta, ingrese '%s? format%s' para más información\n", cyan, sin);

    }
    liberar_char_array(split);

    return 0;
}

int fs_mv(char *k) {

	char **split = string_split(k," ");
	int ii=0;
	while(split[ii]!= NULL) ii++;

	if(ii == 4){

		if( !strcmp(split[3],"-A") || !strcmp(split[3],"-a")){

			t_archivo *archivo = get_metadata_archivo(split[1]);
			if(archivo == NULL){
				printf("No existe archivo original\n");
				liberar_char_array(split);
				log_info(logi,"Usuario ingresó path original de archivo inexistente");
				return 0;
			}
			char **nuevo = sacar_archivo(split[1]);
			int padre = existe_ruta_directorios(nuevo[0]);
			int existe =existe_ruta_directorios(split[2]);
			if(existe == -9){
				printf("No existe directorio destino\n");
				liberar_char_array(split);
				liberar_char_array(nuevo);
				log_info(logi,"Usuario ingresó directorio destino inexistente");
				return 0;
			}
			char *pathNuevo =string_from_format("archivos/%d/%s",existe,nuevo[1]);
			char *completo = completar_path_metadata(pathNuevo);
			char *pathViejo = string_from_format("archivos/%d/%s",padre,nuevo[1]);
			char *completo_viejo = completar_path_metadata(pathViejo);

			if (rename(completo_viejo,completo) == -1){
				printf("No se pudo mover\n");
				liberar_char_array(split);
				liberar_char_array(nuevo);
				free(pathNuevo);
				free(completo);
				free(completo_viejo);
				free(pathViejo);
				log_info(logi,"Falló mover");
				return 0;
			}

			archivo->index_padre = existe;
			printf("Exitos: %s -> %s/%s \n",split[1],split[2],nuevo[1]);
			log_info(logi,"Exitos: %s -> %s/%s \n",split[1],split[2],nuevo[1]);
			liberar_char_array(split);
			liberar_char_array(nuevo);
			free(pathNuevo);
			free(completo);
			free(completo_viejo);
			free(pathViejo);

		}else if( !strcmp(split[3],"-D") || !strcmp(split[3],"-d")){

			int padre = existe_ruta_directorios(split[1]);
			if(padre == -9){
				printf("No existe path original\n");
				log_info(logi,"Usuario quisó mover path inexistente: %s",split[1]);
				liberar_char_array(split);
				return 0;
			}
			int existe = existe_dir_en_padre(split[2],padre);
			if(existe == -9){
				printf("No existe directorio destino \n");
				log_info(logi,"Usuario quisó mover un directorio a otro que no existe");
				liberar_char_array(split);
				return 0;
			}

			directorios[padre].padre = existe;
			actualizar_arbol_directorios();
			char **nombre = sacar_archivo(split[2]);
			printf("Exitos: %s -> %s/%s \n",split[1],split[2],nombre[1]);

			log_info(logi,"Exitos: %s -> %s/%s \n",split[1],split[2],nombre[1]);
			liberar_char_array(split);
			liberar_char_array(nombre);

			return 0;
		}else{
			printf("Ingrese '%s? mv%s' para información sobre su sintaxis \n", cyan, sin);
			liberar_char_array(split);
			log_info(logi,"Usuario ingresó mal el comando mover");
		}

	}else{
		printf("La cantidad de parámetros es incorrecta, ingrese '%s? mover%s' para más información\n", cyan, sin);
		liberar_char_array(split);
		log_info(logi,"Usuario ingresó mal el comando mover");
	}

    return 0;
}

int fs_ayuda(char *pedido_ayuda) {


    printf("COMANDO | OBJETIVO | SINTAXIS\n");

    int var;
    for (var = 0; var < 14; ++var) {
        printf("%s%s%s |  %s  |  %s  \n", cyan, commands[var].name, sin, commands[var].doc, commands[var].sintax);
    }

    return 0;
}

int fs_rm(char *argv) {

    log_info(logi, "Ejecutando comando rm");
    int cantArgv = 0;
    char **arguments = string_split(argv, " ");
    if (arguments[1] == NULL) {
        log_error(logi, "Falta argumentos para el comando rm");
        liberar_char_array(arguments);
        return 0;
    }
    while (arguments[cantArgv] != NULL) {
        cantArgv++;
    }

    if (strcmp(arguments[1], "-d") == 0) {
        if (cantArgv > 3 || cantArgv < 3) {
            log_error(logi, "verifique los argumentos para rm -d {path_directorio}");
            liberar_char_array(arguments);
            return 0;
        }
        deleteDirectory(arguments[2]);
    } else if (strcmp(arguments[1], "-b") == 0) {
        if (cantArgv > 5 || cantArgv < 5) {
            liberar_char_array(arguments);
            log_error(logi, "verifique los argumentos para rm -b {path_archivo} {nro_bloque} {nro_copia}");
            return 0;
        }
        deleteBlocksCpArchive(arguments[2], arguments[3], arguments[4]);
    } else {
        if (cantArgv > 2) {
            liberar_char_array(arguments);
            log_error(logi, "verifique los argumentos para rm {path_archivo}");
            return 0;
        }
        deleteArchive(arguments[1]);
    }

    liberar_char_array(arguments);
    return 0;

}

int fs_payuda(char *duda) {

    char *comando_pedido = string_substring_from(duda, 1);
    string_trim(&comando_pedido);

    comando *buscado = buscar_comando(comando_pedido);
    if (buscado == NULL) {
        printf("Ingrese un comando válido,para consultar los disponibles ingrese '%sayuda%s'\n", cyan, sin);
    } else {
        printf("%s%s%s | %s | %s \n", cyan, buscado->name, sin, buscado->doc, buscado->sintax);
    }


    free(comando_pedido);

    return 0;
}

int fs_cat(char *n) {

    char **split = string_split(n, " ");
    int i = 0;
    while (split[i] != NULL) i++;

    if (i == 2) {

        char **dirName = sacar_archivo(split[1]);
        int padre = existe_ruta_directorios(dirName[0]);
        if (padre == -9) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("No existe ruta\n");
            return 0;
        }
        t_archivo *archivo = get_metadata_archivo_sinvalidar(dirName[1], padre);
        if (archivo == NULL) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("No existe archivo\n");
            return 0;
        }
        if (archivo->estado == no_disponible) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("Archivo no dispobible \n");
            return 0;
        }

        bloqueArchivo *bq;
        int bloques = archivo->cantbloques, alternar = 0;
        char *buff;

//          buff = pedirFile(archivo->bloques); // todo: testar esto
//         ya no deberia hacer falta entrar al ciclo for()

        //pthread_mutex_lock(&mutex_socket);
        for (i = 0; i < bloques; i++) {

            bq = list_get(archivo->bloques, i);

            buff = leer_bloque(bq, alternar);

            printf("%s", buff);

            free(buff);
            alternar = (alternar == 0) ? 1 : 0;
        }
        //pthread_mutex_unlock(&mutex_socket);
        liberar_char_array(split);
        liberar_char_array(dirName);

    } else {

        printf("La cantidad de parámetros es incorrecta, ingrese '%s? cat%s' para más información\n", cyan, sin);
        liberar_char_array(split);
    }

    return 0;
}

int fs_mkdir(char *p) {

    log_info(logi, "Ejecute mkdir con %s \n", p);
    char **splt = string_split(p, " ");
    int i = 0;
    int padre;
    while (splt[i] != NULL) i++;

    if (i == 2) {

        if (!string_contains(splt[1], "/")) {
            padre = existe_ruta_directorios(splt[1]);
            if (padre != -9 || padre == 0) {
                liberar_char_array(splt);
                printf("Ya existe directorio\n");
                return 0;
            }

        } else {
            char **nuevo_dir = sacar_archivo(splt[1]);
            padre = existe_ruta_directorios(nuevo_dir[0]);
            if (padre == -9) {
                liberar_char_array(splt);
                liberar_char_array(nuevo_dir);
                printf("No existe ruta directorios\n");
                return 0;
            }
            int existe = existe_dir_en_padre(nuevo_dir[1], padre);
            if (existe != -9) {
                liberar_char_array(splt);
                liberar_char_array(nuevo_dir);
                printf("Ya existe directorio\n");
                return 0;
            }
            int d = agregar_directorio(nuevo_dir[1], padre);
            if (d == -1) {
                liberar_char_array(splt);
                liberar_char_array(nuevo_dir);
                printf("No hay lugar para un nuevo directorio\n");
                return 0;
            }
            liberar_char_array(splt);
            liberar_char_array(nuevo_dir);
            printf("Operación Exitosa\n");
            actualizar_arbol_directorios();
            return 0;
        }


    } else {
        printf("La cantidad de parámetros es incorrecta, ingrese '%s? mkdir%s' para más información\n", cyan, sin);
        liberar_char_array(splt);
    }


    return 0;
}

int fs_cpfrom(char *q) {


    int i = 0;
    char **split_paths = string_split(q, " ");

    while (split_paths[i] != NULL) {
        i++;
    };

    if (i == 4) {
        int sizearchi = get_file_size(split_paths[1]);
        bool hay = hay_lugar_para_archivo(sizearchi);
        if (!hay) {
            printf("No hay lugar para almacenar el archivo\n");
            liberar_char_array(split_paths);
            return 0;
        }
        char **nombre = sacar_archivo(split_paths[1]);
        int padre = existe_ruta_directorios(split_paths[2]);
        if (padre == -9) {
            liberar_char_array(split_paths);
            liberar_char_array(nombre);
            printf("No existe ruta final, cree los directorios que faltan e intente nuevamente\n");
            return 0;
        }
        bool existe = existe_archivo(nombre[1], padre);
        if (existe) {
            liberar_char_array(split_paths);
            liberar_char_array(nombre);
            printf("Ya existe un archivo con ese nombre\n");
            return 0;
        }
        t_list *ba = escribir_desde_archivo(split_paths[1], split_paths[3][0], sizearchi);

        t_archivo *arch = malloc(sizeof(t_archivo));
        arch->tipo = strdup(split_paths[3]);
        arch->bloques = ba;
        arch->cantbloques = list_size(ba);
        arch->estado = disponible;
        arch->index_padre = padre;
        arch->nombre = strdup(nombre[1]);
        arch->tamanio = sizearchi;

        list_add(archivos, arch);
        crear_metadata_archivo(arch);
        liberar_char_array(split_paths);
        liberar_char_array(nombre);

    } else {
        printf("La cantidad de parámetros es incorrecta, ingrese '%s? cpfrom%s' para más información\n", cyan, sin);
        liberar_char_array(split_paths);
    }

    log_info(logi, "Ejecute cpfrom \n");
    return 0;
}

int fs_cpto(char *r) {
    printf("Ejecute cpto \n");
    return 0;
}

int fs_cpblock(char *s) {
    printf("Ejecute cpblock \n");
    return 0;
}

int fs_md5(char *t) {

    char **split = string_split(t, " ");
    int i = 0;
    while (split[i] != NULL) i++;

    if (i == 2) {

        char **dirName = sacar_archivo(split[1]);
        int padre = existe_ruta_directorios(dirName[0]);
        if (padre == -9) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("No existe ruta\n");
            return 0;
        }
        bool existe = existe_archivo(dirName[1], padre);
        if (!existe) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("No existe archivo\n");
            return 0;
        }
        t_archivo *arhcivo = get_metadata_archivo(split[1]);
        int temp = crear_archivo_temporal(arhcivo, "/tmp/auxiliar_md5");
        if (temp == -1) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("El archivo no se encuentra disponible\n");
            return 0;
        }
        char *linea = NULL;
        char **md;
        size_t len = 0;
        system("md5sum /tmp/auxiliar_md5 > /tmp/auxiliar_md5_2");
        FILE *f = fopen("/tmp/auxiliar_md5_2", "r");
        getline(&linea, &len, f);
        md = string_split(linea, " ");
        printf("%s\n", md[0]);

        liberar_char_array(split);
        liberar_char_array(dirName);
        liberar_char_array(md);
        free(linea);
        fclose(f);

        system("rm /tmp/auxiliar_md5*");

    } else {
        printf("La cantidad de parámetros es incorrecta, ingrese '%s? md5%s' para más información\n", cyan, sin);
        liberar_char_array(split);
    }

    return 0;
}

int fs_info(char *u) {
   //todo: borrar esto antes de entrega, lo dejo porque migue lo usa para controlar
	checkFileSystem();
    checkStateNodos();
    checkArchivos();
    checkdirectoris();

    char **split = string_split(u," ");
    int i=0;
    while(split[i]!= NULL) i++;

    if(i == 2){

    	t_archivo *fi = get_metadata_archivo(split[1]);
    	if(fi == NULL){
            printf("No existe archivo\n");
            log_info(logi,"Usuario pidio info de un archivo que no se pudo encontrar: %s",split[1]);
            liberar_char_array(split);
            return 0;
    	}

    	printf("Nombre:%s\n",fi->nombre);
    	printf("Tamanio:%d\n(bytes)-%d(bloques)",fi->tamanio,fi->cantbloques);
    	printf("Estado:%s\n",getEstado(fi->estado));
    	printf("Tipo:%s\n",fi->tipo);
    	printf("Info Bloques:\n");
    	int i = 0;
    	printf("Copia # :#BloqueDeArchivo - (#BloqueDeCopia,NombreNodo) \n");
    	void _imprimir_info_bloque(bloqueArchivo *self){

    		if(self->bloquenodo0 != -1) printf("COPIA0: %d - (%d,%s)\n",i,self->bloquenodo0,self->nodo0);
    		if(self->bloquenodo1 != -1) printf("COPIA1: %d - (%d,%s)\n",i,self->bloquenodo1,self->nodo1);
    		printf("Bytes en Bloque %d:%d\n",i,self->bytesEnBloque);
    		i++;
    	}
    	list_iterate(fi->bloques,(void *)_imprimir_info_bloque);
    	liberar_char_array(split);
    	log_info(logi,"Usuario solicito info de arhcivo:%s",split[1]);


    }else{
        printf("La cantidad de parámetros es incorrecta, ingrese '%s? info%s' para más información\n", cyan, sin);
        log_info(logi,"Usuario ejecutó mal comando info");
        liberar_char_array(split);
    }
    return 0;
}

comando *buscar_comando(char *nombre) {

    int i;
    for (i = 0; i < 14; i++) {
        if (string_starts_with(nombre, commands[i].name) == 1) {
            return (&commands[i]);
        }
    }
    return ((comando *) NULL);

}

void deleteArchive(char *path) {
    char **pathSplit = sacar_archivo(path);
    char *directory = pathSplit[0];
    char *archive = pathSplit[1];
    int indexDirectory, i, cantFreedBlock;

    log_info(logi, "Verificando path");
    if ((indexDirectory = existe_ruta_directorios(directory)) == -9) {
        liberar_char_array(pathSplit);
        log_error(logi, "No existe directorio");
        return;
    }
    if (!existe_archivo(archive, indexDirectory)) {
        liberar_char_array(pathSplit);
        log_error(logi, "No existe Achivo");
        return;
    }

    log_info(logi, "Buscando Archivo");
    bool _searchByName(t_archivo *item) {
        return (strcmp(item->nombre, archive) == 0 && item->index_padre == indexDirectory);
    }
    t_archivo *foundArchive = list_find(archivos, (void *) _searchByName);
    if (foundArchive == NULL) {
        liberar_char_array(pathSplit);
        log_error(logi, "No se encontro el archivo");
        return;
    }

    log_info(logi, "Liberando los Bloques");
    cantFreedBlock = 0;
    bloqueArchivo *fetchBlock;
    for (i = 0; i < foundArchive->bloques->elements_count; ++i) {
        fetchBlock = list_get(foundArchive->bloques, i);
        if (strlen(fetchBlock->nodo0)) {
            if (!liberarBloqueNodo(fetchBlock->nodo0, (unsigned int) fetchBlock->bloquenodo0)) {
                log_error(logi, "No se pudo liberar el bloque[%d] del nodo %s", fetchBlock->bloquenodo0,
                          fetchBlock->nodo0);
            }
            cantFreedBlock++;
        }
        if (strlen(fetchBlock->nodo1)) {
            if (!liberarBloqueNodo(fetchBlock->nodo1, (unsigned int) fetchBlock->bloquenodo1)) {
                log_error(logi, "No se pudo liberar el bloque[%d] del nodo %s", fetchBlock->bloquenodo1,
                          fetchBlock->nodo1);
            }
            cantFreedBlock++;
        }
    }
    log_info(logi, "Cantidad de bloques liberados %d", cantFreedBlock);

    log_info(logi, "Eliminando archivo");

    eliminar_metadata_archivo(foundArchive);

    log_info(logi, "Removiendo archivo de la lista");
    void _freeMemoryBlock(bloqueArchivo *item) {
        free(item->nodo0);
        free(item->nodo1);
        free(item);
    }
    void _freeMemoryArchive(t_archivo *item) {
        free(item->nombre);
        free(item->tipo);
        list_destroy_and_destroy_elements(item->bloques, (void *) _freeMemoryBlock);
    }
    list_remove_and_destroy_by_condition(archivos, (void *) _searchByName, (void *) _freeMemoryArchive);

    actualizar_tabla_nodos();

}

void deleteDirectory(char *path) {
    int indexDirectory;
    log_info(logi, "Verificando path");
    if ((indexDirectory = existe_ruta_directorios(path)) == -9) {
        log_error(logi, "No existe directorio");
        return;
    }
    log_info(logi, "Verificando si esta vacio el directorio");
    if (!directoryEmpty(indexDirectory)) {
        log_error(logi, "El directorio no esta vacio");
        return;
    }
    log_info(logi, "Liberando directorio");
    t_directory *freeDirectory = &directorios[indexDirectory];
    strcpy(freeDirectory->nombre, "");
    freeDirectory->padre = -9;
    eliminar_directorio(indexDirectory);
    actualizar_arbol_directorios();
}

void deleteBlocksCpArchive(char *path, char *numBlock, char *numCopy) {
    int numberBlock, numberCopy, indexDirectory, i;
    char *isNumber = NULL;
    char **pathSplit = sacar_archivo(path);
    char *directory = pathSplit[0];
    char *archive = pathSplit[1];

    numberBlock = strtol(numBlock, &isNumber, 10);
    if (strlen(isNumber)) {
        liberar_char_array(pathSplit);
        log_error(logi, "el numero de bloque debe ser un entero");
        return;
    }
    numberCopy = strtol(numCopy, &isNumber, 10);
    if (strlen(isNumber)) {
        liberar_char_array(pathSplit);
        log_error(logi, "el numero de copia debe ser un entero");
        return;
    }

    log_info(logi, "Verificando path");
    if ((indexDirectory = existe_ruta_directorios(directory)) == -9) {
        liberar_char_array(pathSplit);
        log_error(logi, "No existe directorio");
        return;
    }

    if (!existe_archivo(archive, indexDirectory)) {
        liberar_char_array(pathSplit);
        log_error(logi, "No existe Achivo");
        return;
    }

    log_info(logi, "Buscando Archivo");

    bool _searchByName(t_archivo *item) {
        return (strcmp(item->nombre, archive) == 0 && item->index_padre == indexDirectory);
    }
    t_archivo *foundArchive = list_find(archivos, (void *) _searchByName);

    if (foundArchive == NULL) {
        liberar_char_array(pathSplit);
        log_error(logi, "No se encontro el archivo");
        return;
    }

    log_info(logi, "Verificando existencia del bloque");
    bloqueArchivo *fetchBlock = NULL;
    for (i = 0; i < foundArchive->bloques->elements_count; ++i) {
        if (i == numberBlock) {
            fetchBlock = list_get(foundArchive->bloques, i);
            break;
        }
    }
    if (fetchBlock == NULL) {
        log_error(logi, "No se encontro el bloque solicitado");
        return;
    }

    log_info(logi, "Verificando exitencia de copia");
    bool foundCopy = false;
    switch (numberCopy) {
        case 0: {
            if (strlen(fetchBlock->nodo0)) {
                foundCopy = true;
            }
            break;
        }
        case 1: {
            if (strlen(fetchBlock->nodo1)) {
                foundCopy = true;
            }
            break;
        }
        default: {
            log_error(logi, "El numero de copia debe ser 0 o 1");
            return;
        }
    }
    if (!foundCopy) {
        liberar_char_array(pathSplit);
        log_error(logi, "No se encontro la copia solicitada");
        return;
    }

    log_info(logi, "Verificando si es ultima copia");
    if (!strlen(fetchBlock->nodo0) || !strlen(fetchBlock->nodo1)) {
        liberar_char_array(pathSplit);
        log_error(logi, "No se puede borrar es la ultima copia");
        return;
    }

    log_info(logi, "Liberando bloque del Nodo y Archivo");
    if (numberCopy) {
        if (!liberarBloqueNodo(fetchBlock->nodo1, (unsigned int) fetchBlock->bloquenodo1)) {
            liberar_char_array(pathSplit);
            log_error(logi, "No se pudo liberar el bloque[%d] del nodo %s", fetchBlock->bloquenodo1,
                      fetchBlock->nodo1);
            return;
        }
        fetchBlock->nodo1 = strdup("");
        fetchBlock->bloquenodo1 = -1;
    } else {
        if (!liberarBloqueNodo(fetchBlock->nodo0, (unsigned int) fetchBlock->bloquenodo0)) {
            liberar_char_array(pathSplit);
            log_error(logi, "No se pudo liberar el bloque[%d] del nodo %s", fetchBlock->bloquenodo0,
                      fetchBlock->nodo0);
            return;
        }
        fetchBlock->nodo0 = strdup("");
        fetchBlock->bloquenodo0 = -1;
    }

    log_info(logi, "Persistiendo Cambios");
    actualizar_tabla_nodos();
    crear_metadata_archivo(foundArchive);

}
