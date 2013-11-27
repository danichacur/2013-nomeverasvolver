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
//#define CON_CONEXION false

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
int32_t tabla_fd[MAX_THREADS];

char * horizontal;
char * vertical;
char * cadenaVacia;

bool CON_CONEXION;

//TODO borrar esto, es para las pruebas.
t_list * listaDeNumeroCajaPorNivel;
bool finalizoCorrectamente;

//PROCESO PERSONAJE
int main(){

	//TODO borrar esto, es para las pruebas.
	listaDeNumeroCajaPorNivel = list_create();

	CON_CONEXION = true;
	horizontal = "H";
	vertical = "V";
	cadenaVacia = "";
	cantidadIntentosFallidos = 0;

	levantarArchivoConfiguracion();

	capturarSeniales(); // TODO

	int ordenNivel;
	while(1){
		finalizoCorrectamente = true;

		for (ordenNivel = 0 ; ordenNivel < list_size(personaje->niveles) ; ordenNivel++){
			pthread_t algo;// = tabla_thr[ordenNivel];
			pthread_create(&algo, NULL, (void*)&conectarAlNivel, (int*)ordenNivel);
			tabla_thr[ordenNivel] = algo;
		}

		for (ordenNivel = list_size(personaje->niveles)-1 ; ordenNivel >= 0  ; ordenNivel--)
			pthread_join(tabla_thr[ordenNivel],NULL);

		if (finalizoCorrectamente)
			break;
	}
	conectarAlOrquestador(-1);
	avisarPlanNivelesConcluido();

	terminarProceso();
	//personaje_destroy(personaje);
	return 1;
}

void* conectarAlNivel(int* nroNivel){

	int ordNivel;
	ordNivel = (int) nroNivel;

	conectarAlOrquestador(ordNivel);
	enviarHandshake(ordNivel);
	recibirHandshake(ordNivel);
	enviaSolicitudConexionANivel(ordNivel);

	bool hubo_error = false;

	// aca mágicamente obtengo el fd del Planificador

	while(1){

		recibirTurno();

		if(tengoTodosLosRecursos(ordNivel))
			break;

		if(!tengoPosicionProximaCaja(ordNivel))
			solicitarYRecibirPosicionProximoRecurso(ordNivel);

		realizarMovimientoHaciaCajaRecursos(ordNivel);
		enviarNuevaPosicion(ordNivel);

		t_posicion * pos = list_get(personaje->posicionesPorNivel,ordNivel);
		if (pos->posX >= 100 || pos->posY >= 100){
			log_info(logger, "Personaje %s (%s) (nivel: %s) TUVO UN PROBLEMA Y PASO LA UBICACION 100 EN ALGUN EJE. REVEER.", personaje->nombre, personaje->simbolo, obtenerNombreNivelDesdeOrden(ordNivel));
			hubo_error = true;
			break;
		}

		if(estoyEnCajaRecursos(ordNivel))
			solicitarRecurso(ordNivel);
		else
			avisarQueNoEstoyEnCajaRecursos(ordNivel);
	}

	if(!hubo_error)
		avisarNivelConcluido(ordNivel);

	//desconectarPlataforma();

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

	//log_info(logger, "envio mensaje PER_meMori_PLA con el mensaje %s", personaje->simbolo);
	//enviarMensaje(obtenerFDPlanificador(ordNivel), PER_meMori_PLA, personaje->simbolo);
	//char * mensaje;
	//recibirUnMensaje(obtenerFDPlanificador(ordNivel), OK1, &mensaje,ordNivel);

	if(meQuedanVidas()){
		descontarUnaVida();
		desconectarmeDePlataforma(ordNivel);

		//Reinicio parametros de ese nivel
		reiniciarListasDeNivelARecomenzar(ordNivel);

		sleep(3);//dani asi me das tiempo a borrarte de las estructuras =p
		conectarAlNivel((int*) ordNivel);
	}else{

		/*int32_t fd;
		if( motivoMuerte == MUERTE_POR_QUEDAR_SIN_VIDAS){
			fd = obtenerFDPlanificador(0);
		}else{
			fd = obtenerFDPlanificador(ordNivel);
		}

		log_info(logger, "Personaje %s (%s) se quedó sin vidas, aviso al Planificador", personaje->nombre, personaje->simbolo);
		enviarMensaje(fd, PER_meMori_PLA, "0");
		char * mens;
		recibirUnMensaje(fd, OK1, &mens, ordNivel);
*/
		char* respuesta = malloc(sizeof(char));
		finalizoCorrectamente = false;
		interrumpirTodosPlanesDeNivelesMenosActual(ordNivel);
		log_info(logger, "Usted ha perdido todas sus vidas. Usted ya reintentó %d veces. Desea volver a comenzar? [S/N]\n", cantidadIntentosFallidos);

		while(1){
			scanf("%s",respuesta);
			if(string_equals_ignore_case(respuesta,"S") || string_equals_ignore_case(respuesta,"N")){
				break;
			}else{
				log_info(logger,"Dale flaco, te dije [S/N] ¬¬ \n");
			}
		}
		if(strcmp(respuesta,"S") == 0){
			log_info(logger,"Se reinicia plan de niveles");
			personaje->cantVidas = cantVidasDelArchivoDeConfiguracion;
			cantidadIntentosFallidos += 1;
		}else{
			log_info(logger,"bueno che, nos vimos en Disney");
			finalizarTodoElProcesoPersonaje();
		}
	}

	//aca mato el hilo que restaba
	interrumpirUnNivel(ordNivel);
}


void reiniciarListasDeNivelARecomenzar(int ordNivel){
	//Pongo en 0,0 la posicion del personaje en el nivel
	t_posicion * pos = list_get(personaje->posicionesPorNivel, ordNivel);
	pos = posicion_create_pos(0,0);
	list_replace(personaje->posicionesPorNivel, ordNivel,pos);

	//pongo en 0,0 la posicion de la caja que el peronsaje va a buscar
	t_nivelProximaCaja * nivelProximaCaja = list_get(listaDeUbicacionProximaCajaNiveles, ordNivel);
	nivelProximaCaja->posicionCaja = malloc(sizeof(t_posicion));
	nivelProximaCaja->posicionCaja->posX = 0;
	nivelProximaCaja->posicionCaja->posY = 0;
	list_replace(listaDeUbicacionProximaCajaNiveles, ordNivel,nivelProximaCaja);

	//creo una lista nueva para la lista de recursos Actuales del personaje en ese nivel
	t_list * listaRecActuales = list_get(personaje->recursosActualesPorNivel, ordNivel);
	listaRecActuales = list_create();
	list_replace(personaje->recursosActualesPorNivel, ordNivel,listaRecActuales);

	//pongo una cadena vacia en el ultimo movimiento del personaje de ese nivel
	char * ultimoMovimiento = list_get(personaje->ultimosMovimientosPorNivel, ordNivel);
	ultimoMovimiento  = string_new();
	list_replace(personaje->ultimosMovimientosPorNivel, ordNivel,ultimoMovimiento);
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
	char * nomNivel = string_new();
	if (ordNivel != -1)
		string_append(&nomNivel, obtenerNombreNivelDesdeOrden(ordNivel));

	int32_t fd;
	if (CON_CONEXION)
		fd = cliente_crearSocketDeConexion(personaje->ipOrquestador, personaje->puertoOrquestador);
	else
		fd = 1;

	if (fd > 0){
		if (ordNivel != -1){
			log_info(logger, "Personaje %s (%s) (nivel: %s) se conectó correctamente al Orquestador", personaje->nombre, personaje->simbolo, nomNivel);
			tabla_fd[ordNivel] = fd;
		}else{
			log_info(logger, "Personaje %s se conectó correctamente al Orquestador", personaje->nombre);
			fdOrquestador = fd;
		}
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
		t_list *listaDeRecursosParaTodosLosNiveles;
		t_list *listaDeNivelesConRecursosAsignadosVacio;
		t_list *listaConPosicionesEnCero;
		t_list * listaUltimosMovimientosPorNivel;
		////t_nivel *miNivel;
		int cantVidas;
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
		listaDeRecursosParaTodosLosNiveles = list_create();
		listaDeNivelesConRecursosAsignadosVacio = list_create();
		listaConPosicionesEnCero = list_create();
		listaUltimosMovimientosPorNivel = list_create();
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
			//Agrego a la lista de Recursos para TODOS los niveles
			list_add(listaDeRecursosParaTodosLosNiveles,listaRecursosPorNivel);

			//Agrego el nivel en la lista de posiciones de cajas
			t_nivelProximaCaja * nivelProximaCaja = malloc(sizeof(t_nivelProximaCaja));
			nivelProximaCaja->nomNivel = listaNiveles[i];
			nivelProximaCaja->posicionCaja = malloc(sizeof(t_posicion));
			nivelProximaCaja->posicionCaja->posX = 0;
			nivelProximaCaja->posicionCaja->posY = 0;
			list_add(listaDeUbicacionProximaCajaNiveles, nivelProximaCaja);

			//Agrego el nivel en la lista de descriptores por nivel
			t_descriptorPorNivel * descriptorNivel = malloc(sizeof(t_descriptorPorNivel));
			descriptorNivel->nomNivel = listaNiveles[i];
			descriptorNivel->fdNivel = 0;
			list_add(listaDeFilesDescriptorsPorNivel, descriptorNivel);

			//Agrego el nivel a la lista de recursosAsignados
			t_list *listaVacia = list_create();
			list_add(listaDeNivelesConRecursosAsignadosVacio,listaVacia);

			//Por nivel agrego una posicion en 0,0
			t_posicion *posicionCaja = malloc(sizeof(t_posicion));
			posicionCaja->posX = 0;
			posicionCaja->posY = 0;
			list_add(listaConPosicionesEnCero,posicionCaja);

			//por nivel agrego el ultimo movimiento, pongo '' ya que no realizó ninguno
			list_add(listaUltimosMovimientosPorNivel,string_new());

			////miNivel = nivel_create(listaNiveles[i],listaRecursosPorNivel);
			////list_add(niveles,miNivel);

			//TODO borrar esto, es para las pruebas.
			if(!CON_CONEXION){
				int * num = malloc(sizeof(int));
				int numero = 1;
				(*num) = numero;
				list_add(listaDeNumeroCajaPorNivel,num);
			}

			//pongo los objetivos en algun lado
			free(objetivos_key);
			i++;
		}
		char ** infoOrquestador = string_split(orquestador,":");
		string_append(&ipOrquestador, infoOrquestador[0]);
		puertoOrquestador = atoi(infoOrquestador[1]);

		//creo el Personaje con todos sus parametros
		personaje = personaje_create(nombre,simbolo,cantVidas,nombresNiveles, listaDeRecursosParaTodosLosNiveles, listaDeNivelesConRecursosAsignadosVacio,
				ipOrquestador,puertoOrquestador, remain, listaConPosicionesEnCero, listaUltimosMovimientosPorNivel);


		config_destroy(config);
}



static t_personaje *personaje_create(char *nombre,
										char *simbolo,
										int8_t cantVidas,
										t_list * niveles,
										t_list * recursosPorNivel,
										t_list * listaDeNivelesConRecursosActualesVacios,
										char * ipOrquestador,
										int16_t puertoOrquestador,
										char* remain,
										t_list * posicionesPorNivel,
										t_list * ultimosMovimientosPorNivel
									 ){
	t_personaje *new = malloc(sizeof(t_personaje));
	new->nombre = nombre;
	new->simbolo = simbolo;
	new->cantVidas = cantVidas;
	new->niveles = niveles;
	new->recursosNecesariosPorNivel = recursosPorNivel;
	new->recursosActualesPorNivel = listaDeNivelesConRecursosActualesVacios;
	new->ipOrquestador = ipOrquestador;
	new->puertoOrquestador = puertoOrquestador;
	new->remain = remain;
	new->posicionesPorNivel = posicionesPorNivel;
	new->ultimosMovimientosPorNivel = ultimosMovimientosPorNivel;
	return new;
}

/*void personaje_destroy(t_personaje *self){
	free(self->niveles);
	free(self);
}*/

void avisarPlanNivelesConcluido(){
	sleep(3);
	log_info(logger, "Personaje %s (%s) informa que finalizó su plan de niveles", personaje->nombre, personaje->simbolo);
	if (CON_CONEXION)
		enviarMensaje(fdOrquestador,PER_finPlanDeNiveles_ORQ,personaje->simbolo);
}

void terminarProceso(){

}

int tengoTodosLosRecursos(int nivel){
	 t_list *listaRecursosNecesarios = list_get(personaje->recursosNecesariosPorNivel,nivel);
	 t_list *listaRecursosActuales = list_get(personaje->recursosActualesPorNivel,nivel);

	 if(list_size(listaRecursosNecesarios) == list_size(listaRecursosActuales)){
		 return true;
	 }else{
		 return false;
	 }
 }

void recibirTurno(int ordNivel){
	char* mensaje;
	//recibirUnMensaje(obtenerFDPlanificador(ordNivel), PLA_turnoConcedido_PER, &mensaje, ordNivel);
	recibirUnMensaje(obtenerFDPlanificador(ordNivel), PLA_turnoConcedido_PER, &mensaje, ordNivel);
	free(mensaje);

}

int tengoPosicionProximaCaja(int ordNivel){
	t_nivelProximaCaja * nivProxCaja = list_get(listaDeUbicacionProximaCajaNiveles, ordNivel);
	return !(nivProxCaja->posicionCaja->posX == 0 && nivProxCaja->posicionCaja->posY == 0);
}

void solicitarYRecibirPosicionProximoRecurso(int ordNivel){
	char * proximoRecursoNecesario  = obtenerProximoRecursosNecesario(ordNivel);

	log_info(logger, "Personaje %s (%s) (nivel: %s) solicita la ubicacion de la caja del recurso %s", personaje->nombre, personaje->simbolo, obtenerNombreNivelDesdeOrden(ordNivel), proximoRecursoNecesario);
	if (CON_CONEXION)
		//enviarMensaje(obtenerFDPlanificador(ordNivel), PER_posCajaRecurso_PLA, proximoRecursoNecesario);
		enviarMensaje(obtenerFDPlanificador(ordNivel), PER_posCajaRecurso_PLA, proximoRecursoNecesario);

	char * mensaje;

	//recibirUnMensaje(obtenerFDPlanificador(ordNivel), PLA_posCajaRecurso_PER, &mensaje, ordNivel);
	recibirUnMensaje(obtenerFDPlanificador(ordNivel), PLA_posCajaRecurso_PER, &mensaje, ordNivel);

	///////////////////   TODO HARDCODEADO por archConf
	if(!CON_CONEXION){
		config = config_create("./posiciones.conf");

		char * clave = string_new();
		int * num = list_get(listaDeNumeroCajaPorNivel,ordNivel);
		int numero = (*num);
		string_append_with_format(&clave, "%s%d%d", "POS", ordNivel, numero);
		mensaje = string_new();
		string_append(&mensaje, config_get_string_value(config, clave));
		free(clave);

		numero++;
		(*num) = numero;
		list_replace(listaDeNumeroCajaPorNivel,ordNivel, num);
		config_destroy(config);
	}
	/////////////////////////////////////////

	log_info(logger, "Personaje %s (%s) (nivel: %s) recibió la ubicacion del recurso %s, y es %s", personaje->nombre, personaje->simbolo, obtenerNombreNivelDesdeOrden(ordNivel), proximoRecursoNecesario, mensaje);
	char ** arr = string_split(mensaje,",");

	t_posicion * nuevaPosicion = malloc(sizeof(t_posicion));
	nuevaPosicion->posX = atoi(arr[0]);
	nuevaPosicion->posY = atoi(arr[1]);

	t_nivelProximaCaja * nivelProxCaja = list_get(listaDeUbicacionProximaCajaNiveles, ordNivel);
	//free(nivelProxCaja->posicionCaja);
	nivelProxCaja->posicionCaja = nuevaPosicion;
	list_replace(listaDeUbicacionProximaCajaNiveles,ordNivel,nivelProxCaja);
}

void realizarMovimientoHaciaCajaRecursos(int ordNivel){
	t_posicion * verdaderaPosicionAnterior = malloc(sizeof(t_posicion));
	t_posicion * posicionAnterior = list_get(personaje->posicionesPorNivel,ordNivel);
	memcpy(verdaderaPosicionAnterior, posicionAnterior, sizeof(t_posicion));

	t_posicion * posicionFinal;



	char * ultimoMovimientoPersonajeEnNivel;

	ultimoMovimientoPersonajeEnNivel = list_get(personaje->ultimosMovimientosPorNivel,ordNivel);

	char * condicion = estoyEnLineaRectaALaCaja(ordNivel); // 'H','V' o ''
	char * direccion;

	if(condicion == cadenaVacia) //si no estoy en linea recta a la caja,
		if(ultimoMovimientoPersonajeEnNivel == horizontal)
			direccion = vertical;
		else if(ultimoMovimientoPersonajeEnNivel == vertical)
			direccion = horizontal;
		else //primer movimiento
			direccion = horizontal;
	else //si estoy en linea recta, condicion me dice la direccion en la que me tengo que mover
		direccion = condicion;

	moverpersonajeEn(direccion, ordNivel);

	posicionFinal = list_get(personaje->posicionesPorNivel,ordNivel);

	log_info(logger, "Personaje %s (%s) (nivel: %s) se movio de posicion %s a posicion %s. Direccion %s", personaje->nombre, personaje->simbolo, obtenerNombreNivelDesdeOrden(ordNivel),
			posicionToString(verdaderaPosicionAnterior),
			posicionToString(posicionFinal),
			direccion);

}

void enviarNuevaPosicion(int ordNivel){
	t_posicion* posicion = list_get(personaje->posicionesPorNivel,ordNivel);
	if (CON_CONEXION)
		//enviarMensaje(obtenerFDPlanificador(ordNivel), PER_movimiento_PLA, posicionToString(posicion));
		enviarMensaje(obtenerFDPlanificador(ordNivel), PER_movimiento_PLA, posicionToString(posicion));
	log_info(logger, "Personaje %s (%s) (nivel: %s) envió su posicion %s al Planificador", personaje->nombre, personaje->simbolo, obtenerNombreNivelDesdeOrden(ordNivel), posicionToString(posicion));

	char * mensaje;

	recibirUnMensaje(obtenerFDPlanificador(ordNivel), PLA_movimiento_PER, &mensaje, ordNivel);
	if(!strcmp(mensaje,"1"))
		log_info(logger, "Personaje %s (%s) (nivel: %s) se fue del mapa. Se rompió todo", personaje->nombre, personaje->simbolo, obtenerNombreNivelDesdeOrden(ordNivel));
}

int estoyEnCajaRecursos(int ordNivel){
	t_posicion * posicionActual = list_get(personaje->posicionesPorNivel,ordNivel);
	t_nivelProximaCaja * nivelProxCajaBuscada = list_get(listaDeUbicacionProximaCajaNiveles, ordNivel);

	int estoy = false;

	if(posicionActual->posX == nivelProxCajaBuscada->posicionCaja->posX && posicionActual->posY == nivelProxCajaBuscada->posicionCaja->posY)
		estoy = true;

	return estoy;
}

void solicitarRecurso(int nivel){ // solicita el recurso y se queda esperando una respuesta
	char * recursoNecesario = obtenerProximoRecursosNecesario(nivel);
	log_info(logger, "Personaje %s (%s) (nivel: %s) solicita el recurso %s", personaje->nombre, personaje->simbolo, obtenerNombreNivelDesdeOrden(nivel), recursoNecesario);
	if (CON_CONEXION)
		//enviarMensaje(obtenerFDPlanificador(nivel),PER_recurso_PLA,recursoNecesario);
		enviarMensaje(obtenerFDPlanificador(nivel),PER_recurso_PLA,recursoNecesario);

	char * mensaje;
	//recibirUnMensaje(obtenerFDPlanificador(nivel),PLA_rtaRecurso_PER,&mensaje,nivel);
	recibirUnMensaje(obtenerFDPlanificador(nivel),PLA_rtaRecurso_PER,&mensaje,nivel);

	//Agrego el recurso que me fue asignado
	list_add(list_get(personaje->recursosActualesPorNivel, nivel),recursoNecesario);

	//Vuelvo a poner en 0,0 la posicion de la proxima caja del nivel
	t_nivelProximaCaja * nivelProxCajaBuscada = list_get(listaDeUbicacionProximaCajaNiveles, nivel);
	nivelProxCajaBuscada->posicionCaja->posX = 0;
	nivelProxCajaBuscada->posicionCaja->posY = 0;
	list_replace(listaDeUbicacionProximaCajaNiveles, nivel, nivelProxCajaBuscada);

	//imprimo la lista de recursos actuales
	char * recursosActuales = obtenerRecursosActualesPorNivel(nivel);
	char * recursosNecesarios = obtenerRecursosNecesariosPorNivel(nivel);

	log_info(logger, "Personaje %s (%s) (nivel: %s) obtuvo el recurso %s. Ahora tiene: %s. Necesita: %s", personaje->nombre, personaje->simbolo, obtenerNombreNivelDesdeOrden(nivel), recursoNecesario, recursosActuales, recursosNecesarios);

	free(recursosActuales);
	free(recursosNecesarios);
}

void avisarQueNoEstoyEnCajaRecursos(int nivel){
	char * recursoNecesario = "0";
		//log_info(logger, "Personaje %s (%s) (nivel: %s) avisa que no está en la caja", personaje->nombre, personaje->simbolo, obtenerNombreNivelDesdeOrden(nivel));
		if (CON_CONEXION)
			enviarMensaje(obtenerFDPlanificador(nivel),PER_recurso_PLA,recursoNecesario);
}

char* obtenerRecursosActualesPorNivel(int ordNivel){
	char * recursosActuales = string_new();
	int i;
	t_list * recursosActualesDelNivel = list_get(personaje->recursosActualesPorNivel, ordNivel);
	for(i = 0 ; i < list_size(recursosActualesDelNivel) ; i++){
		char * unRecurso = list_get(recursosActualesDelNivel,i);
		string_append(&recursosActuales, unRecurso);
	}
	return recursosActuales;
}

char* obtenerRecursosNecesariosPorNivel(int ordNivel){
	char * recursosNecesarios = string_new();
	int i;
	t_list * recursosNecesariosDelNivel = list_get(personaje->recursosNecesariosPorNivel, ordNivel);
	for(i = 0 ; i < list_size(recursosNecesariosDelNivel) ; i++){
		char * unRecurso = list_get(recursosNecesariosDelNivel,i);
		string_append(&recursosNecesarios, unRecurso);
	}

	return recursosNecesarios;
}


void avisarNivelConcluido(int nivel){
	log_info(logger, "Personaje %s (%s) (nivel: %s) informa que finalizó dicho nivel -------------------------------",
			personaje->nombre, personaje->simbolo, obtenerNombreNivelDesdeOrden(nivel));
	if (CON_CONEXION)
		//enviarMensaje(obtenerFDPlanificador(nivel),PER_nivelFinalizado_PLA,"0");
		enviarMensaje(obtenerFDPlanificador(nivel) , PER_nivelFinalizado_PLA , personaje->simbolo);

}

void desconectarPlataforma(){
	close(fdOrquestador);
}

bool meQuedanVidas(){
	return personaje->cantVidas > 0;
}

void descontarUnaVida(){
	personaje->cantVidas = personaje->cantVidas - 1;
}

void interrumpirTodosPlanesDeNivelesMenosActual(int nivelActual){
	int ordenNivel;
	for (ordenNivel = 0 ; ordenNivel < list_size(personaje->niveles) ; ordenNivel++){
		if(nivelActual != ordenNivel){
			interrumpirUnNivel(ordenNivel);
		}
	}
}

void interrumpirUnNivel(int nivel){

	reiniciarListasDeNivelARecomenzar(nivel);

	log_info(logger, "Interrumpo el nivel %d", nivel);

	pthread_t idHilo;
	idHilo = tabla_thr[nivel];
	int32_t fd= tabla_fd[nivel];

	log_info(logger, "cierro el fd %d", fd);
	close(fd);

	log_info(logger, "cancelo el hilo");
	int v = pthread_cancel(idHilo);
	if (v == 0){
		log_info(logger, "Se ha matado el hilo Personaje %s (%s) del (nivel: %s) ", personaje->nombre, personaje->simbolo, obtenerNombreNivelDesdeOrden(nivel));
	}else
		log_info(logger, "Error al matar el hilo Personaje %s (%s) del (nivel: %s) ", personaje->nombre, personaje->simbolo, obtenerNombreNivelDesdeOrden(nivel));

}



void finalizarTodoElProcesoPersonaje(){
	log_info(logger, "Personaje %s (%s) finaliza totalmente.", personaje->nombre, personaje->simbolo);
	kill(getpid(), SIGKILL);
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
	//string_append(&mensaje,personaje->remain);
	if (CON_CONEXION)
		enviarMensaje(obtenerFDPlanificador(ordNivel), PER_handshake_ORQ, mensaje);
	log_info(logger, "P1ersonaje %s (%s) (nivel: %s) envía handshake al Orquestador con los siguientes datos: %s", personaje->nombre, personaje->simbolo, nomNivel, mensaje);

	free(mensaje);
	free(nomNivel);
}

void recibirHandshake(int ordNivel){
	char* mensaje = string_new();
	char * nomNivel = obtenerNombreNivelDesdeOrden(ordNivel);

	enum tipo_paquete tipoRecibido;

	if (CON_CONEXION)
		recibirMensaje(obtenerFDPlanificador(ordNivel),&tipoRecibido,&mensaje);
	else
		tipoRecibido = ORQ_handshake_PER;

	if(tipoRecibido == ORQ_handshake_PER)
		log_info(logger, "Personaje %s (%s) (nivel: %s) recibe handshake del Orquestador", personaje->nombre, personaje->simbolo, nomNivel);
	else
		log_info(logger, "Personaje %s (%s) (nivel: %s) recibe un mensaje incorrecto del Orquestador. Recibe: ", personaje->nombre, personaje->simbolo, nomNivel, obtenerNombreEnum(tipoRecibido));
	free(mensaje);
	free(nomNivel);
}

void enviaSolicitudConexionANivel(int ordNivel){ // TODO ver como hago para volver a pedir el nivel mientras no esté conectado.
	char* mensaje = string_new();
	char * rta = string_new();
	char * nomNivel = obtenerNombreNivelDesdeOrden(ordNivel);
	enum tipo_paquete tipoMensaje;

	string_append(&mensaje,obtenerNumeroNivel(nomNivel));
	if (CON_CONEXION){
		while(1){
			enviarMensaje(obtenerFDPlanificador(ordNivel), PER_conexionNivel_ORQ, mensaje);
			//mensaje = NULL;
			log_info(logger, "Personaje %s (%s) (nivel: %s) pide al Orquestador conectarse al nivel: %s", personaje->nombre, personaje->simbolo, nomNivel, mensaje);
			recibirMensaje(obtenerFDPlanificador(ordNivel), &tipoMensaje, &rta);
			if( strcmp(rta,"0") == 0)
				break;
			else
				sleep(5);
			//validar que si es 1 vuelva a conectar
		}
	}

	free(mensaje);
	free(nomNivel);
}

void recibirUnMensaje(int32_t fd, enum tipo_paquete tipoEsperado, char ** mensajeRecibido, int ordNivel){
	enum tipo_paquete tipoMensaje;
	char* mensaje = NULL;
	//char * nomNivel = obtenerNombreNivelDesdeOrden(ordNivel);
	(*mensajeRecibido) = string_new();

	int retorno;
	if (CON_CONEXION)
		retorno = recibirMensaje(fd, &tipoMensaje, &mensaje);
	else{
		retorno = EXIT_SUCCESS;
		tipoMensaje = tipoEsperado;
		mensaje = "hola";
	}

	if (!retorno) {
		if( tipoMensaje != tipoEsperado){
			if (tipoMensaje == PLA_teMatamos_PER){
				//log_info(logger, "Personaje %s (%s) (nivel: %s) recibió un mensaje de muerte por enemigo",personaje->nombre, personaje->simbolo, nomNivel);
				tratamientoDeMuerte(MUERTE_POR_ENEMIGO, ordNivel);
			}else if (tipoMensaje == PLA_nivelCaido_PER){
				log_info(logger, "Nivel Caido. Finalizo todo abruptamente");
				finalizarTodoElProcesoPersonaje();
			}else{
				log_info(logger, "Mensaje Inválido. Se esperaba %s y se recibió %s", obtenerNombreEnum(tipoEsperado), obtenerNombreEnum(tipoMensaje));
			}
		}else{
			if (tipoMensaje == PLA_rtaRecurso_PER && strcmp(mensaje,"1") == 0){
				//log_info(logger, "Personaje %s (%s) (nivel: %s) recibió un mensaje de muerte por interbloqueo",personaje->nombre, personaje->simbolo, nomNivel);
				tratamientoDeMuerte(MUERTE_POR_INTERBLOQUEO, ordNivel);
			}else
				string_append(mensajeRecibido, mensaje);
		}
	}
}



int32_t obtenerFDPlanificador(int ordNivel){
	//t_descriptorPorNivel * descriptorNivel = list_get(listaDeFilesDescriptorsPorNivel, ordNivel);
	//return descriptorNivel->fdNivel;

	return tabla_fd[ordNivel];
}

char* posicionToString(t_posicion * posicion){
	char * pos = string_new();
	string_append(&pos, string_from_format("%d",posicion->posX));
	string_append(&pos, ",");
	string_append(&pos, string_from_format("%d",posicion->posY));

	return pos;
}

char * estoyEnLineaRectaALaCaja(int ordNivel){
	t_posicion * posicionActual = list_get(personaje->posicionesPorNivel,ordNivel);
	t_nivelProximaCaja * nivelProxCajaBuscada = list_get(listaDeUbicacionProximaCajaNiveles, ordNivel);

	char * condicion = cadenaVacia;

	if(posicionActual->posX == nivelProxCajaBuscada->posicionCaja->posX){
		condicion = vertical;

	}else if(posicionActual->posY == nivelProxCajaBuscada->posicionCaja->posY)
		condicion = horizontal;

	return condicion;
}

void moverpersonajeEn(char * orientacion, int ordNivel){
	t_posicion * posicionActual = list_get(personaje->posicionesPorNivel,ordNivel);
	t_nivelProximaCaja * caja = list_get(listaDeUbicacionProximaCajaNiveles,ordNivel);
	char * horizontal = "H";
	char * vertical = "V";

	if(orientacion == horizontal)
		if(posicionActual->posX > caja->posicionCaja->posX)
			posicionActual->posX = posicionActual->posX - 1;
		else
			posicionActual->posX = posicionActual->posX + 1;
	else if(orientacion == vertical){
		if(posicionActual->posY > caja->posicionCaja->posY)
			posicionActual->posY = posicionActual->posY - 1;
		else
			posicionActual->posY = posicionActual->posY + 1;
	}

	char * ultimoMovimiento = list_get(personaje->ultimosMovimientosPorNivel, ordNivel);
	ultimoMovimiento = orientacion;
	list_replace(personaje->ultimosMovimientosPorNivel, ordNivel, ultimoMovimiento);
}

char * obtenerProximoRecursosNecesario(int ordNivel){
	int cantRecursosObtenidos = list_size(list_get(personaje->recursosActualesPorNivel, ordNivel));
	char * proximoRecurso = list_get(list_get(personaje->recursosNecesariosPorNivel,ordNivel), cantRecursosObtenidos);

	return proximoRecurso;
}

void capturarSeniales(){
	signal(SIGUSR1, recibirSenialGanarVida);
	signal(SIGTERM, recibirSenialPierdeVida);

	log_info(logger, "Escucha de seniales levantada");
}

void recibirSenialGanarVida(){
	int cantVidasPrevias;
	int cantVidasPosterior;
	cantVidasPrevias = personaje->cantVidas;
	personaje->cantVidas = personaje->cantVidas + 1;
	cantVidasPosterior = personaje->cantVidas;
	log_info(logger, "El personaje %s tiene una vida mas. Tenia %d y ahora tiene %d", personaje->nombre, cantVidasPrevias, cantVidasPosterior); //, personaje->cantVidas);
}

void recibirSenialPierdeVida(){
	int cantVidasPrevias;
	int cantVidasPosterior;
	cantVidasPrevias = personaje->cantVidas;
	personaje->cantVidas = personaje->cantVidas - 1;

	cantVidasPosterior = personaje->cantVidas;
	log_info(logger, "El personaje %s tiene una vida menos. Tenia %d y ahora tiene %d", personaje->nombre, cantVidasPrevias, cantVidasPosterior); //, personaje->cantVidas);

	if(!meQuedanVidas())
		tratamientoDeMuerte(MUERTE_POR_QUEDAR_SIN_VIDAS, -1);

}


void desconectarmeDePlataforma(int ordNivel){
	close(obtenerFDPlanificador(ordNivel));
}
