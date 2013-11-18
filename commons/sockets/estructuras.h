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
#include "../collections/list.h"


typedef struct {
	int posX;
	int posY;
} t_posicion;

typedef struct {
	int32_t puerto;
	char ip[15];
} ipPuerto_t;

typedef struct {
	char* nombre;
	char* simbolo;
	int instancias;
	int posX;
	int posY;
}tRecursosNivel;

typedef struct {
		char * simbolo;
        t_posicion * posicion;
        t_list * recursosActuales;
        char * recursoBloqueante;
} t_personaje_niv;

char * posicionToString(t_posicion * posicion);
char * charToString(char valor);
t_posicion * posicion_create();
t_posicion * posicion_create_pos(int x, int y);
t_posicion * posicion_create_pos_rand(int maxRandPos);
tRecursosNivel * recurso_create(char * nombre, char * simbolo, int instancias, int posX, int posY );
t_personaje_niv * personaje_create_pos(char * simbolo, t_posicion * posicion );

#endif /* ESTRUCTURAS_H_ */
