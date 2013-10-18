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
int32_t fdOrquestador;
int cantVidasDelArchivoDeConfiguracion;
// tabla con los identificadores de los threads
pthread_t tabla_thr[MAX_THREADS];


int p = 5;

//PROCESO PERSONAJE
int main(){

	cantidadIntentosFallidos = 0;

	levantarArchivoConfiguracion();

	int nivel;

	for (nivel = 0 ; nivel < list_size(personaje->niveles) ; nivel++){

		pthread_create(&tabla_thr[nivel], NULL, (void*)&conectarAlNivel, (void*)nivel);
	}

	for (nivel = list_size(personaje->niveles)-1 ; nivel != 0  ; nivel--){
		pthread_join(tabla_thr[nivel],NULL);
	}


	conectarAlOrquestador();
	avisarPlanNivelesConcluido();


	terminarProceso();
	//personaje_destroy(personaje);
	return 1;
}

void* conectarAlNivel(void* nroNivel){

	int i ;
	int nivel;
	nivel = (int ) nroNivel;

	char * nomNivel = string_new();
	string_append(&nomNivel,list_get(personaje->niveles, nivel));

	for (i = 0; i<10; i++) {
		p = p + 1;
		printf("%s: %d\n",nomNivel, p);
		sleep(1);
	}
	free(nomNivel);

	/*conectarAlOrquestador();
	enviarMensaje(fdOrquestador, OK,"hola Dani");

	while(1){
		if(tengoTodosLosRecursos(nivel) == EXIT_SUCCESS){
			break;
		}

		recibirTurno();

		if(tengoPosicionProximaCaja(nivel) == EXIT_FAILURE){
			solicitarPosicionProximoRecurso(nivel);
		}

		realizarMovimientoHaciaCajaRecursos(nivel);
		enviarNuevaPosicion(nivel);

		if(estoyEnCajaRecursos(nivel)){
			solicitarRecurso(nivel);

		}
	}

	avisarNivelConcluido(nivel);

	desconectarPlataforma();
*/
	//liberarMemoria!!!
	pthread_exit(NULL);
	//return ptr;
}

void tratamientoDeMuerte(int * motivoMuerte,int* nivel){ //motivos: 1 enemigo. 2 interbloqueo
	if(*motivoMuerte == 1){
		log_info(logger, "Perdí una vida porque me alcanzó un enemigo %s","asda");
	}else{
		log_info(logger, "Perdí una vida porque estaba interbloqueado");
	}

	if(meQuedanVidas() == EXIT_SUCCESS){
		descontarUnaVida();
		desconectarmeDePlataforma(nivel);
		conectarAPlataforma(nivel);
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

void conectarAlOrquestador(){
	int32_t fd = cliente_crearSocketDeConexion(personaje->ipOrquestador, personaje->puertoOrquestador);
	if (fd > 0){
		log_info(logger,"Se conectó correctamente al Orquestador");
		fdOrquestador = fd;
	}
}

void levantarArchivoConfiguracion(){
	char * ruta =  string_new();
	string_append_with_format(&ruta,"%s%s",FIRST_PART_PATH,PATH_CONFIG);
	config = config_create(ruta);

		char *PATH_LOG = config_get_string_value(config, "PATH_LOG");
		logger = log_create(PATH_LOG, "PERSONAJE", true, LOG_LEVEL_INFO);

		//Variables para el Personaje
		char *nombre;
		char *simbolo;
		t_list *nombresNiveles;
		//t_list *niveles;
		t_list *listaRecursosPorNivel;
		////t_nivel *miNivel;
		int8_t cantVidas;
		char * ipOrquestador;
		int32_t puertoOrquestador;

		//Variables adicionales
		//char *planificacionNiveles;
		char **listaNiveles;
		char* orquestador;
		char * simString;

		//Obterngo del archivo de configuracion
		nombre = config_get_string_value(config, "NOMBRE");

		simString = config_get_string_value(config, "SIMBOLO");
		simbolo = simString;//[0];

		listaNiveles = config_get_array_value(config, "PLANIFICACIONNIVELES");
		cantVidas = config_get_int_value(config, "VIDAS");
		cantVidasDelArchivoDeConfiguracion = cantVidas;
		orquestador = config_get_string_value(config, "ORQUESTADOR");


		nombresNiveles = list_create();
		//niveles = list_create();

		int i=0;
		while(listaNiveles[i] != NULL){
			//Agrego el nombre del nivel en el orden de ejecucion
			list_add(nombresNiveles,listaNiveles[i]);

			listaRecursosPorNivel = list_create();

			char* objetivos_key = string_from_format("obj[%s]", listaNiveles[i]);
			char** objetivosArray = config_get_array_value(config, objetivos_key);
			int j=0;
			while(objetivosArray[j] != NULL){
				list_add(listaRecursosPorNivel,objetivosArray[j]);
				j++;
			}

			////miNivel = nivel_create(listaNiveles[i],listaRecursosPorNivel);
			////list_add(niveles,miNivel);

			//pongo los objetivos en algun lado
			free(objetivos_key);
			i++;
		}
		char ** infoOrquestador = string_split(orquestador,":");
		ipOrquestador = infoOrquestador[0];
		puertoOrquestador = atoi(infoOrquestador[1]);

		//creo el Personaje con todos sus parametros
		personaje = personaje_create(nombre,simbolo,cantVidas,nombresNiveles, listaRecursosPorNivel, ipOrquestador,puertoOrquestador);


		config_destroy(config);
}



static t_personaje *personaje_create(char *nombre,
										char *simbolo,
										int8_t cantVidas,
										t_list * niveles,
										t_list * recursosPorNivel,
										char * ipOrquestador,
										int16_t puertoOrquestador
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

int tengoTodosLosRecursos(int * nivel1){
	int nivel = 1; // TODO
	 t_list *listaRecursosNecesarios = list_get(personaje->recursosNecesariosPorNivel,nivel);
	 t_list *listaRecursosActuales = list_get(personaje->recursosActualesPorNivel,nivel);

	 if(list_size(listaRecursosNecesarios) == list_size(listaRecursosActuales)){
		 return EXIT_SUCCESS;
	 }else{
		 return EXIT_FAILURE;
	 }
 }

void recibirTurno(){

}

int tengoPosicionProximaCaja(int * nivel){
	return EXIT_SUCCESS;
}

void solicitarPosicionProximoRecurso(int * nivel){

}

void realizarMovimientoHaciaCajaRecursos(int * nivel){

}

void enviarNuevaPosicion(int * nivel){

}

int estoyEnCajaRecursos(int * nivel){
	return EXIT_SUCCESS;
}

void solicitarRecurso(int * nivel){

}

void avisarNivelConcluido(int * nivel){

}

void desconectarPlataforma(){

}

int meQuedanVidas(){
	return EXIT_SUCCESS;
}

void descontarUnaVida(){

}

void desconectarmeDePlataforma(int * nivel){

}

void conectarAPlataforma(int * nivel){

}
void interrumpirTodosPlanesDeNiveles(){

}
void finalizarTodoElProcesoPersonaje(){

}
