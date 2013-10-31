/*
 * mensajes.h
 *
 *  Created on: 06/10/2013
 *      Author: utnso
 */

#ifndef MENSAJES_H_
#define MENSAJES_H_

#include <stdint.h>
#include <sys/types.h>

enum tipo_paquete{
	PER_handshake_ORQ,
	NIV_handshake_ORQ,
	ORQ_handshake_PER,
	ORQ_handshake_NIV,
	PER_conexionNivel_ORQ,
	PER_finPlanDeNiveles_ORQ,
	PER_posCajaRecurso_PLA,
	PER_movimiento_PLA,
	PER_recurso_PLA,
	PER_nivelFinalizado_PLA,
	PER_meMori_PLA,
	PLA_turnoConcedido_PER,
	PLA_posCajaRecurso_PER,
	PLA_movimiento_PER,
	PLA_rtaRecurso_PER,
	PLA_teMatamos_PER,
	PLA_posCaja_NIV,
	PLA_movimiento_NIV,
	PLA_personajeMuerto_NIV,
	PLA_nivelFinalizado_NIV,
	PLA_solicitudRecurso_NIV,
	PLA_personajeDesconectado_NIV,
	NIV_posCaja_PLA,
	NIV_movimiento_PLA,
	NIV_recursoConcedido_PLA,
	NIV_cambiosConfiguracion_PLA,
	NIV_enemigosAsesinaron_PLA,
	NIV_perMuereInterbloqueo_PLA,
	OK1,		//OK esta en la biblioteca ncurses que se usa para la gui del nivel, la toma como duplicada
	NIV_datosPlanificacion_PLA,

};


struct t_cabecera {
	enum tipo_paquete tipoP;
	uint32_t length;
} __attribute__ ((__packed__));


char *obtenerNombreEnum(enum tipo_paquete tipo);


#endif /* MENSAJES_H_ */
