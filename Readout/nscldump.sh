#!/bin/bash



./psdregdump 1 0 0 > 080nscl.dat
./pharegdump 1 0 1 > 336nscl.dat

diff -y 080.dat 080nscl.dat > 080diff.dat
diff -y 336.dat 336nscl.dat > 336diff.dat

gedit 080diff.dat 336diff.dat & 
disown 
