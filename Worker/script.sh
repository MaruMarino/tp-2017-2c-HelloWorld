#!/bin/bash

gcc -g3 -Wall -Wextra ./src/Worker.c ./src/configuracionWorker.c ./src/auxiliaresWorker.c ./src/nettingWorker.c ./src/rutinasChild.c -lcommons -lcompartidas -o W.out
