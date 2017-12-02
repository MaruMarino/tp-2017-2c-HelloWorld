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
#include <sys/stat.h>
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


#define cyan  "\x1B[36m"
#define sin "\x1B[0m"
#define verde  "\x1B[32m"
#define rojo "\x1B[31m"
#define Mib 0x100000

comando *buscar_comando(char *nombre);

void deleteArchive(char *path);

void deleteDirectory(char *path);

void deleteBlocksCpArchive(char *path, char *numBlock, char *numCopy);

int fs_ls(char *h) {

    char **split = string_split(h, " ");
    int i = 0;
    while (split[i] != NULL) i++;

    if (i == 2) {

        int padre = existe_ruta_directorios(split[1]);
        if (padre == -9) {
            printf("%sNo existe ruta directorio%s\n", rojo, sin);
            log_info(logi, "Usuario pidio archivos de un path no existente %s ", split[1]);
            liberar_char_array(split);
            return 0;
        }

        void _imprimir_nombre(t_archivo *self) {

            if (self->index_padre == padre) printf("%s%s%s\n", verde, self->nombre, sin);
        }
        list_iterate(archivos, (void *) _imprimir_nombre);

        liberar_char_array(split);

    } else {

        printf("La cantidad de parámetros es incorrecta, ingrese '%s? ls%s' para más información\n", cyan, sin);
        liberar_char_array(split);
        log_info(logi, "Usuario ingresó mal el comando ls");
    }

    return 0;
}

int fs_rename(char *i) {

    char **split = string_split(i, " ");
    int ii = 0;
    while (split[ii] != NULL) ii++;

    if (ii == 4) {

        if (!strcmp(split[3], "-A") || !strcmp(split[3], "-a")) {

            t_archivo *archivo = get_metadata_archivo(split[1]);
            if (archivo == NULL) {
                printf("%sNo existe archivo original%s\n", rojo, sin);
                liberar_char_array(split);
                log_info(logi, "Usuario ingresó path original de archivo inexistente");
                return 0;
            }

            char **nuevo = sacar_archivo(split[1]);
            int padre = existe_ruta_directorios(nuevo[0]);
            bool existe = existe_archivo(split[2], padre);
            if (existe) {
                printf("%sYa existe un archivo con ese nombre en el directorio%s\n", rojo, sin);
                liberar_char_array(split);
                liberar_char_array(nuevo);
                log_info(logi, "Usuario ingresó nombre que ya existe");
                return 0;
            }
            char *pathNuevo = string_from_format("archivos/%d/%s", padre, split[2]);
            char *completo = completar_path_metadata(pathNuevo);
            char *pathViejo = string_from_format("archivos/%d/%s", padre, nuevo[1]);
            char *completo_viejo = completar_path_metadata(pathViejo);

            if (rename(completo_viejo, completo) == -1) {
                printf("%sNo se pudo renombrar%s\n", rojo, sin);
                liberar_char_array(split);
                liberar_char_array(nuevo);
                free(pathNuevo);
                free(completo);
                free(completo_viejo);
                free(pathViejo);
                log_info(logi, "Falló rename");
                return 0;
            }

            free(archivo->nombre);
            archivo->nombre = strdup(split[2]);
            printf("%sExitos: %s -> %s%s\n", verde, nuevo[1], split[2], sin);
            log_info(logi, "Exitos rename: %s -> %s", nuevo[1], split[2]);
            liberar_char_array(split);
            liberar_char_array(nuevo);
            free(pathNuevo);
            free(completo);
            free(completo_viejo);
            free(pathViejo);

        } else if (!strcmp(split[3], "-D") || !strcmp(split[3], "-d")) {

            int padre = existe_ruta_directorios(split[1]);
            if (padre == -9) {
                printf("%sNo existe path original%s\n", rojo, sin);
                log_info(logi, "Usuario quisó cambiar nombre de un directorio inexistente: %s", split[1]);
                liberar_char_array(split);
                return 0;
            }
            int existe = existe_dir_en_padre(split[2], directorios[padre].padre);
            if (existe != -9) {
                printf("%sYa existe un directorio con ese nombre%s\n", rojo, sin);
                log_info(logi, "Usuario quisó cambiar nombre a uno que ya existe");
                liberar_char_array(split);
                return 0;
            }

            memset(directorios[padre].nombre, '\0', 255);
            memcpy(directorios[padre].nombre, split[2], strlen(split[2]));
            actualizar_arbol_directorios();
            printf("%sExitos: %s -> %s%s\n", verde, split[1], split[2], sin);
            log_info(logi, "Exitos rename: %s -> %s \n", split[1], split[2]);
            liberar_char_array(split);
            return 0;
        } else {
            printf("Ingrese '%s? rename%s' para información sobre su sintaxis \n", cyan, sin);
            liberar_char_array(split);
            log_info(logi, "Usuario ingresó mal el comando rename");
        }

    } else {
        printf("La cantidad de parámetros es incorrecta, ingrese '%s? rename%s' para más información\n", cyan, sin);
        liberar_char_array(split);
        log_info(logi, "Usuario ingresó mal el comando renme");
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

                printf("%sNo se puede Formatear, se nesecitan al menos 2 Nodos%s\n",rojo,sin);
                log_info(logi, "No se puede Formatear, se nesecitan al menos 2 Nodos");

            } else if (!configuracion->estado_estable) {

                crear_subdirectorios();
                iniciar_arbol_directorios();
                iniciar_nodos();
                iniciar_bitmaps_nodos();

                configuracion->estado_estable = 1;
                printf("%s¡Formateo Exitoso!%s\n", verde, sin);

            } else {

                printf("%sEl File System ya fue fomateado %s\n",cyan,sin);
                log_info(logi, "El File System ya fue fomateado");
            }
        } else {

        	printf("%sEl File System ya fue fomateado %s\n",cyan,sin);
        	log_info(logi, "El File System ya fue fomateado ");
        }

    } else {
        log_info(logi, "Usuario ingreso mal el comando Format ¯\_(ツ)_/¯");
        printf("La cantidad de parámetros es incorrecta, ingrese '%s? format%s' para más información\n", cyan, sin);

    }
    liberar_char_array(split);

    return 0;
}

int fs_mv(char *k) {

    char **split = string_split(k, " ");
    int ii = 0;
    while (split[ii] != NULL) ii++;

    if (ii == 4) {

        if (!strcmp(split[3], "-A") || !strcmp(split[3], "-a")) {

            t_archivo *archivo = get_metadata_archivo(split[1]);
            if (archivo == NULL) {
                printf("%sNo existe archivo original%s\n",rojo,sin);
                liberar_char_array(split);
                log_info(logi, "Usuario ingresó path original de archivo inexistente");
                return 0;
            }
            char **nuevo = sacar_archivo(split[1]);
            int padre = existe_ruta_directorios(nuevo[0]);
            int existe = existe_ruta_directorios(split[2]);
            if (existe == -9) {
                printf("%sNo existe directorio destino%s\n", rojo, sin);
                liberar_char_array(split);
                liberar_char_array(nuevo);
                log_info(logi, "Usuario ingresó directorio destino inexistente");
                return 0;
            }
            char *pathNuevo = string_from_format("archivos/%d/%s", existe, nuevo[1]);
            char *completo = completar_path_metadata(pathNuevo);
            char *pathViejo = string_from_format("archivos/%d/%s", padre, nuevo[1]);
            char *completo_viejo = completar_path_metadata(pathViejo);

            if (rename(completo_viejo, completo) == -1) {
                printf("%sNo se pudo mover%s\n",rojo,sin);
                liberar_char_array(split);
                liberar_char_array(nuevo);
                free(pathNuevo);
                free(completo);
                free(completo_viejo);
                free(pathViejo);
                log_info(logi, "Falló mover");
                return 0;
            }

            archivo->index_padre = existe;
            printf("%sExitos: %s -> %s/%s %s\n", verde, split[1], split[2], nuevo[1], sin);
            log_info(logi, "Exitos: %s -> %s/%s \n", split[1], split[2], nuevo[1]);
            liberar_char_array(split);
            liberar_char_array(nuevo);
            free(pathNuevo);
            free(completo);
            free(completo_viejo);
            free(pathViejo);

        } else if (!strcmp(split[3], "-D") || !strcmp(split[3], "-d")) {

            int padre = existe_ruta_directorios(split[1]);
            if (padre == -9) {
                printf("%sNo existe path original%s\n",rojo,sin);
                log_info(logi, "Usuario quisó mover path inexistente: %s", split[1]);
                liberar_char_array(split);
                return 0;
            }

            int existe = existe_ruta_directorios(split[2]);
            if (existe == -9) {
                printf("%sNo existe directorio destino %s\n", rojo, sin);
                log_info(logi, "Usuario quisó mover un directorio a otro que no existe");
                liberar_char_array(split);
                return 0;
            }
            char **aux = sacar_archivo(split[1]);
            int yaHay = existe_dir_en_padre(aux[1], existe);
            if (yaHay != -9) {
                printf("%sYa existe un directorio con ese nombre en directorio destino%s\n", rojo, sin);
                log_info(logi, "Usuario quisó mover un directorio a otro que ya tiene uno con ese nombre");
                liberar_char_array(split);
                liberar_char_array(aux);
                return 0;
            }
            directorios[padre].padre = existe;
            actualizar_arbol_directorios();
            printf("%s¡Se movio el Directorio!%s\n",verde,sin);

            log_info(logi, "Se movio el Directorio");
            liberar_char_array(split);
            liberar_char_array(aux);

            return 0;
        } else {
            printf("Ingrese '%s? mv%s' para información sobre su sintaxis \n", cyan, sin);
            liberar_char_array(split);
            log_info(logi, "Usuario ingresó mal el comando mover");
        }

    } else {
        printf("La cantidad de parámetros es incorrecta, ingrese '%s? mover%s' para más información\n", cyan, sin);
        liberar_char_array(split);
        log_info(logi, "Usuario ingresó mal el comando mover");
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
        printf("%sFaltan argumentos para el comando rm %s\n", cyan, sin);
        log_error(logi, "Falta argumentos para el comando rm");
        liberar_char_array(arguments);
        return 0;
    }
    while (arguments[cantArgv] != NULL) cantArgv++;

    if (strcmp(arguments[1], "-d") == 0) {
        if (cantArgv > 3 || cantArgv < 3) {
            printf("Cantidad incorrecta de parámetros, ingrese %s'? rm'%s para más infomarción\n",cyan,sin);
        	log_error(logi, "verifique los argumentos para rm -d {path_directorio}");
            liberar_char_array(arguments);
            return 0;
        }
        deleteDirectory(arguments[2]);
    } else if (strcmp(arguments[1], "-b") == 0) {
        if (cantArgv > 5 || cantArgv < 5) {
            liberar_char_array(arguments);
            printf("Cantidad incorrecta de parámetros, ingrese %s'? rm'%s para más infomarción\n",cyan,sin);
            log_error(logi, "verifique los argumentos para rm -b {path_archivo} {nro_bloque} {nro_copia}");
            return 0;
        }
        deleteBlocksCpArchive(arguments[2], arguments[3], arguments[4]);
    } else {
        if (cantArgv > 2) {
            liberar_char_array(arguments);
            log_error(logi, "verifique los argumentos para rm {path_archivo}");
            printf("Cantidad incorrecta de parámetros, ingrese %s'? rm'%s para más infomarción\n",cyan,sin);

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
            printf("%sNo existe ruta %s\n",rojo,sin);
            return 0;
        }
        t_archivo *archivo = get_metadata_archivo_sinvalidar(dirName[1], padre);
        if (archivo == NULL) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("%sNo existe archivo%s\n",rojo,sin);
            return 0;
        }
        if (archivo->estado == no_disponible) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("%sArchivo no dispobible %s\n",cyan,sin);
            return 0;
        }

        bloqueArchivo *bq;
        int bloques = archivo->cantbloques, alternar = 0;
        char *buff;

        // todo: testar esto
        buff = pedirFile(archivo->bloques, (size_t) archivo->tamanio);
//         ya no deberia hacer falta entrar al ciclo for()

        //pthread_mutex_lock(&mutex_socket);
        /*for (i = 0; i < bloques; i++) {

            bq = list_get(archivo->bloques, i);

            buff = leer_bloque(bq, alternar);

            printf("%s%s%s", verde,buff,sin);

            free(buff);
            alternar = (alternar == 0) ? 1 : 0;
        }*/
        //pthread_mutex_unlock(&mutex_socket);
//        puts(buff);
//        free(buff);
        if(buff == NULL){
        	printf("%s%s%s",rojo,"no esta disponible el archivo",sin);
        }else{
        	printf("%s%s%s", verde, buff,sin);
        	free(buff);
        }
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
            if (padre != -9) {
                liberar_char_array(splt);
                printf("%sYa existe directorio%s\n",rojo,sin);
                return 0;
            }
            int d = agregar_directorio(splt[1], 0);
            if (d == -1) {
                liberar_char_array(splt);
                printf("%sNo hay lugar para un nuevo directorio%s\n",rojo,sin);
                return 0;
            }

            liberar_char_array(splt);
            printf("%s¡Se creó el Directorio!%s\n", verde, sin);
            actualizar_arbol_directorios();

            char *aux = string_from_format("archivos/%d", d);
            char *path = completar_path_metadata(aux);

            mkdir(path, 0775);
            free(aux);
            free(path);

            return 0;

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
            printf("%s¡Se creó el Directorio!%s\n", verde, sin);
            actualizar_arbol_directorios();
            char *aux = string_from_format("archivos/%d", d);
            char *path = completar_path_metadata(aux);

            mkdir(path, 0775);
            free(aux);
            free(path);

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
            printf("%sNo hay lugar para almacenar el archivo%s\n", rojo, sin);
            liberar_char_array(split_paths);
            return 0;
        }
        char **nombre = sacar_archivo(split_paths[1]);
        int padre = existe_ruta_directorios(split_paths[2]);
        if (padre == -9) {
            liberar_char_array(split_paths);
            liberar_char_array(nombre);
            printf("%sNo existe ruta final, cree los directorios que faltan e intente nuevamente%s\n",rojo,sin);
            return 0;
        }
        bool existe = existe_archivo(nombre[1], padre);
        if (existe) {
            liberar_char_array(split_paths);
            liberar_char_array(nombre);
            printf("%sYa existe un archivo con ese nombre%s\n", rojo, sin);
            return 0;
        }
        t_list *ba = escribir_desde_archivo(split_paths[1], split_paths[3][0], sizearchi);
        if (ba == NULL) {
            printf("%sNo existe archivo archivo en el FS local%s\n", rojo, sin);
            log_info(logi, "No existe archivo archivo en el FS local");
            liberar_char_array(split_paths);
            liberar_char_array(nombre);
            return 0;
        }

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
        printf("%s ¡Archivo Almacenado!%s\n", verde, sin);

    } else {
        printf("La cantidad de parámetros es incorrecta, ingrese '%s? cpfrom%s' para más información\n", cyan, sin);
        liberar_char_array(split_paths);
    }

    log_info(logi, "Ejecute cpfrom \n");
    return 0;
}

int fs_cpto(char *r) {
    char **split = string_split(r, " ");
    int i = 0;
    while (split[i] != NULL) i++;

    if (i == 3) {

        char **dirName = sacar_archivo(split[1]);
        int padre = existe_ruta_directorios(dirName[0]);
        if (padre == -9) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("%sNo existe ruta en yamafs%s\n", rojo, sin);
            return 0;
        }
        t_archivo *archivo = get_metadata_archivo_sinvalidar(dirName[1], padre);
        if (archivo == NULL) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("%sNo existe archivo en yamafs %s\n",rojo,sin);
            return 0;
        }
        if (archivo->estado == no_disponible) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("%sArchivo no dispobible en yamafs %s\n", rojo, sin);
            return 0;
        }
        struct stat info;
        if(stat(split[2],&info)==-1){
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("%sNo existe directorio de almacenado final en el FS local%s\n",rojo,sin);
            return 0;
        }
        if(!S_ISDIR(info.st_mode)){
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("%sPath de almacenado final en el FS local no es un directorio%s\n",rojo,sin);
            return 0;
        }
        char *path = string_from_format("%s/%s", split[2], dirName[1]);
        int temp = crear_archivo_temporal(archivo, path);
        if (temp == -1) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            free(path);
            printf("%sNo se pudo guardar el archivo en el FS-local %s\n", rojo, sin);
            return 0;
        }

        printf("%s¡Archivo copiado al FS local!%s\n",verde,sin);
        free(path);
        liberar_char_array(split);
        liberar_char_array(dirName);

    } else {

        printf("La cantidad de parámetros es incorrecta, ingrese '%s? cpto%s' para más información\n", cyan, sin);
        liberar_char_array(split);
    }

    return 0;
}

int fs_cpblock(char *s) {

    int numberBlock;
    char *isNumber = NULL;
	char **splt = string_split(s," ");
	int i = 0;
	while(splt[i]!=NULL) i++;

	if(i==4){

		t_archivo *arch = get_metadata_archivo(splt[1]);
		if(arch == NULL){
			printf("%sEl archivo o la ruta del mismo no existen%s\n",rojo,sin);
			log_error(logi,"El archivo o la ruta del mismo no existen\n");
			liberar_char_array(splt);
			return 0;
		}
	    numberBlock = strtol(splt[2], &isNumber, 10);
	    if (strlen(isNumber)) {
	    	liberar_char_array(splt);
	    	printf("%sEl numero de bloque debe ser un entero%s\n",rojo,sin);
	    	log_error(logi, "el numero de bloque debe ser un entero");
	        return 0;
	    }
		bloqueArchivo *bloque = list_get(arch->bloques,numberBlock);
		if(bloque == NULL){
	    	liberar_char_array(splt);
	    	printf("%sEl bloque no existe%s\n",rojo,sin);
	    	log_error(logi, "El bloque no existe");
	    	return 0;
		}
		if(bloque->bloquenodo0 != -1 && bloque->bloquenodo1 != -1){
	    	liberar_char_array(splt);
	    	printf("%sEl bloque elegido ya tiene dos copias%s\n",rojo,sin);
	    	log_error(logi, "El bloque elegido ya tiene dos copia");
	    	return 0;
		}

		int _buscar_nodo(NODO *self){

			return ( strcmp(self->nombre,splt[3]) == 0);
		}
		NODO *nodito = list_find(nodos,(void *)_buscar_nodo);

		if(nodito == NULL){
	    	liberar_char_array(splt);
	    	printf("%sNo existe nodo con ese nombre%s\n",rojo,sin);
	    	log_error(logi, "No existe nodo con ese nombre");
	    	return 0;
		}
		if(nodito->estado == no_disponible){
			liberar_char_array(splt);
			printf("%sEl nodo elegido no esta disponible %s\n",rojo,sin);
			log_error(logi, "El nodo elegido no esta disponible");
			return 0;
		}
		if(nodito->espacio_libre < Mib){
			liberar_char_array(splt);
	    	printf("%sEl nodo elegido no tiene espacio libre suficiente %s\n",rojo,sin);
	    	log_error(logi, "El nodo elegido no tiene espacio libre suficiente");
	    	return 0;
		}


		void *data = leer_bloque(bloque,0);
		if(data == NULL){
			liberar_char_array(splt);
	    	printf("%sNo se pudo acceder al bloque, nodos no disponibles %s\n",rojo,sin);
	    	log_error(logi, "No se pudo acceder al bloque, nodos no disponibles");
	    	return 0;
		}

		int control;
		size_t bufferWithBlockSize = (bloque->bytesEnBloque + sizeof(int));
		void *bufferWithBlock = malloc(bufferWithBlockSize);
		int nodoSendBlock = -1;
		for (i = 0; i < nodito->bitmapNodo->size; ++i) {
			if (!bitarray_test_bit(nodito->bitmapNodo, i)) {
				nodoSendBlock=i;
			}
		}
		if(nodoSendBlock == -1){
			liberar_char_array(splt);
			free(data);
			free(bufferWithBlock);
			printf("%sNodo elegido no tiene bloques libres %s\n",rojo,sin);
			log_error(logi, "Nodo elegido no tiene bloques libres");
			return 0;
		}
		if(bloque->bloquenodo0 == -1 && strcmp(bloque->nodo1,nodito->nombre) == 0){

			liberar_char_array(splt);
			free(data);
			free(bufferWithBlock);
			printf("%sUn bloque no puede estar copiado en el mismo nodo %s\n",rojo,sin);
			log_error(logi, "Un bloque no puede estar copiado en el mismo nodo ");
			return 0;

		}else if(bloque->bloquenodo1 == -1 && strcmp(bloque->nodo0,nodito->nombre) == 0){
				liberar_char_array(splt);
				free(data);
				free(bufferWithBlock);
				printf("%sUn bloque no puede estar copiado en el mismo nodo %s\n",rojo,sin);
				log_error(logi, "Un bloque no puede estar copiado en el mismo nodo ");
				return 0;

		}
		header reqRes;
		reqRes.codigo = 2;
		reqRes.letra = 'F';
		reqRes.sizeData = bufferWithBlockSize;

		memcpy(bufferWithBlock, &nodoSendBlock, sizeof(int));
		memcpy((bufferWithBlock + sizeof(int)), data, bloque->bytesEnBloque);
		message *request = createMessage(&reqRes, bufferWithBlock);

		if (enviar_messageIntr(nodito->soket, request, logi, &control) < 0) {
			liberar_char_array(splt);
			free(data);
			free(bufferWithBlock);
			free(request->buffer);
			free(request);
	    	printf("%sNodo elegido no disponible %s\n",rojo,sin);
	    	log_error(logi, "Nodo elegido no disponible");
	    	return 0;
		}

        bitarray_set_bit(nodito->bitmapNodo, nodoSendBlock);
        nodito->espacio_libre -= Mib;


        if(bloque->bloquenodo0 == -1){
        	if(strcmp(bloque->nodo1,nodito->nombre)== 0){
    			liberar_char_array(splt);
    			free(data);
    			free(bufferWithBlock);
    			free(request->buffer);
    			free(request);
    	    	printf("%sUn bloque no puede estar copiado en el mismo nodo %s\n",rojo,sin);
    	    	log_error(logi, "Un bloque no puede estar copiado en el mismo nodo ");
    	    	return 0;
        	}

        	bloque->bloquenodo0 = nodoSendBlock;
        	if(bloque->nodo0) free(bloque->nodo0);
        	bloque->nodo0= strdup(nodito->nombre);

        }else{
        	if(strcmp(bloque->nodo0,nodito->nombre)== 0){
    			liberar_char_array(splt);
    			free(data);
    			free(bufferWithBlock);
    			free(request->buffer);
    			free(request);
    	    	printf("%sUn bloque no puede estar copiado en el mismo nodo %s\n",rojo,sin);
    	    	log_error(logi, "Un bloque no puede estar copiado en el mismo nodo ");
    	    	return 0;
        	}

        	bloque->bloquenodo1 = nodoSendBlock;
        	if(bloque->nodo1) free(bloque->nodo1);
        	bloque->nodo1= strdup(nodito->nombre);
        }

        actualizar_tabla_nodos();
        crear_metadata_archivo(arch);

		liberar_char_array(splt);
		free(data);
		free(bufferWithBlock);
		free(request->buffer);
		free(request);
		printf("%s¡Bloque Copiado!\n%s",verde,sin);


	}else{
        printf("La cantidad de parámetros es incorrecta, ingrese '%s? cpblock%s' para más información\n", cyan, sin);
        liberar_char_array(splt);
	}

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
            printf("%sNo existe ruta%s\n", rojo, sin);
            return 0;
        }
        bool existe = existe_archivo(dirName[1], padre);
        if (!existe) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("%sNo existe archivo%s\n", rojo, sin);
            return 0;
        }
        t_archivo *arhcivo = get_metadata_archivo(split[1]);
        int temp = crear_archivo_temporal(arhcivo, "/tmp/auxiliar_md5");
        if (temp == -1) {
            liberar_char_array(split);
            liberar_char_array(dirName);
            printf("%sEl archivo no se encuentra disponible%s\n",rojo,sin);
            return 0;
        }
        char *linea = NULL;
        char **md;
        size_t len = 0;
        system("md5sum /tmp/auxiliar_md5 > /tmp/auxiliar_md5_2");
        FILE *f = fopen("/tmp/auxiliar_md5_2", "r");
        getline(&linea, &len, f);
        md = string_split(linea, " ");
        printf("%s%s%s\n", verde, md[0], sin);

        liberar_char_array(split);
        liberar_char_array(dirName);
        liberar_char_array(md);
        free(linea);
        fclose(f);

        unlink("/tmp/auxiliar_md5");
        unlink("/tmp/auxiliar_md5_2");

    } else {

        printf("La cantidad de parámetros es incorrecta, ingrese '%s? md5%s' para más información\n", cyan, sin);
        liberar_char_array(split);
    }

    return 0;
}

int fs_info(char *u) {
    char **split = string_split(u, " ");
    int i = 0;
    while (split[i] != NULL) i++;

    if (i == 2) {

        t_archivo *fi = get_metadata_archivo(split[1]);
        if (fi == NULL) {
            printf("%sNo existe archivo%s\n",rojo,sin);
            log_info(logi, "Usuario pidio info de un archivo que no se pudo encontrar: %s", split[1]);
            liberar_char_array(split);
            return 0;
        }

        printf("Nombre:%s\n", fi->nombre);
        printf("Tamanio: %d(bytes)- %d(bloques)\n", fi->tamanio, fi->cantbloques);
        printf("Estado:%s\n", getEstado(fi->estado));
        printf("Tipo:%s \n", fi->tipo);
        printf("Info Bloques:\n");
        int i = 0;
        printf("Copia # :#BloqueDeArchivo - (#BloqueEnNodo,NombreNodo) \n");
        void _imprimir_info_bloque(bloqueArchivo *self) {

            if (self->bloquenodo0 != -1) printf("COPIA0: %d - (%d,%s)\n", i, self->bloquenodo0, self->nodo0);
            if (self->bloquenodo1 != -1) printf("COPIA1: %d - (%d,%s)\n", i, self->bloquenodo1, self->nodo1);
            printf("Bytes en Bloque %d:%d\n", i, self->bytesEnBloque);
            i++;
        }
        list_iterate(fi->bloques, (void *) _imprimir_info_bloque);
        log_info(logi, "Usuario solicito info de arhcivo:%s", split[1]);
        liberar_char_array(split);


    } else if(strcmp(u,"info")==0){

    	checkFileSystem();
    	checkStateNodos();
    	checkdirectoris();
    	liberar_char_array(split);

    }else{
    	printf("La cantidad de parámetros es incorrecta, ingrese '%s? info%s' para más información\n", cyan, sin);
        log_info(logi, "Usuario ejecutó mal comando info");
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
        printf("%sNo existe directorio%s\n", rojo, sin);
        log_error(logi, "No existe directorio");
        return;
    }
    if (!existe_archivo(archive, indexDirectory)) {
        liberar_char_array(pathSplit);
        printf("%sNo existe archivo%s\n",rojo,sin);
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
        printf("%sNo se encontro el archivo%s\n",rojo,sin);
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
        free(item);
    }
    list_remove_and_destroy_by_condition(archivos, (void *) _searchByName, (void *) _freeMemoryArchive);

    actualizar_tabla_nodos();
    liberar_char_array(pathSplit);
    printf("%s¡Archivo eliminado!%s\n",verde,sin);

}

void deleteDirectory(char *path) {
    int indexDirectory;
    log_info(logi, "Verificando path");
    if ((indexDirectory = existe_ruta_directorios(path)) == -9) {
        printf("%sNo existe directorio%s\n", rojo, sin);
        log_error(logi, "No existe directorio");
        return;
    }
    log_info(logi, "Verificando si esta vacio el directorio");
    if (!directoryEmpty(indexDirectory)) {
        printf("%sEl directorio no esta vacío%s\n", rojo, sin);
        log_error(logi, "El directorio no esta vacio");
        return;
    }
    log_info(logi, "Liberando directorio");
    t_directory *freeDirectory = &directorios[indexDirectory];
    memset(freeDirectory->nombre,'\0',255);
    freeDirectory->padre = -9;
    eliminar_directorio(indexDirectory);
    actualizar_arbol_directorios();
    printf("%s¡Directorio Eliminado!%s\n", verde, sin);
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
    	printf("%sEl numero de bloque debe ser un entero%s\n",rojo,sin);
    	log_error(logi, "el numero de bloque debe ser un entero");
        return;
    }
    numberCopy = strtol(numCopy, &isNumber, 10);
    if (strlen(isNumber)) {
        liberar_char_array(pathSplit);
        printf("%sEl numero de copia debe ser un entero%s\n",rojo,sin);
        log_error(logi, "el numero de copia debe ser un entero");
        return;
    }

    log_info(logi, "Verificando path");
    if ((indexDirectory = existe_ruta_directorios(directory)) == -9) {
        liberar_char_array(pathSplit);
        printf("%sNo existe directorio%s\n",rojo,sin);
        log_error(logi, "No existe directorio");
        return;
    }

    if (!existe_archivo(archive, indexDirectory)) {
        liberar_char_array(pathSplit);
        printf("%sNo existe Archivo%s\n",rojo,sin);
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
        printf("%sNo existe archivo%s\n",rojo,sin);
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
    	liberar_char_array(pathSplit);
        printf("%sNo existe el bloque solicitado %s\n",rojo,sin);
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
            printf("%sEl numero de copia debe ser 0 o 1%s\n",rojo,sin);
            log_error(logi, "El numero de copia debe ser 0 o 1");
            liberar_char_array(pathSplit);
            return;
        }
    }
    if (!foundCopy) {
        liberar_char_array(pathSplit);
        printf("%sNo se encontro la copia solicitada%s\n",rojo,sin);
        log_error(logi, "No se encontro la copia solicitada");
        return;
    }

    log_info(logi, "Verificando si es ultima copia");
    if (!strlen(fetchBlock->nodo0) || !strlen(fetchBlock->nodo1)) {
        liberar_char_array(pathSplit);
        printf("%sNo se puede borrar, es la última copia%s\n",rojo,sin);
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
        free(fetchBlock->nodo1);
        fetchBlock->nodo1 = strdup("");
        fetchBlock->bloquenodo1 = -1;
    } else {
        if (!liberarBloqueNodo(fetchBlock->nodo0, (unsigned int) fetchBlock->bloquenodo0)) {
            liberar_char_array(pathSplit);
            log_error(logi, "No se pudo liberar el bloque[%d] del nodo %s", fetchBlock->bloquenodo0,
                      fetchBlock->nodo0);
            return;
        }
        free(fetchBlock->nodo0);
        fetchBlock->nodo0 = strdup("");
        fetchBlock->bloquenodo0 = -1;
    }

    liberar_char_array(pathSplit);
    log_info(logi, "Persistiendo Cambios");
    actualizar_tabla_nodos();
    crear_metadata_archivo(foundArchive);
    printf("%s¡Copia Eliminada! %s\n",verde,sin);

}
