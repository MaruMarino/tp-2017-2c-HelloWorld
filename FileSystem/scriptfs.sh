#!/bin/bash

#cd /home/utnso/tp-2017-2c-HelloWorld/FileSystem/

gcc  -o "FileSystem" ./src/FileSystem.c ./src/FS_administracion.c ./src/FS_conexiones.c ./src/FS_consola.c ./src/FS_funciones.c ./src/FS_interfaz_nodos.c ./src/estructurasfs.h ./src/FS_administracion.h ./src/FS_conexiones.h ./src/FS_consola.h ./src/FS_funciones.h ./src/FS_interfaz_nodos.h -lcommons -lpthread -lreadline -lcompartidas
