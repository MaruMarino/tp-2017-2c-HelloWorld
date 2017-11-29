#!/bin/bash

cd --

git clone https://github.com/sisoputnfrba/so-commons-library.git

cd so-commons-library

make

sudo make install

cd --

echo "Commons Instaladas"

cd /home/utnso/tp-2017-2c-HelloWorld/funcionesCompartidas

sudo make install

echo "Funciones Compartidas Instaladas"

cd --

sudo apt-get install libreadline6 libreadline6-dev

echo "ReadLine instalada"

mkdir /home/utnso/metadata/

mkdir /home/utnso/conf

cd /home/utnso/tp-2017-2c-HelloWorld/DataNode 

gcc -o "DataNode" ./src/*.c -lcommons -lcompartidas

cd ..

cd FileSystem

gcc  -o "FileSystem" ./src/FileSystem.c ./src/FS_administracion.c ./src/FS_conexiones.c ./src/FS_consola.c ./src/FS_funciones.c ./src/FS_interfaz_nodos.c ./src/showState.c ./src/estructurasfs.h ./src/FS_administracion.h ./src/FS_conexiones.h ./src/FS_consola.h ./src/FS_funciones.h ./src/FS_interfaz_nodos.h ./src/showState.h -lcommons -lpthread -lreadline -lcompartidas

cd ..

cd Master

gcc -o "Master" ./src/*.c -lcommons -lcompartidas -lpthread

cd ..

cd Worker

gcc -g3 -Wall -Wextra ./src/Worker.c ./src/configuracionWorker.c ./src/auxiliaresWorker.c ./src/nettingWorker.c ./src/rutinasChild.c -lcommons -lcompartidas -o W.out

cd ..

cd YAMA

gcc -g3 -Wall -Wextra ./src/YAMA.c ./src/clocks.c ./src/conexion_fs.c ./src/conexion_master.c ./src/manejo_tabla_estados.c -lcompartidas -lpthread -lcommons -o YAMA.out

cd 

echo "Si no hubo errores, el ejecutable se escontrara en la carpeta de cada proyecto, ejecutelo con normalidad"
echo "¡ Exitos !"
