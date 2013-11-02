/*
 * enemigo.h
 *
 *  Created on: 01/11/2013
 *      Author: utnso
 */

#ifndef ENEMIGO_H_
#define ENEMIGO_H_

typedef struct {
	char* nombre;
	char* simbolo;
	int instancias;
	int posX;
	int posY;
}tRecursosNivel;

void enemigo();

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////   nivel.h    //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <sockets/sockets.h>
#include <sockets/mensajes.h>
#include <sockets/estructuras.h>
#include <commons/temporal.h>
#include <collections/list.h>
#include <commons/config.h>
#include <pthread.h>

int maxRandPos = 50;

typedef struct {
        t_posicion * posicion;
        char * ultimoMovimiento;
        int cantTurnosEnL;
        char * orientacion1;
        int orientacion2;
} t_enemigo;

typedef struct {
		char * simbolo;
        t_posicion * posicion;
} t_personaje;

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

t_posicion * posicion_create_pos_rand(){

	t_posicion * posicion = malloc(sizeof(t_posicion));
	posicion->posX =  rand() % maxRandPos;
	posicion->posX =  rand() % maxRandPos;

	return posicion;
}

t_enemigo * enemigo_create(){
	t_enemigo * enemigo = malloc(sizeof(t_enemigo));

	//enemigo->posicion = posicion_create_pos_rand(); //TODO le saco que cree random la posicion para realizar pruebas
	enemigo->posicion = posicion_create_pos(1,0);

	enemigo->ultimoMovimiento = "V";
	enemigo->cantTurnosEnL = 0;
	enemigo->orientacion1 = "";
	enemigo->orientacion2 = 0;

	return enemigo;
}

t_personaje * personaje_create(char * simbolo, t_posicion * posicion){
	t_personaje * personaje = malloc(sizeof(t_personaje));
	personaje->posicion = posicion;
	personaje->simbolo = simbolo;

	return personaje;
}

//static void personaje_delete(t_personaje * personaje){
//	free(personaje);
//}

tRecursosNivel * recurso_create(char * nombre, char * simbolo, int instancias, int posX, int posY ){
	tRecursosNivel * recurso = malloc(sizeof(tRecursosNivel));
	recurso->nombre = nombre;
	recurso->simbolo = simbolo;
	recurso->instancias = instancias;
	recurso->posX = posX;
	recurso->posY = posY;

	return recurso;
}

t_enemigo * crearseASiMismo();
bool hayPersonajeAtacable();
t_personaje * moverseHaciaElPersonajeDeFormaAlternada(t_enemigo * enemigo);
char * estoyEnLineaRectaAlPersonaje(t_enemigo * enemigo, t_personaje * personaje);
void moverEnemigoEn(t_enemigo * enemigo, t_personaje * personaje, char * orientacion);
void moverEnemigoEnDireccion(t_enemigo * enemigo, char * orientacion1, int orientacion2);
bool hayCaja(int x, int y);
int obtenerDireccionCercaniaEn(char * orientacion, t_enemigo * enemigo, t_personaje * personaje);
t_personaje * buscaPersonajeCercano(t_enemigo * enemigo);
int distanciaAPersonaje(t_enemigo * enemigo, t_personaje * personaje);
t_list * buscarPersonajesAtacables();
bool personajeEstaEnCaja(t_personaje * personaje, int posX, int posY);
bool estoyArribaDeAlgunPersonaje(t_enemigo * enemigo);
void avisarAlNivel(t_enemigo * enemigo);
t_list * obtenerListaDePersonajesAbajoDeEnemigo(t_enemigo * enemigo);
void movermeEnL(t_enemigo * enemigo);

#endif /* ENEMIGO_H_ */
