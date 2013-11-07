/*
 * nivel.h
 *
 *  Created on: 23/10/2013
 *      Author: utnso
 */

#ifndef NIVEL_H_
#define NIVEL_H_

////////////////////////////////////////////////////BIBLIOTECAS ESTANDAR////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <unistd.h>
#include <string.h>

////////////////////////////////////////////////////BIBLIOTECAS COMMONS////////////////////////////////////////////////////
#include <sockets/sockets.h>
#include <sockets/mensajes.h>
#include <sockets/estructuras.h>
#include <commons/temporal.h>
#include <collections/list.h>
#include <commons/config.h>
#include <pthread.h>
#include "nivel.h"
#include <commons/log.h>
#include <tad_items.h>
#include <sys/inotify.h>
#include <stdlib.h>
#include <curses.h>

////////////////////////////////////////////////////ESPACIO DE DEFINICIONES////////////////////////////////////////////////////

int maxRandPos = 50;

typedef struct {
	char* nombre;
	char* simbolo;
	char* instancias;
	char* posX;
	char* posY;
}tRecursosNivel;

typedef struct {
	char* nombre;
	int quantum;
	int retardo;
	char* tipoAlgoritmo;
	int recovery;
	int enemigos;
	long tiempoDeadlock;
	long sleepEnemigos;
	char* direccionIPyPuerto;

}tNivel; // NO SE SI TIENE UTILIDAD DEFINIRLO COMO UN STRUCT, PUEDE SER UTIL PARA ENEMIGO Y PLATAFORMA


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

t_personaje * personaje_create(char * simbolo, t_posicion * posicion){
	t_personaje * personaje = malloc(sizeof(t_personaje));
	personaje->posicion = posicion;
	personaje->simbolo = simbolo;

	return personaje;
}
bool validarMovimientoPersonaje(char ** mensaje,ITEM_NIVEL * personaje);
int hilo_inotify(void);
void eliminarEstructuras();
void buscaPersonajeCercano();
void mensajesConPlataforma(int32_t socketEscucha);
void inicializarMapaNivel(t_list* listaRecursos);
void moverseAlternado();
void actualizarUltimoMovimiento();
void crearseASiMismo();
void movermeEnL();
void crearCaja(char ** caja);
int leerArchivoConfiguracion();
int32_t handshakeConPlataforma(); // handshake inicial
void enviarDatosInicioNivel(); //envia algoritmo,quantum y retardo
void crearHilosEnemigos();
void crearHiloInterbloqueo();
void crearHiloInotify();
int hilo_inotify(void);
void procesarSolicitudesPlanificador(int32_t socket, enum tipo_paquete tipoMensaje,char* mensaje);
ITEM_NIVEL * buscarRecursoEnLista(t_list * lista, char * simbolo);
ITEM_NIVEL * buscarPersonajeLista(t_list * lista, char * simbolo);
t_personaje * buscarPersonajeListaPersonajes(t_list * lista, char * simbolo);

#endif /* NIVEL_H_ */
