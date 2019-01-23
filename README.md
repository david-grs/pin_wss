WSS (working set size) estimator
--------------------------------

### Build & run
```
$ PIN_ROOT=~/src/pin-3.7-97619-g0d0c92f4f-gcc-linux/ make
$ pin -t ./obj-intel64//wss.so -- ls
makefile  makefile.rules  obj-intel64  README.md  wss.cpp
exit
509348 instructions
167505 access
127388 reads
40117 writes
WSS=262kB
```
