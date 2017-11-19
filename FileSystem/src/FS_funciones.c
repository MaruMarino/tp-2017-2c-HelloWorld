/*
 * FS_funciones.c
 *
 *  Created on: 12/9/2017
 *      Author: utnso
 */


#include "FS_funciones.h"

#include <commons/string.h>
#include <funcionesCompartidas/generales.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "FS_administracion.h"
#include "estructurasfs.h"
#include "FS_interfaz_nodos.h"
#include "showState.h"

extern comando commands[];
extern yamafs_config *configuracion;
extern t_log *logi;
extern t_list *archivos;
extern t_directory directorios[100];
extern t_list *nodos;

#define cyan  "\x1B[36m"
#define sin "\x1B[0m"
#define Mib 0x100000

comando *buscar_comando(char *nombre);

int fs_ls(char *h) {
    printf("Ejecute ls \n");
    return 0;
}

int fs_rename(char *i) {
    printf("Ejecute rename \n");
    return 0;
}

int fs_format(char *j) {

    if (configuracion->inicio_limpio) {
        if(nodos->elements_count < 2){
            log_info(logi, "No se puede Formatiar se nesecita al menos 2 Nodo\n");
        }else{
            crear_subdirectorios();
            iniciar_arbol_directorios();
            iniciar_nodos();
            iniciar_bitmaps_nodos();

            configuracion->estado_estable = 1;
            configuracion->inicio_limpio = 0;
        }
    } else {
        log_info(logi, "No se puede Formatiar debe iniciar FileSystem con parametro --clean\n");
    }
    return 0;
}

int fs_mv(char *k) {
    printf("Ejecute mv\n");
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

int fs_rm(char *m) {
    printf("Ejecute rm\n");
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
    checkFileSystem();
    checkStateNodos();
    checkArchivos();
    printf("Ejecute cat \n");
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
            return 0;
        }


    } else {
        printf("La cantidad de parámetros es incorrecta, ingrese '%s? mkdir%s' para más información\n", cyan, sin);
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
        arch->cantbloques = (sizearchi % Mib != 0) ? (sizearchi / Mib + 1) : (sizearchi / Mib);
        arch->estado = no_disponible;
        arch->index_padre = padre;
        arch->nombre = strdup(nombre[1]);
        arch->tamanio = sizearchi;

        list_add(archivos, arch);
        liberar_char_array(split_paths);
        liberar_char_array(nombre);

    } else {
        printf("La cantidad de parámetros es incorrecta, ingrese '%s? cpfrom%s' para más información\n", cyan, sin);
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
    printf("Ejecute md5 \n");
    return 0;
}

int fs_info(char *u) {
    printf("Ejecute info \n");
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
