#!/bin/bash

gcc -g3 -Wall -Wextra ./src/YAMA.c ./src/clocks.c ./src/conexion_fs.c ./src/conexion_master.c ./src/manejo_tabla_estados.c -lcompartidas -lpthread -lcommons -o YAMA.out
