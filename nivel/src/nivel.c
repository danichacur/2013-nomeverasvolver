
////////////////////////////////////////////////////HEADER////////////////////////////////////////////////////
#include "nivel.h"

////////////////////////////////////////////////////ESPACIO DE DEFINICIONES////////////////////////////////////////////////////
#define DIRECCION "192.168.1.115"
#define PUERTO 5000

#define BUFF_SIZE 1024
#define RUTA "./config.cfg"
#define CANT_NIVELES_MAXIMA 100
#define BUF_LEN 1024

////////////////////////////////////////////////////ESPACIO DE VARIABLES GLOBALES////////////////////////////////////////////////////
int retardo;
int cantidadIntentosFallidos;
int quantum;
int retardoSegundos;
int rows;
int cols; // TAMAÑO DEL MAPA
char * nombre;
char * direccionIPyPuerto;
char * algoritmo;
t_list * listaRecursosNivel; //GLOBAL, PERSONAJES SABEN DONDE ESTAN SUS OBJETIVOS
t_list * listaCajas;
t_list * items;
t_list * listaDePersonajes;
t_config * config;
t_log * logger;
int32_t socketDeEscucha;
pthread_t hilo1;
pthread_mutex_t *mutex_mensajes;

t_list * listaPersonajesRecursos;

bool listaRecursosVacia;
char * buffer_log;


////////////////////////////////////////////////////PROGRAMA PRINCIPAL////////////////////////////////////////////////////
int main (){
	config = config_create(RUTA); //CREO LA RUTA PARA EL ARCHIVO DE CONFIGURACION
	listaRecursosNivel=list_create(); //CREO LA LISTA DE RECURSOS POR NIVEL
	items = list_create(); // CREO LA LISTA DE ITEMS
	listaPersonajesRecursos = list_create(); //CREO LA LISTA DE PERSONAJES CON SUS RECURSOS
	listaDePersonajes = list_create(); // CREO LA LISTA DE PERSONAJES (NO VA)
	leerArchivoConfiguracion(); //TAMBIEN CONFIGURA LA LISTA DE RECURSOS POR NIVEL
	//crearHiloInotify();
	inicializarMapaNivel(listaRecursosNivel);
	socketDeEscucha=handshakeConPlataforma(); //SE CREA UN SOCKET NIVEL-PLATAFORMA DONDE RECIBE LOS MENSAJES POSTERIORMENTE
	while(1){
		if(socketDeEscucha!=-1){
		mensajesConPlataforma(socketDeEscucha); //ACA ESCUCHO TODOS LOS MENSAJES EXCEPTO HANDSHAKE
		}else{
			log_info(logger, "Hubo un error al leer el socket y el programa finalizara su ejecucion");
			break;
		}

	}

	eliminarEstructuras();
	return true;
}

////////////////////////////////////////////////////ESPACIO DE FUNCIONES////////////////////////////////////////////////////
int leerArchivoConfiguracion(){
	//VOY A LEER EL ARCHIVO DE CONFIGURACION DE UN NIVEL//
	char *PATH_LOG = config_get_string_value(config, "PATH_LOG");
	logger = log_create(PATH_LOG, "NIVEL", true, LOG_LEVEL_INFO); //CREO EL ARCHIVO DE LOG
	log_info(logger, "Voy a leer mi archivo de configuracion");


	nombre= config_get_string_value(config, "Nombre");
	log_info(logger, "Encontramos los atributos de %s",nombre);

	quantum = config_get_int_value(config, "quantum");
	log_info(logger, "El quantum para %s es de %d ut",nombre,quantum);

	int recovery = config_get_int_value(config, "Recovery");
	log_info(logger, "El recovery para %s es de %d ut",nombre,recovery);

	int enemigos = config_get_int_value(config, "Enemigos");
	log_info(logger, "La cantidad de enemigos de %s es %d",nombre,enemigos);

	long tiempoDeadlock = config_get_long_value(config, "TiempoChequeoDeadlock");
	log_info(logger, "El tiempo de espera para ejecucion del hilo de deadlock para %s es de %d ut",nombre,tiempoDeadlock);

	long sleepEnemigos = config_get_long_value(config, "Sleep_Enemigos");
	log_info(logger, "El tiempo de espera para mover los enemigos para %s es de %d ut",nombre,sleepEnemigos);

	algoritmo=config_get_string_value(config,"algoritmo");
	log_info(logger, "El %s se planificara con algoritmo %s",nombre,algoritmo);

	direccionIPyPuerto = config_get_string_value(config, "Plataforma");
	log_info(logger, "El %s tiene la platforma cuya direccion es %s",nombre,direccionIPyPuerto);


	retardo = config_get_int_value(config, "retardo");
	log_info(logger, "El retardo para el  %s es de %d milisegundos",nombre,retardoSegundos);
	//int retardo=retardoSegundos*1000;


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

	tRecursosNivel *unaCaja=malloc(sizeof(tRecursosNivel));
	unaCaja->nombre=caja[0];
	unaCaja->simbolo=caja[1];
	unaCaja->instancias=caja[2];
	unaCaja->posX=caja[3];
	unaCaja->posY=caja[4];

	int cantElementos= list_add(listaRecursosNivel,unaCaja);
	log_info(logger, "La cantidad de cajas del %s es ahora %d",nombre,cantElementos+1);
	free(unaCaja);
}

void inicializarMapaNivel(t_list* listaRecursos){

    t_list* items = list_create(); //LISTA DE ITEMS DEL MAPA (CAJAS PERSONAJES Y ENEMIGOS)
	int rows, cols; // TAMAÑO DEL MAPA
	int i=0;

	int ok=nivel_gui_inicializar();
		if(ok!=0){

			log_info(logger, "El mapa de %s no ha podido dibujarse",nombre);

		}else{

			log_info(logger, "El mapa de %s se ha dibujado",nombre);
		}

	nivel_gui_get_area_nivel(&rows, &cols);
	log_info(logger, "El mapa de %s se ha dibujado, tiene %s filas por %s columnas",nombre,rows,cols);

	tRecursosNivel *unaCaja= list_get(listaRecursos, i);

	while(unaCaja!=NULL){
		char* simbolo=unaCaja->simbolo;
		char* instancias=unaCaja->instancias;
		char* posX=unaCaja->posX;
		char* posY=unaCaja->posY;
		int posXint=atoi(posX);
		int posYint=atoi(posY);
		int instanciasInt=atoi(instancias);

		CrearCaja(items,*simbolo, posXint, posYint, instanciasInt);
		i++;
	}
	log_info(logger, "Se procede a graficar los elementos en el mapa creado",nombre,rows,cols);
	nivel_gui_dibujar(items,nombre);

}

int32_t handshakeConPlataforma(){ //SE CONECTA A PLATAFORMA Y PASA LOS VALORES INICIALES
//	pthread_mutex_lock(mutex_mensajes);

	log_info(logger, "El %s se conectara a la plataforma en %s ",nombre,direccionIPyPuerto);
	char ** IPyPuerto = string_split(direccionIPyPuerto,":");
	char ** numeroNombreNivel = string_split(nombre,"l");
	int32_t numeroNivel=atoi(numeroNombreNivel[1]);
	char * IP=IPyPuerto[0];

	printf("numero nivel %d \n",numeroNivel);




	char * buffer=malloc(sizeof(char*));
	int32_t puerto= atoi(IPyPuerto[1]);


	socketDeEscucha= cliente_crearSocketDeConexion(IP,puerto);
	sprintf(buffer,"%d,%s,%d,%d",numeroNivel,algoritmo,quantum,retardo);
	int32_t ok= enviarMensaje(socketDeEscucha, NIV_handshake_ORQ,buffer);

	if(socketDeEscucha>-1){
	log_info(logger, "El %s se conecto a la plataforma en %s ",nombre,direccionIPyPuerto);
		if(ok==0){
			log_info(logger, "El %s envio correctamente handshake a plataforma en %s ",nombre,direccionIPyPuerto);
			char buffer[1024];
			sprintf(buffer,"%d,%d,%s",quantum,retardo,algoritmo);
			int32_t respuesta=enviarMensaje(socketDeEscucha,NIV_cambiosConfiguracion_PLA,buffer);
			printf("%d",respuesta);

			log_info(logger, "Se envio a la plataforma quantum %d retardo %d y algoritmo %s ",quantum,retardo,algoritmo);


			return socketDeEscucha;
		}
	} else {

		log_info(logger, "El %s no pudo conectarse a la plataforma, se termina la ejecucion en %s ",nombre,direccionIPyPuerto);

		}
	free(buffer);
	return socketDeEscucha;

//	pthread_mutex_unlock(mutex_mensajes);
}

void mensajesConPlataforma(int32_t socketEscucha) {//ATIENDE LA RECEPCION Y POSTERIOR RESPUESTA DE MENSAJES DEL ORQUESTADOR
	enum tipo_paquete unMensaje;
	char* elMensaje=NULL;

	recibirMensaje(socketEscucha, &unMensaje,&elMensaje);

		switch (unMensaje) {

			case PLA_movimiento_NIV: {//graficar y actualizar la lista RECIBE "@,1,3"
				char ** mens = string_split(elMensaje,",");
				bool movValido;

				ITEM_NIVEL * pers = buscarPersonajeLista(items, mens[0]);
				movValido= validarMovimientoPersonaje(mens,pers);

				if (movValido==true){

				pers->posx = atoi(mens[1]);
				pers->posy = atoi(mens[2]);

				MoverPersonaje(items, elMensaje[0],pers->posx ,pers->posy); //hay q validar q se mueva dentro del mapa...

				printf("el personaje se movio %s",elMensaje);

				log_info(logger, "El personaje %s se movio a %d %d ",mens[0],pers->posx,pers->posy);

				int32_t respuesta=enviarMensaje(socketEscucha,NIV_movimiento_PLA,"0"); //"0" SI ES VALIDO
				printf("%d \n",respuesta);

				} else {

					enviarMensaje(socketEscucha,NIV_movimiento_PLA,"1");
					log_info(logger, "El personaje %s no se movio, movimiento invalido",mens[0]);//"1" SI ES INVALIDO

				}

				break;
			}
			case PLA_personajeMuerto_NIV:{ //RECIBE "@"
				char id=elMensaje[0];

				BorrarItem(items,id);
				log_info(logger, "El personaje %s ha muerto ",elMensaje[0]);

				break;
			}
			case PLA_nuevoPersonaje_NIV:{ //RECIBE "@" LA POSICION DE INICIO SERA SIEMPRE 0,0 POR LO QUE NO LA RECIBE
				char * simbolo = malloc(strlen(elMensaje)+1);
				ITEM_NIVEL * item = malloc(sizeof(ITEM_NIVEL));

				strcpy(simbolo,elMensaje);

				CrearPersonaje(items,elMensaje[0],0,0);

				//ACA CREO UNA LISTA DE PERSONAJES CON SUS RESPECTIVOS RECURSOS ASIGNADOS
				t_personaje * personaje = malloc(sizeof(t_personaje));
				personaje->simbolo = string_substring_until(elMensaje,1);
				personaje->recursosActuales = list_create();
				personaje->simbolo = string_new();

				free(item);
				free(simbolo);

				log_info(logger, "El nuevo personaje %s se dibujo en el mapa",simbolo);

				break;

			}
			case PLA_posCaja_NIV:{ // RECIBE "F" SI ESTA SOLICITANDO UNA FLOR, POR EJEMPLO
				char * pos = string_new();
				ITEM_NIVEL * caja = buscarRecursoEnLista(items, elMensaje);

				string_append(&pos, string_from_format("%d",caja->posx));
				string_append(&pos, ",");
				string_append(&pos, string_from_format("%d",caja->posy));

				int32_t mensaje= enviarMensaje(socketEscucha, NIV_posCaja_PLA,pos); //"X,Y"

				if(mensaje){
					log_info(logger, "Envio posicion del recurso %s coordenadas %s ",elMensaje,pos);

				} else {
					log_info(logger, "El envio de posicion del recurso ha fallado ");

				}
				free(pos);
				break;

			}case PLA_solicitudRecurso_NIV:{ // LE CONTESTO SI EL RECURSOS ESTA DISPONIBLE
				// El mensaje de ejemplo es : "@,H"

				char * rta;
				bool hayRecurso = determinarRecursoDisponible (string_substring_from(elMensaje, 2));
				t_personaje * pers = buscarPersonajeListaPersonajes(listaPersonajesRecursos, string_substring_until(elMensaje,1));

				if (hayRecurso){
					rta = "0";
					list_add(pers->recursosActuales, string_substring_from(elMensaje, 2));
					pers->recursoBloqueante = string_new();

				}else{
					pers->recursoBloqueante = string_substring_from(elMensaje, 2);
					rta = "1";
					restarRecurso(items,elMensaje[1]);
				}

				enviarMensaje(socketEscucha, NIV_recursoConcedido_PLA,rta);//"0" CONCEDIDO, "1" NO CONCEDIDO

				break;
			}
			{
				default:
				printf("%s \n","recibio cualquier cosa");
				break;
			}

		nivel_gui_dibujar(items,nombre);
		free(elMensaje);

		}
}


bool validarMovimientoPersonaje(char ** mensaje,ITEM_NIVEL * personaje){ //TERMINAR
	if(personaje->posx <= atoi(mensaje[1]) && personaje->posy <= atoi(mensaje[2])){
		log_info(logger, "El personaje %s realizo un movimiento valido y se dibujara en el mapa",personaje->id);
		return true;
	}else{
		log_info(logger, "El personaje %s realizo un movimiento invalido y no se dibujara en el mapa",personaje->id);
		return false;
	}

}

bool determinarRecursoDisponible(char * recursoSolicitado){
	ITEM_NIVEL * item;

	item=buscarRecursoEnLista(items,recursoSolicitado);
	char id=item->id;
	char * nulo="0";
	if(id== nulo[0]) {
		return false;
		}else{
			return true;
	}

}

ITEM_NIVEL * buscarRecursoEnLista(t_list * lista, char * simbolo){//BUSCA SI HAY UN RECURSO PEDIDO Y LO DEVUELVE
	ITEM_NIVEL * item;
	ITEM_NIVEL * unItem;                      //QUE PASA SI EL RECURSO NO EXISTE?? QUE DEVOLVERIA ITEM?
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

t_personaje * buscarPersonajeListaPersonajes(t_list * lista, char * simbolo){ //NO SIRVE, BORRAR
	t_personaje * personaje;
	t_personaje * unPers;

	bool encontrado = false;
	int i=0;
	while(i < list_size(lista) && !encontrado){
		unPers = list_get(lista,i);
		if (strcmp(unPers->simbolo,simbolo) == 0){
			encontrado = true;
			personaje = unPers;
		}
		i++;
	}

	return personaje;
}

void eliminarEstructuras() { //TERMINAR

	config_destroy(config);

}


// FUNCIONES DEL TP MIO CUATRI PASADO Y DEL TP DE PABLO, ADAPTAR Y CORREGIR
int inotify(void) {
        char buffer[BUF_LEN];
        int quantumAux;
        int retardoAux;
        char * algoritmoAux;
        int ret;
        int i;
        int file_descriptor = inotify_init(); //DESCRIPTOR DE ARCHIVO DE INOTIFY

        if (file_descriptor < 0) {
                perror("inotify_init");
        }

        // Creamos un monitor sobre un path indicando que eventos queremos escuchar
        ///home/utnso/workspace/inotify/src
        int watch_descriptor = inotify_add_watch(file_descriptor, "./", IN_MODIFY | IN_CREATE | IN_DELETE);

        // El file descriptor creado por inotify, es el que recibe la información sobre los eventos ocurridos
        // para leer esta información el descriptor se lee como si fuera un archivo comun y corriente pero
        // la diferencia esta en que lo que leemos no es el contenido de un archivo sino la información
        // referente a los eventos ocurridos


        i = 0;
        while (1) {
                int length = read(file_descriptor, buffer, BUF_LEN);
                printf("BUFFER = [%s]\n",buffer);
                if (length < 0) {
                        perror("read");
                }
        printf("DESPUES DE LEER\n");

        int offset = 0;

        // Luego del read buffer es un array de n posiciones donde cada posición contiene
        // un eventos ( inotify_event ) junto con el nombre de este.
        while (offset < length) {

                // El buffer es de tipo array de char, o array de bytes. Esto es porque como los
                // nombres pueden tener nombres mas cortos que 24 caracteres el tamaño va a ser menor
                // a sizeof( struct inotify_event ) + 24.
                struct inotify_event *event = (struct inotify_event *) &buffer[offset];

                // El campo "len" nos indica la longitud del tamaño del nombre
                if (event->len) {
                        // Dentro de "mask" tenemos el evento que ocurrio y sobre donde ocurrio
                        // sea un archivo o un directorio
                        if (event->mask & IN_CREATE) {
                                if (event->mask & IN_ISDIR) {
                                        printf("The directory %s was created.\n", event->name);
                                } else {
                                        printf("The file %s was created.\n", event->name);
                                }
                        } else if (event->mask & IN_DELETE) {
                                if (event->mask & IN_ISDIR) {
                                        printf("The directory %s was deleted.\n", event->name);
                                } else {
                                        printf("The file %s was deleted.\n", event->name);
                                }
                        } else if (event->mask & IN_MODIFY) {
                                if (event->mask & IN_ISDIR) {
                                        printf("The directory [%s] was modified.\n", event->name);

                                } else {
                                        printf("The file [%s] was modified.\n", event->name);
                                        ret = strcmp(event->name,RUTA);
                                        if (ret == 0) {
                                                if (i == 0){
                                                        sleep(1);
                                                        config_destroy(config);
                                                        config = config_create(RUTA);
                                                        ret = config_keys_amount(config);
                                                        retardoAux = config_get_int_value(config, "retardo");
                                                        quantumAux = config_get_int_value(config,"quantum");
                                                        algoritmoAux=config_get_string_value(config,"algoritmo");
                                                        printf("El nuevo quantum es: %d\n",quantum);
                                                        printf("El nuevo retardo es: %d\n",retardoAux);
                                                        //pthread_mutex_lock( &mutex3 );
                                                        quantum=quantumAux;
                                                        retardo=retardoAux;
                                                        algoritmo=algoritmoAux;
                                                    	sprintf(buffer,"%s,%d,%d",algoritmo,quantum,retardo);
                                                    	enviarMensaje(socketDeEscucha, NIV_cambiosConfiguracion_PLA,buffer);

                                                        //pthread_mutex_unlock( &mutex3 );
                                                        i ++;
                                                }
                                                else i = 0;
                                        }
                                }
                        }
                }
                else {
                        printf("NO DEBERIAS LLEGAR ACA!\n");
                        printf("event->len = %d",event->len);
                }
                offset += sizeof (struct inotify_event) + event->len;
        }
        }

        inotify_rm_watch(file_descriptor, watch_descriptor);
        close(file_descriptor);

        printf("SALGO DEL PROGRAMA\n");

        return EXIT_SUCCESS;
}

















