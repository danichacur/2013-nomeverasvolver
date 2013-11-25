
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
#include <stdbool.h>


/* Librerias de Commons */
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <sockets/mensajes.h>
#include <sockets/estructuras.h>
#include <sockets/sockets.h>
#include <collections/list.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

typedef struct {
	char *nombre;
	char *simbolo;
	int8_t cantVidas;
	t_list *niveles;
	char * ipOrquestador;
	int16_t puertoOrquestador;
	t_list *recursosNecesariosPorNivel;
	char * remain;
	t_list *recursosActualesPorNivel;
	t_list *posicionesPorNivel;
	t_list *ultimosMovimientosPorNivel;
} t_personaje;

typedef struct {
	char * nomNivel;
	t_posicion * posicionCaja;
} t_nivelProximaCaja;

typedef struct {
	char * nomNivel;
	int32_t fdNivel;
} t_descriptorPorNivel;

enum tipoMuertes{
	MUERTE_POR_ENEMIGO,
	MUERTE_POR_INTERBLOQUEO,
};

void levantarArchivoConfiguracion();
static t_personaje *personaje_create(char *nombre,char *simbolo,int8_t cantVidas,t_list * nivelesOrdenados,
										t_list * recursosPorNivel, t_list * listaDeNivelesConRecursosActualesVacios,
										char * ipOrquestador,int16_t puertoOrquestador, char* remain, t_list * posicionesPorNivel,
										t_list * ultimosMovimientosPorNivel);
/*static void personaje_destroy(t_personaje *self);*/
void* conectarAlNivel(int* nivel);
void reiniciarListasDeNivelARecomenzar(int ordNivel);
void conectarAlOrquestador();
//int todosNivelesFinalizados();
void avisarPlanNivelesConcluido();
void terminarProceso();
void personaje_destroy(t_personaje *self);
int tengoTodosLosRecursos(int nivel);
void recibirTurno();
int tengoPosicionProximaCaja(int nivel);
void solicitarYRecibirPosicionProximoRecurso(int nivel);
void realizarMovimientoHaciaCajaRecursos(int nivel);
void enviarNuevaPosicion(int nivel);
int estoyEnCajaRecursos(int nivel);
void solicitarRecurso(int nivel);
void avisarQueNoEstoyEnCajaRecursos(int nivel);
char* obtenerRecursosActualesPorNivel(int ordNivel);
char* obtenerRecursosNecesariosPorNivel(int ordNivel);
void avisarNivelConcluido(int nivel);
void desconectarPlataforma();
void tratamientoDeMuerte(enum tipoMuertes motivoMuerte,int ordNivel);
bool meQuedanVidas();
void descontarUnaVida();
void interrumpirTodosPlanesDeNivelesMenosActual(int nivelActual);
void interrumpirUnNivel(int nivel);
void finalizarTodoElProcesoPersonaje();
char * obtenerNombreNivelDesdeOrden(int ordNivel);
char * obtenerNumeroNivel(char * nomNivel);
void enviarHandshake(int ordNivel);
void recibirHandshake(int ordNivel);
void enviaSolicitudConexionANivel(int ordNivel);
void recibirUnMensaje(int32_t fd, enum tipo_paquete tipoEsperado, char ** mensajeRecibido, int ordNivel);
int32_t obtenerFDPlanificador(int ordNivel);
char *estoyEnLineaRectaALaCaja(int ordNivel);
void moverpersonajeEn(char * orientacion, int ordNivel);
char * obtenerProximoRecursosNecesario(int ordNivel);
void capturarSeniales();
void recibirSenialGanarVida();
void recibirSenialPierdeVida();
void desconectarmeDePlataforma(int ordNivel);

#endif /* PERSONAJE_H_ */
