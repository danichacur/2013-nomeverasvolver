/*
 * personaje.c
 *
 *  Created on: 05/10/2013
 *      Author: utnso
 */

#include "personaje.h"

#define FIRST_PART_PATH "./"
#define PATH_CONFIG "personaje.conf"
#define MAX_THREADS 10

//Variables de clase
t_config *config;
t_log* logger;
t_personaje *personaje;
int cantidadIntentosFallidos;
//t_list *listaDeNivelesIniciados;
t_list *listaDeNivelesFinalizados;
t_list *listaDeUbicacionProximaCajaNiveles;

int32_t fdOrquestador;
t_list *listaDeFilesDescriptorsPorNivel;

int cantVidasDelArchivoDeConfiguracion;

// tabla con los identificadores de los threads
pthread_t tabla_thr[MAX_THREADS];


//PROCESO PERSONAJE
int main(){
	cantidadIntentosFallidos = 0;

	levantarArchivoConfiguracion();

	int nivel;
	for (nivel = 0 ; nivel < list_size(personaje->niveles) ; nivel++)
		pthread_create(&tabla_thr[nivel], NULL, (void*)&conectarAlNivel, (void*)nivel);

	for (nivel = list_size(personaje->niveles)-1 ; nivel != 0  ; nivel--)
		pthread_join(tabla_thr[nivel],NULL);

	conectarAlOrquestador();
	avisarPlanNivelesConcluido();

	terminarProceso();
	//personaje_destroy(personaje);
	return 1;
}

void* conectarAlNivel(void* nroNivel){

	int ordNivel;
	ordNivel = (int) nroNivel;

	conectarAlOrquestador(nroNivel);
	enviarHandshake(ordNivel);
	recibirHandshake(ordNivel);
	enviaSolicitudConexionANivel(ordNivel);
	// aca mágicamente obtengo el fd del Planificador

	while(1){

		solicitarTurno(ordNivel);
		recibirTurno();

		if(!tengoPosicionProximaCaja(ordNivel))
			solicitarYRecibirPosicionProximoRecurso(ordNivel);

		realizarMovimientoHaciaCajaRecursos(ordNivel);
		enviarNuevaPosicion(ordNivel);

		if(estoyEnCajaRecursos(ordNivel)){
			solicitarRecurso(ordNivel);

		}

		if(tengoTodosLosRecursos(ordNivel))
			break;
	}

	avisarNivelConcluido(ordNivel);

	desconectarPlataforma();

	//liberarMemoria!!!
	pthread_exit(NULL);
	//return ptr;
}

void tratamientoDeMuerte(enum tipoMuertes motivoMuerte,int ordNivel){
	if(motivoMuerte == MUERTE_POR_ENEMIGO){
		log_info(logger, "Perdí una vida porque me alcanzó un enemigo");
	}else if( motivoMuerte == MUERTE_POR_INTERBLOQUEO){
		log_info(logger, "Perdí una vida porque estaba interbloqueado");
	}

	if(meQuedanVidas() == EXIT_SUCCESS){
		descontarUnaVida();
		desconectarmeDePlataforma(ordNivel);
		conectarAPlataforma(ordNivel);
	}else{
		char* respuesta = NULL;

		interrumpirTodosPlanesDeNiveles();
		printf("Usted ha perdido todas sus vidas. Usted ya reintentó %d veces. Desea volver a comenzar? [S/N]\n", cantidadIntentosFallidos);

		while(1){
			scanf("%s",respuesta);
			if(strcmp(respuesta,"S") == 0 || strcmp(respuesta,"N") == 0){
				break;
			}else{
				printf("Dale flaco, te dije [S/N] ¬¬ \n");
			}
		}
		if(strcmp(respuesta,"S") == 0){
			log_info(logger,"Se reinicia plan de niveles");
			cantidadIntentosFallidos += 1;
		}else{
			log_info(logger,"bueno che, nos vimos en Disney");
			finalizarTodoElProcesoPersonaje();
		}
	}
}

/*int todosNivelesFinalizados(){
	if(list_size(personaje->niveles) ==
			list_size(listaDeNivelesFinalizados)){
		return EXIT_SUCCESS;
	}else{
		return EXIT_FAILURE;
	}
}*/

void conectarAlOrquestador(int ordNivel){
	char * nomNivel = obtenerNombreNivelDesdeOrden(ordNivel);

	// TODO
	//int32_t fd = cliente_crearSocketDeConexion(personaje->ipOrquestador, personaje->puertoOrquestador);
	int32_t fd = 1;
	if (fd > 0){
		log_info(logger, "Personaje %s (nivel: %s) se conectó correctamente al Orquestador", personaje->nombre, nomNivel);
		fdOrquestador = fd;
	}

	free(nomNivel);
}

void levantarArchivoConfiguracion(){
	char * ruta =  string_new();
	string_append_with_format(&ruta,"%s%s",FIRST_PART_PATH,PATH_CONFIG);
	config = config_create(ruta);

		char *PATH_LOG = config_get_string_value(config, "PATH_LOG");
		logger = log_create(PATH_LOG, "PERSONAJE", true, LOG_LEVEL_INFO);

		//Variables para el Personaje
		char *nombre = string_new();
		char *simbolo = string_new();
		t_list *nombresNiveles;
		//t_list *niveles;
		t_list *listaRecursosPorNivel;
		////t_nivel *miNivel;
		int8_t cantVidas;
		char * ipOrquestador = string_new();
		int32_t puertoOrquestador;
		char *remain = string_new();


		//Variables adicionales
		//char *planificacionNiveles;
		char **listaNiveles;
		char* orquestador;

		//Obterngo del archivo de configuracion
		string_append(&nombre, config_get_string_value(config, "NOMBRE"));

		string_append(&simbolo ,config_get_string_value(config, "SIMBOLO"));


		listaNiveles = config_get_array_value(config, "PLANIFICACIONNIVELES");
		cantVidas = config_get_int_value(config, "VIDAS");
		cantVidasDelArchivoDeConfiguracion = cantVidas;
		orquestador = config_get_string_value(config, "ORQUESTADOR");

		string_append(&remain, config_get_string_value(config, "REMAIN"));

		nombresNiveles = list_create();
		listaDeUbicacionProximaCajaNiveles = list_create();
		listaDeFilesDescriptorsPorNivel = list_create();
		//niveles = list_create();

		int i=0;
		while(listaNiveles[i] != NULL){
			//Agrego el nombre del nivel en el orden de ejecucion
			list_add(nombresNiveles,listaNiveles[i]);

			//Agrego la lista de Recursos para ese nivel
			listaRecursosPorNivel = list_create();
			char* objetivos_key = string_from_format("obj[%s]", listaNiveles[i]);
			char** objetivosArray = config_get_array_value(config, objetivos_key);
			int j=0;
			while(objetivosArray[j] != NULL){
				list_add(listaRecursosPorNivel,objetivosArray[j]);
				j++;
			}

			//Agrego el nivel en la lista de posiciones de cajas
			/*t_nivelProximaCaja * nivelProximaCaja = malloc(sizeof(t_nivelProximaCaja));
			nivelProximaCaja->nomNivel = listaNiveles[i];
			nivelProximaCaja->posicionCaja = posicion_create_in_zero();
			list_add(listaDeUbicacionProximaCajaNiveles, nivelProximaCaja);
			*/ //FIXME

			//Agrego el nivel en la lista de descriptores por nivel
			t_descriptorPorNivel * descriptorNivel = malloc(sizeof(t_descriptorPorNivel));
			descriptorNivel->nomNivel = listaNiveles[i];
			descriptorNivel->fdNivel = 0;
			list_add(listaDeFilesDescriptorsPorNivel, descriptorNivel);

			////miNivel = nivel_create(listaNiveles[i],listaRecursosPorNivel);
			////list_add(niveles,miNivel);

			//pongo los objetivos en algun lado
			free(objetivos_key);
			i++;
		}
		char ** infoOrquestador = string_split(orquestador,":");
		string_append(&ipOrquestador, infoOrquestador[0]);
		puertoOrquestador = atoi(infoOrquestador[1]);

		//creo el Personaje con todos sus parametros
		personaje = personaje_create(nombre,simbolo,cantVidas,nombresNiveles, listaRecursosPorNivel, ipOrquestador,puertoOrquestador, remain);


		config_destroy(config);
}



static t_personaje *personaje_create(char *nombre,
										char *simbolo,
										int8_t cantVidas,
										t_list * niveles,
										t_list * recursosPorNivel,
										char * ipOrquestador,
										int16_t puertoOrquestador,
										char* remain
									 ){
	t_personaje *new = malloc( sizeof(t_personaje) );
	new->nombre = nombre;
	new->simbolo = simbolo;
	new->cantVidas = cantVidas;
	new->niveles = niveles;
	new->recursosNecesariosPorNivel = recursosPorNivel;
	new->recursosActualesPorNivel = list_create();
	new->ipOrquestador = ipOrquestador;
	new->puertoOrquestador = puertoOrquestador;
	new->remain= remain;
	return new;
}

/*void personaje_destroy(t_personaje *self){
	free(self->niveles);
	free(self);
}*/

void avisarPlanNivelesConcluido(){

}

void terminarProceso(){

}

int tengoTodosLosRecursos(int nivel){
	 t_list *listaRecursosNecesarios = list_get(personaje->recursosNecesariosPorNivel,nivel);
	 t_list *listaRecursosActuales = list_get(personaje->recursosActualesPorNivel,nivel);

	 if(list_size(listaRecursosNecesarios) == list_size(listaRecursosActuales)){
		 return EXIT_SUCCESS;
	 }else{
		 return EXIT_FAILURE;
	 }
 }

void solicitarTurno(int ordNivel) {
	//enviarMensaje(obtenerFDPlanificador(ordNivel), PER_dameUnTurno_PLA, "0");
}

void recibirTurno(int ordNivel){

	char* mensaje;
	recibirUnMensaje(obtenerFDPlanificador(ordNivel), PLA_turnoConcedido_PER, &mensaje, ordNivel);
	free(mensaje);

}

int tengoPosicionProximaCaja(int ordNivel){
/*	T0DO ESTO ERA SUPONIENDO QUE NO SABIA LA POSICION DE NI NIVELPROXIMACAJA EN LA LISTA, PERO SÍ LA SE.
 	IGUAL POR LAS DUDAS ME GUARDO EL CÓDIGO
 	char * nomNivel = obtenerNombreNivelDesdeOrden(ordNivel);
	int i = 0;
	int encontrado = 0;

	while(i < list_size(listaDeUbicacionProximaCajaNiveles) && encontrado == 0){
		t_nivelProximaCaja * nivProxCaja = list_get(listaDeUbicacionProximaCajaNiveles, i);
		if (nomNivel == nivProxCaja->nomNivel){
			encontrado = 1;
			break;
		}
	}
		free(nomNivel);
	*/

	t_nivelProximaCaja * nivProxCaja = list_get(listaDeUbicacionProximaCajaNiveles, ordNivel);
	if (nivProxCaja->posicionCaja->posX == 0 && nivProxCaja->posicionCaja->posY == 0)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}

void solicitarYRecibirPosicionProximoRecurso(int ordNivel){
	enviarMensaje(obtenerFDPlanificador(ordNivel), PER_posCajaRecurso_PLA, "0");

	/*char * mensaje;
	recibirUnMensaje(obtenerFDPlanificador(ordNivel), PLA_posCajaRecurso_PER, &mensaje, ordNivel);
	char ** arr = string_split(mensaje,",");

	t_posicion * nuevaPosicion;
	nuevaPosicion = posicion_create(atoi(arr[0]),atoi(arr[1]));

	t_nivelProximaCaja * nivelProxCaja = list_get(listaDeUbicacionProximaCajaNiveles, ordNivel);
	free(nivelProxCaja->posicionCaja);
	nivelProxCaja->posicionCaja = nuevaPosicion;
	*/ //FIXME
}

void realizarMovimientoHaciaCajaRecursos(int ordNivel){

}

void enviarNuevaPosicion(int ordNivel){

}

int estoyEnCajaRecursos(int ordNivel){
	return EXIT_SUCCESS;
}

void solicitarRecurso(int nivel){

}

void avisarNivelConcluido(int nivel){

}

void desconectarPlataforma(){

}

int meQuedanVidas(){
	return EXIT_SUCCESS;
}

void descontarUnaVida(){

}

void desconectarmeDePlataforma(int nivel){

}

void conectarAPlataforma(int nivel){

}
void interrumpirTodosPlanesDeNiveles(){

}
void finalizarTodoElProcesoPersonaje(){

}

char * obtenerNombreNivelDesdeOrden(int ordNivel){
	char * nomNivel = string_new();
	string_append(&nomNivel,list_get(personaje->niveles, ordNivel));
	return nomNivel;
}

char * obtenerNumeroNivel(char * nomNivel){
	char * numeroNivel = string_new();
	//string_append(&numeroNivel, string_substring(nomNivel,string_length(nomNivel)-1,string_length(nomNivel)));
	string_append(&numeroNivel, string_substring(nomNivel,strlen(nomNivel)-1,strlen(nomNivel)));
	return numeroNivel;
}

void enviarHandshake(int ordNivel){
	char* mensaje = string_new();
	char * nomNivel = obtenerNombreNivelDesdeOrden(ordNivel);

	string_append(&mensaje,personaje->simbolo);
	string_append(&mensaje,personaje->remain);
	//enviarMensaje(fdOrquestador, PER_handshake_ORQ, mensaje);
	log_info(logger, "Personaje %s (nivel: %s) envía handshake al Orquestador con los siguientes datos: %s", personaje->nombre, nomNivel, mensaje);

	free(mensaje);
	free(nomNivel);
}

void recibirHandshake(int ordNivel){
	char* mensaje = string_new();
	char * nomNivel = obtenerNombreNivelDesdeOrden(ordNivel);

	/*enum tipoMensaje tipoRecibido;

	//TODO
	recibirMensaje(fdOrquestador,tipoRecibido,&mensaje);
	tipoRecibido = ORQ_handshake_PER;

	if(tipoRecibido == ORQ_handshake_PER)
		log_info(logger, "Personaje %s (nivel: %s) recibe handshake del Orquestador", personaje->nombre, nomNivel);
	else
		log_info(logger, "Personaje %s (nivel: %s) recibe un mensaje incorrecto del Orquestador. Recibe: ", personaje->nombre, nomNivel, obtenerNombreEnum(tipoRecibido));
	 */ // FIXME
	free(mensaje);
	free(nomNivel);
}

void enviaSolicitudConexionANivel(int ordNivel){
	char* mensaje = string_new();
	char * nomNivel = obtenerNombreNivelDesdeOrden(ordNivel);

	string_append(&mensaje,obtenerNumeroNivel(nomNivel));
	//enviarMensaje(fdOrquestador, PER_conexionNivel_ORQ, mensaje);
	log_info(logger, "Personaje %s (nivel: %s) pide al Orquestador conectarse al nivel: %s", personaje->nombre, nomNivel, mensaje);

	free(mensaje);
	free(nomNivel);
}

void recibirUnMensaje(int32_t fd, enum tipo_paquete tipoEsperado, char ** mensajeRecibido, int ordNivel){
	enum tipo_paquete tipoMensaje;
	char* mensaje = NULL;
	char * nomNivel = obtenerNombreNivelDesdeOrden(ordNivel);

	//TODO
	//int retorno = recibirMensaje(fd, &tipoMensaje, &mensaje);
	int retorno = EXIT_SUCCESS;
	tipoMensaje = tipoEsperado;
	mensaje = "hola";


	if (!retorno) {
		if( tipoMensaje != tipoEsperado){
			if (tipoMensaje == PLA_teMatamos_PER){
				log_info(logger, "Personaje %s (nivel: %s) recibió un mensaje de muerte por enemigo",personaje->nombre, nomNivel);
				tratamientoDeMuerte(MUERTE_POR_ENEMIGO, ordNivel);
			}else{
				log_info(logger, "Mensaje Inválido. Se esperaba %s y se recibió %s", obtenerNombreEnum(tipoEsperado), obtenerNombreEnum(tipoMensaje));
			}
		}else{
			(*mensajeRecibido) = mensaje;
		}
	}
}

int32_t obtenerFDPlanificador(int ordNivel){
	t_descriptorPorNivel * descriptorNivel = list_get(listaDeFilesDescriptorsPorNivel, ordNivel);
	return descriptorNivel->fdNivel;
}
