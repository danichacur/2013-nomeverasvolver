/*
 * estructuras.c
 *
 *  Created on: 19/10/2013
 *      Author: utnso
 */

#include "estructuras.h"

char * posicionToString(t_posicion * posicion){
	char * pos = string_new();
	string_append(&pos, string_from_format("%d",posicion->posX));
	string_append(&pos, ",");
	string_append(&pos, string_from_format("%d",posicion->posY));

	return pos;
}


char * charToString(char valor){
	char * res = string_new();
	string_append_with_format(&res, "%c", valor);
	return res;
}


t_posicion * posicion_create(){

	t_posicion * posicion = malloc(sizeof(t_posicion));
	posicion->posX =  0;
	posicion->posX =  0;

	return posicion;
}

t_posicion * posicion_create_pos(int x, int y){

	t_posicion * posicion = malloc(sizeof(t_posicion));
	posicion->posX =  x;
	posicion->posY =  y;

	return posicion;
}

t_posicion * posicion_create_pos_rand(int maxRandPos){

	t_posicion * posicion = malloc(sizeof(t_posicion));
	posicion->posX =  rand() % maxRandPos;
	posicion->posX =  rand() % maxRandPos;

	return posicion;
}

tRecursosNivel * recurso_create(char * nombre, char * simbolo, int instancias, int posX, int posY ){
	tRecursosNivel * recurso = malloc(sizeof(tRecursosNivel));
	recurso->nombre = nombre;
	recurso->simbolo = simbolo;
	recurso->instancias = instancias;
	recurso->posX = posX;
	recurso->posY = posY;

	return recurso;
}

t_personaje_niv * personaje_create_pos(char * simbolo, t_posicion * posicion ){
	t_personaje_niv * personaje = malloc(sizeof(t_personaje_niv));
	personaje->posicion = posicion;
	personaje->simbolo = simbolo;

	return personaje;
}

/*
static t_posicion * posicion_create(int posX, int posY){
	t_posicion *new = malloc(sizeof(t_posicion));
	new->posX = posX;
	new->posY = posY;

	return new;
}

static t_posicion * posicion_create_in_zero(){
	t_posicion *new = malloc( sizeof(t_posicion) );
	new->posX = 0;
	new->posY = 0;

	return new;
}

void fnAlPedo(){
	t_posicion *pos = posicion_create_in_zero();
	pos->posX = 22;
	pos->posY = 3;

	t_posicion *pos2 = posicion_create(4,3);
	pos2->posX = 22;
	pos2->posY = 3;

}*/
