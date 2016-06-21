#!/bin/bash
gcc server.c -o server.o `mysql_config --cflags --libs` 
gcc client.c -o client.o 
./server.o
