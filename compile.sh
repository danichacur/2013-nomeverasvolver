#!/bin/sh
# nuestro compilador general :)
cd commons/Debug
make clean
make all
cd ../../plataforma/Debug/
make clean
make all

cd ../../personaje/Debug/
make clean
make all

cd ../../nivel/Debug/
make clean
make all

cd ../../koopa/Debug/
make clean
make all


