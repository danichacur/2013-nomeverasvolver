/*
 * estructuras.c
 *
 *  Created on: 19/10/2013
 *      Author: utnso
 */

//#include "estructuras.h"

char* posicionToString(t_posicion * posicion){
	char * pos = string_new();
	string_append(&pos, string_from_format("%d",posicion->posX));
	string_append(&pos, ",");
	string_append(&pos, string_from_format("%d",posicion->posY));

	return pos;
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
