#!/bin/bash
rm 336.dat 080.dat;
./psdregdump 1 0 0 > 080.dat
./pharegdump 1 0 1 > 336.dat
