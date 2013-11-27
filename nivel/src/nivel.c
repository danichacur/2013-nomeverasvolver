
////////////////////////////////////////////////////HEADER////////////////////////////////////////////////////
#include "nivel.h"
#include "interbloqueo.h"
////////////////////////////////////////////////////ESPACIO DE DEFINICIONES////////////////////////////////////////////////////

#define RUTA "./config.cfg"

////////////////////////////////////////////////////ESPACIO DE VARIABLES GLOBALES////////////////////////////////////////////////////
int retardo;
int quantum;
int retardoSegundos;
int rows;
int cols; // TAMAÑO DEL MAPA
char * nombre;
char * buffer1;
char * direccionIPyPuerto;
char * algoritmo;
int enemigos;
bool nivelTerminado;
bool huboCambios;
int distancia;
long tiempoDeadlock;
int recovery;

t_list * listaPersonajesRecursos;
t_list * items;

t_config * config;
t_log * logger;
int32_t socketDeEscucha;

pthread_t hiloInotify;
pthread_mutex_t mutex_listas;
pthread_mutex_t mutex_mensajes;



pthread_mutex_t mx_lista_personajes;
pthread_mutex_t mx_lista_items;

bool listaRecursosVacia;
char * buffer_log;

int graficar;
bool crearLosEnemigos;
t_list * listaDeEnemigos;
useconds_t sleepEnemigos;
bool activarInterbloqueo;
int ingresoAlSistema;

////////////////////////////////////////////////////PROGRAMA PRINCIPAL////////////////////////////////////////////////////
int main (){
	nivelTerminado=false;
	huboCambios=false;

	listaDeEnemigos = list_create();
	items = list_create(); // CREO LA LISTA DE ITEMS
	listaPersonajesRecursos = list_create(); //CREO LA LISTA DE PERSONAJES CON SUS RECURSOS
	config = config_create(RUTA); //CREO LA RUTA PARA EL ARCHIVO DE CONFIGURACION
	pthread_mutex_init(&mutex_mensajes, NULL );
	pthread_mutex_init(&mutex_listas, NULL );


	pthread_mutex_init(&mutex_mensajes,NULL);
	pthread_mutex_init(&mx_lista_personajes,NULL);
	pthread_mutex_init(&mx_lista_items,NULL);

	leerArchivoConfiguracion(); //TAMBIEN CONFIGURA LA LISTA DE RECURSOS POR NIVEL
	dibujar();

	crearHiloInotify(hiloInotify);


	socketDeEscucha=handshakeConPlataforma();


	 //SE CREA UN SOCKET NIVEL-PLATAFORMA DONDE RECIBE LOS MENSAJES POSTERIORMENTE


	if (crearLosEnemigos){
		log_info(logger,"Se levantaron los enemigos");
		crearHilosEnemigos();
	}
	if(activarInterbloqueo){
		crearHiloInterbloqueo();
	}

	while(1){
		if(socketDeEscucha!=-1){
			mensajesConPlataforma(socketDeEscucha); //ACA ESCUCHO TODOS LOS MENSAJES EXCEPTO HANDSHAKE

		}else{
			log_info(logger, "Hubo un error al leer el socket y el programa finalizara su ejecucion");
			eliminarEstructuras();
			break;
		}

	}


	return true;
}

////////////////////////////////////////////////////ESPACIO DE FUNCIONES////////////////////////////////////////////////////
int leerArchivoConfiguracion(){
	//VOY A LEER EL ARCHIVO DE CONFIGURACION DE UN NIVEL//

	char *PATH_LOG = config_get_string_value(config, "PATH_LOG");
	graficar = config_get_int_value(config, "graficar");

	nombre= config_get_string_value(config, "Nombre");

	if(graficar){

		logger = log_create(PATH_LOG, "NIVEL", false, LOG_LEVEL_INFO);
		log_info(logger, "La config de graficado para el  %s es %d",nombre,graficar);

	}else{


		logger = log_create(PATH_LOG, "NIVEL", true, LOG_LEVEL_INFO);
		log_info(logger, "La config de graficado para el  %s es %d",nombre,graficar);
	}

	log_info(logger, "Voy a leer mi archivo de configuracion");

	log_info(logger, "Encontramos los atributos de %s",nombre);

	quantum = config_get_int_value(config, "quantum");
	log_info(logger, "El quantum para %s es de %d ut",nombre,quantum);

	recovery = config_get_int_value(config, "Recovery");
	log_info(logger, "El recovery para %s es de %d ut",nombre,recovery);

	enemigos = config_get_int_value(config, "Enemigos");
	log_info(logger, "La cantidad de enemigos de %s es %d",nombre,enemigos);

	tiempoDeadlock = config_get_long_value(config, "TiempoChequeoDeadlock");
	log_info(logger, "El tiempo de espera para ejecucion del hilo de deadlock para %s es de %d ut",nombre,tiempoDeadlock);

	sleepEnemigos = config_get_long_value(config, "Sleep_Enemigos");
	log_info(logger, "El tiempo de espera para mover los enemigos para %s es de %d ut",nombre,sleepEnemigos);

	algoritmo=config_get_string_value(config,"algoritmo");
	log_info(logger, "El %s se planificara con algoritmo %s",nombre,algoritmo);

	direccionIPyPuerto = config_get_string_value(config, "Plataforma");
	log_info(logger, "El %s tiene la platforma cuya direccion es %s",nombre,direccionIPyPuerto);




	retardo = config_get_int_value(config, "retardo");
	log_info(logger, "El retardo para el  %s es de %d milisegundos",nombre,retardo);

	distancia = config_get_int_value(config, "distancia");
	log_info(logger, "El remaining distance para el %s es de %d",nombre,distancia);

	crearLosEnemigos = config_get_int_value(config, "crearLosEnemigos");
	activarInterbloqueo = config_get_int_value(config, "activarInterbloqueo");


	// LEO LAS CAJAS DEL NIVEL //

	char litCaja[6]="Caja1\0";
	bool ret = config_has_property(config, litCaja);

	while (ret!=true){ //POR SI EMPIEZA EN UNA CAJA DISTINTA DE 1
		litCaja[4]++;
	}

	while (ret == true){
		char **caja = config_get_array_value(config, litCaja);
		crearCaja(caja);
		litCaja[4]++;
		log_info(logger, "El %s tiene %s cuyo simbolo es %s tiene %s instancias y su posicion x e y son %s %s",nombre,caja[0], caja[1],caja[2],caja[3],caja[4]);
		ret = config_has_property(config, litCaja);
	}


	return EXIT_SUCCESS;
}

void crearCaja(char ** caja){ //CREA LA UNIDAD CAJA Y LA ENGANCHA EN LA LISTA DE RECURSOS DEL NIVEL

	pthread_mutex_lock(&mutex_listas);
	CrearCaja(items, caja[1][0],atoi(caja[3]), atoi(caja[4]), atoi(caja[2]));
	pthread_mutex_unlock(&mutex_listas);

	log_info(logger, "Se crea la caja de %s", caja[1]);

}

int32_t handshakeConPlataforma(){ //SE CONECTA A PLATAFORMA Y PASA LOS VALORES INICIALES



	log_info(logger, "El %s se conectara a la plataforma en %s ",nombre,direccionIPyPuerto);
	char ** IPyPuerto = string_split(direccionIPyPuerto,":");
	char ** numeroNombreNivel = string_split(nombre,"l");
	int32_t numeroNivel=atoi(numeroNombreNivel[1]);
	char * IP=IPyPuerto[0];
	log_info(logger, "Este es el nivel %d",numeroNivel);

	char * buffer=malloc(sizeof(char)*50);
	int32_t puerto= atoi(IPyPuerto[1]);


	socketDeEscucha= cliente_crearSocketDeConexion(IP,puerto);
	if(socketDeEscucha==-1){
		kill(getpid(), SIGKILL); //PROCESO MUERTO POR TIMEOUT
	}




	sprintf(buffer,"%d,%s,%d,%d,%d",numeroNivel,algoritmo,quantum,retardo,distancia);

	pthread_mutex_lock(&mutex_mensajes);
	int32_t ok= enviarMensaje(socketDeEscucha, NIV_handshake_ORQ,buffer);
	pthread_mutex_unlock(&mutex_mensajes);


	enum tipo_paquete unMensaje = PER_handshake_ORQ;
	char* elMensaje=NULL;

	pthread_mutex_lock(&mutex_mensajes);
	recibirMensaje(socketDeEscucha,&unMensaje,&elMensaje);
	pthread_mutex_unlock(&mutex_mensajes);



	if(unMensaje==ORQ_handshake_NIV){

			if(ok==0){
				log_info(logger, "El %s envio correctamente handshake a plataforma en %s ",nombre,direccionIPyPuerto);
				return socketDeEscucha;

			}else{

				log_info(logger, "El %s no pudo enviar handshake a plataforma en %s, el nivel se cierra ",nombre,direccionIPyPuerto);
				kill(getpid(), SIGKILL);


			}

	}


	free(buffer);


	return socketDeEscucha;


}

void mensajesConPlataforma(int32_t socketEscucha) {//ATIENDE LA RECEPCION Y POSTERIOR RESPUESTA DE MENSAJES DEL ORQUESTADOR
	enum tipo_paquete unMensaje = PER_conexionNivel_ORQ;
	char* elMensaje=NULL;


	sleep(2);

	recibirMensaje(socketEscucha, &unMensaje,&elMensaje);




		switch (unMensaje) {

			case PLA_movimiento_NIV: {//graficar y actualizar la lista RECIBE "@,1,3"

				char ** mens = string_split(elMensaje,",");

				char idPers = mens[0][0];


				if (true){

				if(existePersonajeEnListaItems(idPers)){
					pthread_mutex_lock(&mutex_listas);
					MoverPersonaje(items, elMensaje[0],atoi(mens[1]), atoi(mens[2]));
					pthread_mutex_unlock(&mutex_listas);

					log_info(logger, "El personaje %s se movio a %s %s ",mens[0],mens[1],mens[2]);
					if(graficar)
						nivel_gui_dibujar(items,nombre);



				}
				pthread_mutex_lock(&mutex_mensajes);


				enviarMensaje(socketEscucha,NIV_movimiento_PLA,"0"); //"0" SI ES VALIDO
				pthread_mutex_unlock(&mutex_mensajes);

				} else {
					pthread_mutex_lock(&mutex_mensajes);
					enviarMensaje(socketEscucha,NIV_movimiento_PLA,"1");
					pthread_mutex_unlock(&mutex_mensajes);
					log_info(logger, "El personaje %s no se movio, movimiento invalido",mens[0]);//"1" SI ES INVALIDO

				}

				break;
			}
			case PLA_personajeMuerto_NIV:{ //RECIBE "@"
				char id=elMensaje[0];
				t_personaje_niv1 * personaje = malloc(sizeof(t_personaje_niv1));

				personaje = buscarPersonajeListaPersonajes(listaPersonajesRecursos,string_substring_until(elMensaje,1));

				pthread_mutex_lock(&mutex_listas);
				liberarRecursosDelPersonaje(personaje->recursosActuales); // tambien suma sus recursos a disponible
				BorrarItem(items,id);
				pthread_mutex_unlock(&mutex_listas);

				log_info(logger, "El personaje %c ha muerto ",id);
				if(graficar){
					nivel_gui_dibujar(items,nombre);

				}



				break;
			}
			case PLA_nuevoPersonaje_NIV:{ //RECIBE "@" LA POSICION DE INICIO SERA SIEMPRE 0,0 POR LO QUE NO LA RECIBE


				char * simbolo = malloc(strlen(elMensaje)+1);
				strcpy(simbolo,elMensaje);

				pthread_mutex_lock(&mutex_listas);
				CrearPersonaje(items,elMensaje[0],0,0);
				pthread_mutex_unlock(&mutex_listas);

				//ACA CREO UNA LISTA DE PERSONAJES CON SUS RESPECTIVOS RECURSOS ASIGNADOS
				t_personaje_niv1 * personaje = malloc(sizeof(t_personaje_niv1));
				personaje->simbolo = string_substring_until(elMensaje,1);
				personaje->recursosActuales = list_create();
				personaje->recursoBloqueante = string_new();
				personaje->posicion = posicion_create_pos(0,0);
				personaje->ingresoSistema = ingresoAlSistema;
				ingresoAlSistema++;

				pthread_mutex_lock(&mutex_listas);
				list_add(listaPersonajesRecursos,personaje);
				pthread_mutex_unlock(&mutex_listas);

				if(graficar)
					nivel_gui_dibujar(items,nombre);
				log_info(logger, "El nuevo personaje %c se dibujo en el mapa",elMensaje[0]);


				break;

			}
			case PLA_posCaja_NIV:{ // RECIBE "F" SI ESTA SOLICITANDO UNA FLOR, POR EJEMPLO


				char * pos = string_new();
				ITEM_NIVEL * caja = buscarRecursoEnLista(items, elMensaje);

				string_append(&pos, string_from_format("%d",caja->posx));
				string_append(&pos, ",");
				string_append(&pos, string_from_format("%d",caja->posy));

				pthread_mutex_lock(&mutex_mensajes);


				if(huboCambios==true){
					log_info(logger,"envio los cambios de configuracion");
					enviarMensaje(socketDeEscucha, NIV_cambiosConfiguracion_PLA,buffer1);
					huboCambios=false;

					char* elMensaje=NULL;

					log_info(logger,"me preparo para recibir el ok");

					recibirMensaje(socketEscucha, &unMensaje,&elMensaje);

					log_info(logger,"recibi el ok");


					if(unMensaje==OK1){

						log_info(logger,"enviare la posicion de la caja");
						enviarMensaje(socketEscucha, NIV_posCaja_PLA,pos); //"X,Y"

						log_info(logger,"envie la posicion de la caja");
					} else {
						log_info(logger,"recibi cualquiera");

					}
				} else {

						log_info(logger,"enviare la posicion de la caja");
						enviarMensaje(socketEscucha, NIV_posCaja_PLA,pos); //"X,Y"
						log_info(logger, "Envio posicion del recurso %s coordenadas %s ",elMensaje,pos);

					}





				pthread_mutex_unlock(&mutex_mensajes);


				free(pos);


				break;

			}
			case PLA_solicitudRecurso_NIV:{ // LE CONTESTO SI EL RECURSOS ESTA DISPONIBLE




				// El mensaje de ejemplo es : "@,H"

				char * rta;
				bool hayRecurso = determinarRecursoDisponible (string_substring_from(elMensaje, 2)); //SI ESTA EL RECURSO TMB LO RESTA
				t_personaje_niv1 * pers = buscarPersonajeListaPersonajes(listaPersonajesRecursos, string_substring_until(elMensaje,1));

				if (hayRecurso){
					rta = "0";

					pthread_mutex_lock(&mutex_listas);
					list_add(pers->recursosActuales, string_substring_from(elMensaje, 2));
					pers->recursoBloqueante = string_new();

					pthread_mutex_unlock(&mutex_listas);

				}else{

					pthread_mutex_lock(&mutex_listas);
					pers->recursoBloqueante = string_substring_from(elMensaje, 2);
					rta = "1";
					pthread_mutex_unlock(&mutex_listas);
				}


				pthread_mutex_lock(&mutex_mensajes);
				enviarMensaje(socketEscucha, NIV_recursoConcedido_PLA,rta);//"0" CONCEDIDO, "1" NO CONCEDIDO
				pthread_mutex_unlock(&mutex_mensajes);

				break;
			}

			case PLA_personajesDesbloqueados_NIV:{//"5,@,#,....." recorro lista personaje recursos y actualizo recBloqueante a vacio
				char ** mens = string_split(elMensaje,",");
				int i;
				log_info(logger, "Personajes %s desbloqueados",elMensaje);
				int cantPersonajes=atoi(mens[0]);
				for(i=1;i<=cantPersonajes;i++){
					char * unPersDesbloqueado=mens[i];

					t_personaje_niv1 * unPers=buscarPersonajeListaPersonajes(listaPersonajesRecursos,unPersDesbloqueado);

					pthread_mutex_lock(&mutex_listas);
					unPers->recursoBloqueante=string_new();
					pthread_mutex_unlock(&mutex_listas);

				}


				break;
			}

			case PLA_actualizarRecursos_NIV:{ //"3;F,1;C,3;....." actualizo la lista de recursos sumandole esas cantidades
				char ** mens = string_split(elMensaje,";");
				int cantRecursos= atoi(mens[0]);
				int i;
				log_info(logger, "Recursos desbloqueados: %s",elMensaje);
				for(i=1;i<=cantRecursos;i++){
					char** mensajeIndividual= string_split(mens[i],",");
					pthread_mutex_lock(&mutex_listas);
					int cantidad=atoi(mensajeIndividual[1]);
					sumarRecurso(items, mensajeIndividual[0][0],cantidad);
					pthread_mutex_unlock(&mutex_listas);

					if(graficar)
						nivel_gui_dibujar(items,nombre);
				}


				break;
			}
			case PLA_nivelFinalizado_NIV:{  //recibe personaje que termino el nivel ej: "@"
				char id=elMensaje[0];
				t_personaje_niv1 * personaje = malloc(sizeof(t_personaje_niv1));

				personaje = buscarPersonajeListaPersonajes(listaPersonajesRecursos,string_substring_until(elMensaje,1));

				pthread_mutex_lock(&mutex_listas);
				liberarRecursosDelPersonaje(personaje->recursosActuales);
				BorrarItem(items,id);
				borrarPersonajeListaPersonajes(listaPersonajesRecursos,elMensaje);
				pthread_mutex_unlock(&mutex_listas);

				log_info(logger, "El personaje %c ha terminado el nivel ",id);

				if(graficar){

					nivel_gui_dibujar(items,nombre);

				}

				if (list_size(listaPersonajesRecursos)==0){
					eliminarEstructuras();


				}


				break;
			}

			case PLA_perMuereInterbloqueo_NIV:{
				char id=elMensaje[0];
				t_personaje_niv1 * personaje = malloc(sizeof(t_personaje_niv1));

				personaje = buscarPersonajeListaPersonajes(listaPersonajesRecursos,string_substring_until(elMensaje,1));

				pthread_mutex_lock(&mutex_listas);
				liberarRecursosDelPersonaje(personaje->recursosActuales);
				BorrarItem(items,id);
				pthread_mutex_unlock(&mutex_listas);

				log_info(logger, "El personaje %c ha muerto por interbloqueo ",id);

				if(graficar)
					nivel_gui_dibujar(items,nombre);

				break;
			}

			{
				default:
				log_info(logger, "Recibi mensaje inexistente, la ejecucion del nivel finalizara");
				eliminarEstructuras();
				break;
			}


			free(elMensaje);

		}


}




void liberarRecursosDelPersonaje(t_list * recursos){

	while(list_size(recursos)!=0){
		char * unElemento=list_remove(recursos, 0);
		sumarRecurso(items, unElemento[0],1);
	}
}

bool determinarRecursoDisponible(char * recursoSolicitado){
	ITEM_NIVEL * item;

	item=buscarRecursoEnLista(items,recursoSolicitado);
	char id=item->id;
	int cantidad=item->quantity;
	if(cantidad==0) {
		return false;
		}else{
			restarRecurso(items,id);
			return true;
	}

}

ITEM_NIVEL * buscarRecursoEnLista(t_list * lista, char * simbolo){//BUSCA SI HAY UN RECURSO PEDIDO Y LO DEVUELVE
	ITEM_NIVEL * item;
	ITEM_NIVEL * unItem;
	bool encontrado = false;

	int i=0;
	while(i < list_size(lista) && !encontrado){

		unItem=list_get(lista,i);
		if(unItem->item_type==RECURSO_ITEM_TYPE)

			if(unItem->id==simbolo[0]){

				encontrado=true;
				item=unItem;
			}
		i++;
		}
	return item;
}

/*
ITEM_NIVEL * buscarPersonajeLista(t_list * lista, char * simbolo){ //BUSCA SI HAY UN PERSONAJE PEDIDO Y LO DEVUELVE
	ITEM_NIVEL * item;
	ITEM_NIVEL * unItem;
	bool encontrado = false;
	int i=0;

	while(i < list_size(lista) && !encontrado){
		unItem = list_get(lista,i);

		if(unItem->item_type == PERSONAJE_ITEM_TYPE)

			if (unItem->id == simbolo[0]){
				encontrado = true;
				item = unItem;
			}
		i++;
	}

	return item;
}

*/
void  borrarPersonajeListaPersonajes(t_list * lista, char * simbolo){


	int32_t _esta_el_personaje(t_personaje_niv1 * personaje){

		return string_equals_ignore_case(personaje->simbolo,simbolo);
	}

	t_personaje_niv1 * pers = list_remove_by_condition(lista, (void*) _esta_el_personaje);
	personaje_destroy(pers);


}

void personaje_destroy(t_personaje_niv1 * pers){
	free(pers->posicion);
	//list_destroy(pers->recursosActuales);
	//free(pers->simbolo);
	free(pers);
}

t_personaje_niv1 * buscarPersonajeListaPersonajes(t_list * lista, char * simbolo){


	int32_t _esta_el_personaje(t_personaje_niv1 * personaje){

		return string_equals_ignore_case(personaje->simbolo,simbolo);
	}

	t_personaje_niv1 * unPers = list_find(lista, (void*) _esta_el_personaje);



	return unPers;
}

void eliminarEstructuras() { //TERMINAR

	nivelTerminado=true;
	list_destroy(items);
	list_destroy(listaPersonajesRecursos);

	config_destroy(config);
	log_info(logger,"el nivel ha terminado satisfactoriamente");

	if(graficar){
		nivel_gui_terminar();

	}
	log_destroy(logger);

	kill(getpid(), SIGKILL);

}

void crearHiloInotify(pthread_t hiloNotify){

	pthread_create(&hiloNotify, NULL, (void*)&inotify, NULL);
}

 void * inotify(void) {



	 while (nivelTerminado==false) {
		 int file_descriptor = inotify_init(); //DESCRIPTOR DE ARCHIVO DE INOTIFY
		 char * buffer=malloc(sizeof(struct inotify_event)); //INICIALIZO BUFFER DONDE VOY A GUARDAR LAS MODIFICACIONES LEIDAS DEL FD

		 if (file_descriptor < 0) {

			 perror("inotify_init");

		 }

		 int watch_descriptor= inotify_add_watch(file_descriptor, "./config.cfg", IN_MODIFY); //CREAMOS MONITOR SOBRE EL PATH DONDE ESCUCHAREMOS LAS MODIFICACIONES

		 int length = read(file_descriptor, buffer, sizeof(struct inotify_event)); //CARGAMOS LAS MODIFICACIONES QUE ESCUCHAMOS EN EL BUFFER
		 log_info(logger, "Se cargó el buffer");

		 if (length < 0) {
			 log_info(logger, "Archivo invalido");

		 }

		 sleep(1);

		 struct inotify_event *event = (struct inotify_event *) &buffer[0]; //CARGAMOS LOS EVENTOS ESCUCHADOS EN UNA ESTRUCTURA

		 if (event->mask & IN_MODIFY) { //EL CAMPO MASK, SI NO ESTA VACIO, INDICA QUE EVENTO OCURRIO (SI SOLO ESCUCHAMOS EL 2, DEBERIAN OCURRIR SOLO DE TIPO 2)
			 config = config_create(RUTA);

			 int retardoAux = config_get_int_value(config, "retardo");
			 int quantumAux = config_get_int_value(config,"quantum");
			 char * algoritmoAux=config_get_string_value(config,"algoritmo");

			 if(retardoAux==retardo && quantumAux==quantum && strcmp(algoritmoAux,algoritmo)==0){

				 log_info(logger,"Hubo cambios en el archivo pero no sobre las variables en consideracion ");


			 } else {
				 log_info(logger, "Guardo los cambios para ser mandados en el mensaje de movimiento de personaje ");
				 pthread_mutex_lock(&mutex_mensajes);

				 retardo=retardoAux;
				 quantum=quantumAux;
				 strcpy(algoritmo,algoritmoAux);

				 free(buffer1);
				 buffer1=malloc(sizeof(char)*50);
				 sprintf(buffer1,"%s,%d,%d",algoritmoAux,quantumAux,retardoAux); // ejemplo "RR,5,5000"

				 huboCambios=true;

				 pthread_mutex_unlock(&mutex_mensajes);


			 }


		 } else {

			log_info(logger,"SI SOLO BUSCO MODIFY NO DEBERIA HABER LLEGADO UN MASK QUE NO SEA MODIFY!\n");

		 }


		 inotify_rm_watch(file_descriptor, watch_descriptor);
		 close(file_descriptor);
		 log_info(logger,"continuo un nuevo ciclo");
		 free(buffer);
	}

	 log_info(logger,"el nivel %s termino, finaliza el hilo de notificaciones",nombre);
	 return EXIT_SUCCESS;

 }

void crearHiloInterbloqueo(){
	 log_info(logger, "Creo el hilo deteccion de interbloqueo ");
	pthread_t id;
	pthread_create(&id, NULL, (void*)&rutinaInterbloqueo, NULL);
}

void crearHilosEnemigos(){
	int i;
	pthread_t pid;
	for(i = 0 ; i<enemigos ; i++){
		pthread_create(&pid, NULL, (void*)&enemigo, (int*) i );
	}
}







/////////// pruebaa //////////////////
void rnd(int *x, int max){
	*x += (rand() % 3) - 1;
	*x = (*x<max) ? *x : max-1;
	*x = (*x>0) ? *x : 1;
}

int dibujar (void) {
	int rows, cols;

		if(graficar){
			nivel_gui_inicializar();
			nivel_gui_get_area_nivel(&rows, &cols);
			nivel_gui_dibujar(items, nombre);
		}

		//nivel_gui_terminar();

	return EXIT_SUCCESS;
}


bool existePersonajeEnListaItems(char idPers){
	bool encontrado = false;
	int i = 0;
	while(i<list_size(items) && !encontrado){
		ITEM_NIVEL * elem = list_get(items,i);
		if (elem->item_type == PERSONAJE_ITEM_TYPE)
			if (strcmp(charToString(idPers), charToString(elem->id)) == 0){
				encontrado = true;
			}
		i++;
	}
	return encontrado;
}


