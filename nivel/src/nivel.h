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
//#include <sockets/estructuras.h>
#include <commons/temporal.h>
#include <commons/string.h>
#include <collections/list.h>
#include <commons/config.h>
#include <pthread.h>
#include <nivel.h>
#include <signal.h>
#include <commons/log.h>
#include <tad_items.h>
#include <sys/inotify.h>
#include <stdlib.h>
#include <curses.h>
#include "enemigo.h"

////////////////////////////////////////////////////ESPACIO DE DEFINICIONES////////////////////////////////////////////////////




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
	        t_list * recursosActuales;
	        char * recursoBloqueante;
	        int ingresoSistema;
	} t_personaje_niv1;



void crearHiloInotify();
void eliminarEstructuras();
void  borrarPersonajeListaPersonajes(t_list * lista, char * simbolo);
void mensajesConPlataforma(int32_t socketEscucha);
void crearCaja(char ** caja);
int leerArchivoConfiguracion();
void liberarRecursosDelPersonaje(t_list *recursosActuales);
int32_t handshakeConPlataforma(); // handshake inicial
void enviarDatosInicioNivel(); //envia algoritmo,quantum y retardo
void * inotify(void);
void procesarSolicitudesPlanificador(int32_t socket, enum tipo_paquete tipoMensaje,char* mensaje);
bool determinarRecursoDisponible(char * recursoSolicitado);
ITEM_NIVEL * buscarRecursoEnLista(t_list * lista, char * simbolo);
ITEM_NIVEL * buscarPersonajeLista(t_list * lista, char * simbolo);
t_personaje_niv1 * buscarPersonajeListaPersonajes(t_list * lista, char * simbolo);
void crearHiloInterbloqueo();
void crearHilosEnemigos();
int dibujar (void);
bool existePersonajeEnListaItems(char idPers);
//t_personaje_niv1 * personaje_create_posicion(char * simbolo, t_posicion * posicion );
void personaje_destroy(t_personaje_niv1 * pers);



#endif /* NIVEL_H_ */
