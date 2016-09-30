#!/bin/bash

./tmem.c.o firefox | perl driveGnuPlots.pl 4 400 400 400 400 vmsize vmdata vmstk vmRSS
