#!/bin/bash
rm -rf *.o

gcc tmem.c -o tmem.c.o
gcc mallo.c -o malloc.c.o

./tmem.c.o firefox | perl driveGnuPlots.pl 4 400 400 400 400 vmsize vmdata vmstk vmRSS
