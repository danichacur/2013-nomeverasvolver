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
	PER_dameUnTurno_PLA,
	PLA_turnoConcedido_PER,
	PER_posCajaRecurso_PLA,
	PLA_posCajaRecurso_PER,
	PER_movimiento_PLA,
	PLA_movimiento_PER,
	PER_recurso_PLA,
	PLA_rtaRecurso_PER,
	PER_nivelFinalizado_PLA,
	PER_meMori_PLA,
	PLA_teMatamos_PER,
	PLA_posCaja_NIV,
	NIV_posCaja_PLA,
	PLA_movimiento_NIV,
	NIV_movimiento_PLA,
	PLA_solicitudRecurso_NIV,
	NIV_recursoConcedido_PLA,
	NIV_cambiosConfiguracion_PLA,
	NIV_enemigosAsesinaron_PLA,
	NIV_perMuereInterbloqueo_PLA,
	OK
};


struct t_cabecera {
	enum tipo_paquete tipoP;
	uint32_t length;
} __attribute__ ((__packed__));


char *nombre_del_enum_paquete(enum tipo_paquete tipo);


#endif /* MENSAJES_H_ */
