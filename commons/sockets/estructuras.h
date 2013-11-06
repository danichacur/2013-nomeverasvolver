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

#endif /* ESTRUCTURAS_H_ */
