/*
 * estructuras.h
 *
 *  Created on: 06/10/2013
 *      Author: utnso
 */

#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../commons/string.h"


typedef struct {
	int posX;
	int posY;
} t_posicion;

typedef struct {
	int32_t puerto;
	char ip[15];
} ipPuerto_t;


char * posicionToString(t_posicion * posicion);
char * charToString(char valor);
t_posicion * posicion_create();
t_posicion * posicion_create_pos(int x, int y);
t_posicion * posicion_create_pos_rand(int maxRandPos);


#endif /* ESTRUCTURAS_H_ */
