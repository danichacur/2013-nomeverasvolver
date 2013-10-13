/*
 * personaje.h
 *
 *  Created on: 13/10/2013
 *      Author: utnso
 */

#ifndef PERSONAJE_H_
#define PERSONAJE_H_

#include <stdio.h>
#include <stdlib.h>


/* Librerias de Commons */
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <sockets/sockets.h>
#include <sockets/mensajes.h>
#include <sockets/estructuras.h>
#include <collections/list.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

typedef struct {
	char *nombre;
	char *simbolo;
	int8_t cantVidas;
	t_list *niveles;
	char * ipOrquestador;
	int16_t puertoOrquestador;
	t_list *recursosNecesariosPorNivel;
	t_list *recursosActualesPorNivel;


} t_personaje;


void levantarArchivoConfiguracion();
static t_personaje *personaje_create(char *nombre,char *simbolo,int8_t cantVidas,t_list * nivelesOrdenados,
										char * ipOrquestador,int16_t puertoOrquestador);
/*static void personaje_destroy(t_personaje *self);*/
void* conectarAlNivel(int* nivel);
void conectarAlOrquestador();
int todosNivelesFinalizados();
void avisarPlanNivelesConcluido();
void terminarProceso();
void personaje_destroy(t_personaje *self);
int tengoTodosLosRecursos(int * nivel);
void recibirTurno();
int tengoPosicionProximaCaja(int * nivel);
void solicitarPosicionProximoRecurso(int * nivel);
void realizarMovimientoHaciaCajaRecursos(int * nivel);
void enviarNuevaPosicion(int * nivel);
int estoyEnCajaRecursos(int * nivel);
void solicitarRecurso(int * nivel);
void avisarNivelConcluido(int * nivel);
void desconectarPlataforma();
void tratamientoDeMuerte(int * motivoMuerte,int* nivel);
int meQuedanVidas();
void descontarUnaVida();
void desconectarmeDePlataforma(int * nivel);
void conectarAPlataforma(int * nivel);
void interrumpirTodosPlanesDeNiveles();
void finalizarTodoElProcesoPersonaje();

#endif /* PERSONAJE_H_ */
