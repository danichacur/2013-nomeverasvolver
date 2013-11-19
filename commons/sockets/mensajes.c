/*
 * mensajes.c
 *
 *  Created on: 06/10/2013
 *      Author: utnso
 */


#define cant_paquetes 40

#include "mensajes.h"


static char *enum_packets_names[cant_paquetes] = {
		"PER_handshake_ORQ",
		"NIV_handshake_ORQ",
		"ORQ_handshake_PER",
		"ORQ_handshake_NIV",
		"PER_conexionNivel_ORQ",
		"PER_finPlanDeNiveles_ORQ",
		"PER_posCajaRecurso_PLA",
		"PER_movimiento_PLA",
		"PER_recurso_PLA",
		"PER_nivelFinalizado_PLA",
		"PER_meMori_PLA",
		"PLA_turnoConcedido_PER",
		"PLA_posCajaRecurso_PER",
		"PLA_movimiento_PER",
		"PLA_rtaRecurso_PER",
		"PLA_teMatamos_PER",
		"PLA_posCaja_NIV",
		"PLA_movimiento_NIV",
		"PLA_personajeMuerto_NIV",
		"PLA_nivelFinalizado_NIV",
		"PLA_solicitudRecurso_NIV",
		"PLA_personajeDesconectado_NIV",
		"NIV_posCaja_PLA",
		"NIV_movimiento_PLA",
		"NIV_recursoConcedido_PLA",
		"NIV_cambiosConfiguracion_PLA",
		"NIV_enemigosAsesinaron_PLA",
		"NIV_perMuereInterbloqueo_PLA",
		"OK1", //OK esta en la biblioteca ncurses que se usa para la gui del nivel, la toma como duplicada
		"NIV_datosPlanificacion_PLA",
		"PLA_nuevoPersonaje_NIV",
		"PLA_actualizarRecursos_NIV",
		"PLA_personajesDesbloqueados_NIV",
		"ORQ_conexionNivel_PER",
		"NIV_recursosPersonajesBloqueados_PLA",
		"PLA_recursosPersonajesBloqueados_NIV",

};



char *obtenerNombreEnum(enum tipo_paquete tipo) {
    if(tipo>cant_paquetes) {
        return "TIPO_INVALIDO";
    }
    return enum_packets_names[tipo];
}
