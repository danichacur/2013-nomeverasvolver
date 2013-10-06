/*
 * mensajes.c
 *
 *  Created on: 06/10/2013
 *      Author: utnso
 */


#define cant_paquetes 27

#include "mensajes.h"


static char *enum_packets_names[cant_paquetes] = {
	"PER_handshake_ORQ",
	"NIV_handshake_ORQ",
	"ORQ_handshake_PER",
	"ORQ_handshake_NIV",
	"PER_conexionNivel_ORQ",
	"PER_finPlanDeNiveles_ORQ",
	"PER_dameUnTurno_PLA",
	"PLA_turnoConcedido_PER",
	"PER_posCajaRecurso_PLA",
	"PLA_posCajaRecurso_PER",
	"PER_movimiento_PLA",
	"PLA_movimiento_PER",
	"PER_recurso_PLA",
	"PLA_rtaRecurso_PER",
	"PER_nivelFinalizado_PLA",
	"PER_meMori_PLA",
	"PLA_teMatamos_PER",
	"PLA_posCaja_NIV",
	"NIV_posCaja_PLA",
	"PLA_movimiento_NIV",
	"NIV_movimiento_PLA",
	"PLA_solicitudRecurso_NIV",
	"NIV_recursoConcedido_PLA",
	"NIV_cambiosConfiguracion_PLA",
	"NIV_enemigosAsesinaron_PLA",
	"NIV_perMuereInterbloqueo_PLA",
	"OK"
};



char *nombre_del_enum_paquete(enum tipo_paquete tipo) {
    if(tipo>cant_paquetes) {
        return "TIPO_INVALIDO";
    }
    return enum_packets_names[tipo];
}
