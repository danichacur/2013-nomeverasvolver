/*
 * enemigo.h
 *
 *  Created on: 01/11/2013
 *      Author: utnso
 */



#ifndef ENEMIGO_H_
#define ENEMIGO_H_

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
#include <tad_items.h>
#include <string.h>
//#include "nivel.h"

typedef struct {
        t_posicion * posicion;
        char * ultimoMovimiento;
        int cantTurnosEnL;
        char * orientacion1;
        int orientacion2;
        char id;
} t_enemigo;

void enemigo(int* pIdEnemigo);

t_enemigo * crearseASiMismo();
bool hayPersonajeAtacable();
t_personaje_niv * moverseHaciaElPersonajeDeFormaAlternada(t_enemigo * enemigo);
char * estoyEnLineaRectaAlPersonaje(t_enemigo * enemigo, t_personaje_niv * personaje);
void moverEnemigoEn(t_enemigo * enemigo, t_personaje_niv * personaje, char * orientacion);
void moverEnemigoEnDireccion(t_enemigo * enemigo, char * orientacion1, int orientacion2);
bool hayCajaOExcedeLimite(int x, int y);
bool hayCaja(int x, int y);
bool excedeLimite(int x, int y);
int obtenerDireccionCercaniaEn(char * orientacion, t_enemigo * enemigo, t_personaje_niv * personaje);
t_personaje_niv * buscaPersonajeCercano(t_enemigo * enemigo);
int distanciaAPersonaje(t_enemigo * enemigo, t_personaje_niv * personaje);
t_list * buscarPersonajesAtacables();
t_list * obtenerListaPersonajesAtacables();
t_list * obtenerListaCajas();
bool personajeEstaEnCaja(t_personaje_niv * personaje, int posX, int posY);
bool estoyArribaDeAlgunPersonaje(t_enemigo * enemigo);
void avisarAlNivel(t_enemigo * enemigo);
t_list * obtenerListaDePersonajesAbajoDeEnemigo(t_enemigo * enemigo);
void movermeEnL(t_enemigo * enemigo);
t_enemigo * enemigo_create();

#endif /* ENEMIGO_H_ */
