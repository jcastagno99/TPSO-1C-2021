#include "discordiador.h"
int main(void)
{
	signal(SIGINT, catch_sigint_signal);
	//inicializacion del modulo
	inicializar_discordiador();
	// ORDENAR LA destruccion de los logs y el config
	// terminar_programa(conexion_mi_ram_hq, conexion_i_mongo_store, logger, config);
}

void cargar_config()
{
	// Grado de multiprocesamiento
	char *multitask = config_get_string_value(config, "GRADO_MULTITAREA");
	grado_de_multiprocesamiento = atoi(multitask);
	// Conexiones
	ip_mi_ram_hq = config_get_string_value(config, "IP_MI_RAM_HQ");
	puerto_mi_ram_hq = config_get_string_value(config, "PUERTO_MI_RAM_HQ");
	ip_i_mongo_store = config_get_string_value(config, "IP_I_MONGO_STORE");
	puerto_i_mongo_store = config_get_string_value(config, "PUERTO_I_MONGO_STORE");
	// Algoritmo
	algoritmo = config_get_string_value(config, "ALGORITMO");
	// RR
	char *q = config_get_string_value(config, "QUANTUM");
	quantum_rr = atoi(q);
	// RETARDO
	char *retardo = config_get_string_value(config, "RETARDO_CICLO_CPU");
	tiempo_retardo_ciclo_cpu = atoi(retardo);
	// Puerto de escucha para los sabotajes
	char *puerto_escucha_sabotaje = config_get_string_value(config, "PUERTO_ESCUCHA_SABOTAJE");
	puerto_escucha = atoi(puerto_escucha_sabotaje);
	// Sabotaje
	char *aux_sabotaje = config_get_string_value(config, "DURACION_SABOTAJE");
	duracion_sabotaje = atoi(aux_sabotaje);
}

void inicializar_discordiador()
{
	system("rm cfg/discordiador.log");
	logger = log_create("cfg/discordiador.log", "discordiador.log", 1, LOG_LEVEL_INFO);
	config = config_create("cfg/discordiador.config");

	cargar_config();
	// [TODO] Planificacion
	hilos_pausables = list_create();
	sem_init(&sem_planificador_a_largo_plazo,0,0);
	sem_init(&sem_hilo_de_bloqueados,0,0);
	list_add(hilos_pausables,&sem_planificador_a_largo_plazo);
	list_add(hilos_pausables,&sem_hilo_de_bloqueados);

	// [TODO] inicializar colas/listas y semaforos
	cola_de_new = list_create();
	cola_de_ready = list_create();
	cola_de_exec = list_create();
	lista_de_patotas = list_create();
	list_total_tripulantes = list_create();
	cola_de_iniciar_patotas = queue_create();
	cola_de_block = list_create();

	pthread_mutex_init(&mutex_cola_iniciar_patota, NULL);
	pthread_mutex_init(&mutex_cola_de_ready, NULL);
	pthread_mutex_init(&mutex_cola_de_new, NULL);
	pthread_mutex_init(&mutex_cola_de_exec, NULL);
	pthread_mutex_init(&mutex_cola_de_block, NULL);
	pthread_mutex_init(&mutex_tarea, NULL);

	sem_init(&sem_contador_cola_iniciar_patota, 0, 0);
	sem_init(&sem_contador_cola_de_new, 0, 0);
	sem_init(&sem_contador_cola_de_block, 0, 0);
	sem_init(&sem_contador_cola_de_ready, 0, 0);
	sem_init(&operacion_entrada_salida_finalizada, 0, 0);
	sem_init(&sem_sabotaje, 0, 0);
	sem_init(&sem_sabotaje_fin, 0, 0);

	// [TODO] Inicializacion de VAR GLOBALES
	id_patota = 1;
	id_tcb = 1;
	planificacion_pausada = true;
	sabotaje = false;

	printf("Multiprocesamiento %i\n", grado_de_multiprocesamiento);

	pthread_t hilo_server;
	pthread_t thread;
	pthread_attr_t atributo_hilo;
	pthread_attr_init(&atributo_hilo);
	pthread_attr_setdetachstate(&atributo_hilo, PTHREAD_CREATE_DETACHED);
	pthread_create(&hilo_server, NULL, esperar_conexiones, NULL);

	// HILO CREACION PATOTAS ---------------------------------------------
	pthread_create(&thread, &atributo_hilo, gestion_patotas_a_memoria, (void *)logger);

	// HILO CONSOLA ------------------------------------------------------
	pthread_t hilo_consola;
	pthread_create(&hilo_consola, NULL, (void *)leer_consola, (void *)logger);

	// Hilo ejecutando planificador a largo plazo  -------------------------
	pthread_create(&thread, &atributo_hilo, (void *)planificador_a_largo_plazo, NULL);

	// Hilo ejecutando la Cola de Bloqueado  -------------------------
	pthread_create(&thread, &atributo_hilo, (void *)ejecutar_tripulantes_bloqueados, NULL);

	// Hilo ejecutando la Cola de Exec  -------------------------
	if (string_equals_ignore_case(algoritmo, "RR"))
	{
		for (int i = 0; i < grado_de_multiprocesamiento; i++)
		{
			int *temp = (int *)malloc(sizeof(int));
			*temp = i;
			pthread_create(&thread, &atributo_hilo, (void *)procesar_tripulante_rr, (void *)temp);
		}
		log_info(logger, "[ Discordiador ] Algoritmo: %s. Quantum: %i. Grado de multiprocesamiento: %i.", algoritmo, quantum_rr, grado_de_multiprocesamiento);
	}
	else
	{
		for (int i = 0; i < grado_de_multiprocesamiento; i++)
		{
			int *temp = (int *)malloc(sizeof(int));
			*temp = i;
			pthread_create(&thread, &atributo_hilo, (void *)procesar_tripulante_fifo, (void *)temp);
		}
		log_info(logger, "[ Discordiador ] Algoritmo: %s. Grado de multiprocesamiento: %i", algoritmo, grado_de_multiprocesamiento);
	}
	pthread_join(hilo_consola, NULL);
}

// SERVIDOR QUE ESPERA UN SABOTAJE --------------
void *esperar_conexiones()
{
	int socket_escucha = iniciar_servidor_discordiador(puerto_escucha);
	int socket_cliente;
	log_info(logger, "[ Discordiador ] SERVIDOR LEVANTADO! ESCUCHANDO EN %i", puerto_escucha);
	struct sockaddr cliente;
	socklen_t len = sizeof(cliente);
	do
	{
		socket_cliente = accept(socket_escucha, &cliente, &len);
		if (socket_cliente > 0)
		{
			log_info(logger, "[ Discordiador ] NUEVA CONEXIÓN!");
			crear_hilo_para_manejar_suscripciones(socket_cliente);
		}
		else
		{
			log_error(logger, "[ Discordiador ] ERROR ACEPTANDO CONEXIONES: %s", strerror(errno));
		}
	} while (1);
}

int iniciar_servidor_discordiador(int puerto)
{
	int socket_v;
	int val = 1;
	struct sockaddr_in servaddr;

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(puerto);

	socket_v = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_v < 0)
	{
		char *error = strerror(errno);
		perror(error);
		free(error);
		return EXIT_FAILURE;
	}

	setsockopt(socket_v, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	if (bind(socket_v, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		return EXIT_FAILURE;
	}
	if (listen(socket_v, MAX_CLIENTS) < 0)
	{
		return EXIT_FAILURE;
	}

	return socket_v;
}

void crear_hilo_para_manejar_suscripciones(int socket)
{
	int *socket_hilo = malloc(sizeof(int));
	*socket_hilo = socket;
	pthread_t hilo_conectado;
	pthread_attr_t atributo_hilo;
	pthread_attr_init(&atributo_hilo);
	pthread_attr_setdetachstate(&atributo_hilo, PTHREAD_CREATE_DETACHED);
	pthread_create(&hilo_conectado, &atributo_hilo, (void *)manejar_suscripciones_discordiador, socket_hilo);
}

void manejar_suscripciones_discordiador(int *socket_hilo)
{
	int fd = *socket_hilo;
	free(socket_hilo);
	log_info(logger, "[ Discordiador ] Atendiendo sabotaje. Socket fd %i", fd);
	t_paquete *paquete = recibir_paquete(fd);
	
	posicion pos = deserializar_posicion(paquete->stream);
	printf("\033[1;32mPosiciones (%d,%d)\033[0m\n",pos.pos_x,pos.pos_y);
	pos_sab_x = pos.pos_x;
	pos_sab_y = pos.pos_y;

	liberar_paquete(paquete);
	manejar_sabotaje(fd);
	pthread_exit(NULL);
	//close(fd); esto rompia XD era doble
}

void manejar_sabotaje(int conexion_imongo)
{
	int cantidad_en_ejecucion = 0;
	log_info(logger, "[ Discordiador ] Sabotaje en la ubicación (%i,%i)", pos_sab_x, pos_sab_y);
	t_list *cola_sabotaje = list_create();

	// 1- Pausar planificacion. Se detienen todos los hilos involucrados, incluido I/O. (nueva version)

	sabotaje = true;
	log_warning(logger, "[ Sabotaje ] Iniciando...");
	pausar_planificacion();
	// sem_wait(semafor_sabotaje) *4
	log_warning(logger, "[ Sabotaje ] Planificacion Pausada");
	void* stream_respuesta = serializar_respuesta_ok_fail(RESPUESTA_OK);
	enviar_paquete(conexion_imongo,RESPUESTA_INICIAR_SABOTAJE,sizeof(respuesta_ok_fail),stream_respuesta);
	close(conexion_imongo);
	sleep(tiempo_retardo_ciclo_cpu + 4); // [TODO] se borraria

	// 2- Ordenar la cola de exec, agregar sus tripulantes a la lista de sabotaje y vaciar la cola. Lo mismo con la cola de Ready
	list_sort(cola_de_exec, (void *)ordenar_por_tid);
	list_sort(cola_de_ready, (void *)ordenar_por_tid);
	list_add_all(cola_sabotaje, cola_de_exec);
	list_add_all(cola_sabotaje, cola_de_ready);
	log_warning(logger, "[ Sabotaje ] Cola de sabotaje con %i elementos", list_size(cola_sabotaje));
	cantidad_en_ejecucion = list_size(cola_de_exec);
	vaciar_lista(cola_de_exec);
	vaciar_lista(cola_de_ready);
	log_warning(logger, "[ Sabotaje ] Colas de Exec y Ready vacias. %i %i", list_size(cola_de_ready), list_size(cola_de_exec));

	// 3- Marcar a todos con el estado BLOCKED_SABOTAJE
	printf("Sabotaje\n");
	list_iterate(cola_sabotaje, (void *)bloqueo_por_sabotaje);

	//list_iterate(cola_sabotaje, (void *)iterate_distancias);

	// 4- De la cola_sabotaje, tomar el mínimo.
	dis_tripulante *tripulante = list_get_minimum(cola_sabotaje, (void *)minimum);
	log_warning(logger, "[ Sabotaje ] Tripulante %i elegido", tripulante->id);

	// 5- Setear la tarea de Sabotaje
	dis_tarea *tarea_sabotaje = malloc(sizeof(dis_tarea));
	tarea_sabotaje->nombre_tarea = "SABOTAJE";
	tarea_sabotaje->pos_x = pos_sab_x;
	tarea_sabotaje->pos_y = pos_sab_y;
	tarea_sabotaje->tiempo_restante = duracion_sabotaje;
	tarea_sabotaje->tiempo_total = duracion_sabotaje;
	tarea_sabotaje->estado_tarea = EN_CURSO;
	tarea_sabotaje->parametro=0;

	tripulante->tarea_actual = tarea_sabotaje;

	// 7- Procesar tripulante hasta terminar la tarea
	while (tripulante->tarea_actual->estado_tarea != TERMINADA)
	{
		sem_post(&(tripulante->sem_tri));
		sem_wait(&(tripulante->procesador));
	}
	sem_post(&sem_sabotaje);
	sem_wait(&sem_sabotaje_fin);
	sabotaje = false;
	/* 
		Este par de semaforos cruzados es porque queremos asegurarnos de que no haya condicion de carrera
		al leer el estado de la tarea y al leer el estado del flag de sabotaje, ya que van a haber 2 hilos (tripulante y hilo del sabotaje)
		leyendo y escribiendo sobre esas variables.
	*/

	//Enviando mensaje a imongo
	uint32_t n = tripulante->id;
	void *stream_tid = pserializar_tid(n);
	int socket_respuesta = crear_conexion(ip_i_mongo_store, puerto_i_mongo_store);
	enviar_paquete(socket_respuesta, REGISTRAR_SABOTAJE_RESUELTO, sizeof(uint32_t), stream_tid);
	close(socket_respuesta);
	// 7- Mandar a todos a ready en el orden de la lista esa local de sabotaje
	list_iterate(cola_sabotaje, (void *)enviar_a_ready);

	list_destroy(cola_sabotaje);

	// 8- Reiniciar la planificacion
	log_warning(logger, "[ Sabotaje ] Fin de sabotaje");
	iniciar_planificacion();
	for (int i = 0; i < cantidad_en_ejecucion; i++)
	{
		sem_post(&sem_contador_cola_de_ready);
	}
	log_warning(logger, "[ Sabotaje ] Ya iniciamos la planificacion!!!");
	free(tarea_sabotaje);
}

int calcular_distancia(int pos_x1, int pos_y1, int pos_x2, int pos_y2)
{
	int dis_x = fabs(pos_x1 - pos_x2);
	int dis_y = fabs(pos_y1 - pos_y2);
	return dis_x + dis_y;
}

dis_tripulante *minimum(dis_tripulante *a, dis_tripulante *b)
{
	int dis_sabotaje_a = calcular_distancia(a->pos_x, a->pos_y, pos_sab_x, pos_sab_y);
	int dis_sabotaje_b = calcular_distancia(b->pos_x, b->pos_y, pos_sab_x, pos_sab_y);
	if (dis_sabotaje_a < dis_sabotaje_b)
		return a;
	else {
		if (dis_sabotaje_a == dis_sabotaje_b){	
			if(a->id < b->id)
				return a;
			else
				return b;
		}
		else
			return b;
	}
}

bool ordenar_por_tid(dis_tripulante *t1, dis_tripulante *t2)
{
	return t1->id < t2->id;
}

void bloqueo_por_sabotaje(dis_tripulante *t)
{	
	//printf("Trip %d | ",tripulante->id);
	t->estado = BLOCKED_SABOTAJE;// [ISSUE] COMENTAR YA TODOS LOS QUE FUERON PAUSADOS EL PLANIFICADOR LOS PAUSO POR SABOTAJE
	int dis = calcular_distancia(t->pos_x, t->pos_y, pos_sab_x, pos_sab_y);
	printf("Trip %d (%d,%d) -> (%d,%d) | distancia = %d \n", t->id, t->pos_x, t->pos_y, pos_sab_x, pos_sab_y, dis);
}

void enviar_a_ready(dis_tripulante *tripulante)
{
	tripulante->estado = READY;
	agregar_elemento_a_cola(cola_de_ready, &mutex_cola_de_ready, tripulante);
}

void vaciar_lista(t_list *lista)
{
	while (list_size(lista) > 0)
	{
		list_remove(lista, 0);
	}
}
// FIN SERVIDOR PARA SABOTAJES ---------------------------------

void leer_consola(t_log *logger)
{
	char *leido;
	leido = readline(">");
	while (1)
	{
		log_info(logger, leido);
		validacion_sintactica(leido); // VALIDACION SINTACTICA

		add_history(leido);
		free(leido);
		leido = readline(">");
	}

	free(leido);
}

int chequear_expulsion_de_tripulante(t_list *cola, pthread_mutex_t *mutex){
	pthread_mutex_lock(mutex);
	dis_tripulante *trip = list_remove_by_condition(cola,(void*)fue_expulsado);
	pthread_mutex_unlock(mutex);

	if(trip){
			tripulante_expulsado(trip);
			return trip->id;
	}
	return false;
}

bool fue_expulsado(dis_tripulante *tripulante){
	bool flag;
	pthread_mutex_lock(&(tripulante->mutex_expulsado));
	flag = tripulante->expulsado;
	pthread_mutex_unlock(&(tripulante->mutex_expulsado));
	return flag;
}

void tripulante_expulsado(dis_tripulante *tripulante){
	log_warning(logger, "EXPULSANDO TRIPULANTE %i DE LA NAVE...", tripulante->id);
	tripulante->estado = EXPULSADO;
	int status = pthread_kill(tripulante->threadid, 0);
        if (status < 0)
            perror("pthread_kill failed");

	int size_paquete = sizeof(uint32_t);
    void *info = pserializar_tid(tripulante->id);

	int conexion_mi_ram_hq = crear_conexion(ip_mi_ram_hq, puerto_mi_ram_hq);
    enviar_paquete(conexion_mi_ram_hq, EXPULSAR_TRIPULANTE, size_paquete, info);
    //t_paquete *paquete_recibido = recibir_paquete(conexion_mi_ram_hq);
    close(conexion_mi_ram_hq);
}

//----------- PLANIFICADORES--------------
void *planificador_a_largo_plazo()
{
	while (1)
	{
		//si hay tripulantes enviados a NEW y la planificación está, actuará.
		sem_wait(&sem_contador_cola_de_new);
		chequear_planificacion_pausada(&sem_planificador_a_largo_plazo,-1);

		//chequear si alguno fue expulsado
		int res = chequear_expulsion_de_tripulante(cola_de_new, &mutex_cola_de_new);
		if( res ){
			log_warning(logger, "[ Planificador ] : Tripulante %i expulsado con exito!", res);
			continue;
		}

		loggear_estado_de_cola(cola_de_new, "Planificador a largo plazo", "New antes del ciclo");

		dis_tripulante *tripulante = (dis_tripulante *)quitar_primer_elemento_de_cola(cola_de_new, &mutex_cola_de_new);
		tripulante->estado = READY;
		agregar_elemento_a_cola(cola_de_ready, &mutex_cola_de_ready, tripulante);
		sleep(tiempo_retardo_ciclo_cpu);
		
		actualizar_estado_miriam(tripulante->id,tripulante->estado);
		//ACA estaria lo que dice juan
		sem_post(&sem_contador_cola_de_ready);
		sem_post(&(tripulante->sem_tri));
		loggear_estado_de_cola(cola_de_new, "Planificador a largo plazo", "New despues del ciclo");
	}
	return NULL;
}

// EL 'temp' o 'id' es el identificador del procesador
void *procesar_tripulante_fifo(void *temp)
{
	int id = *((int *)temp);
	free(temp);
	sem_t sem_procesador;
	sem_init(&sem_procesador, 0, 0);
	list_add(hilos_pausables, &sem_procesador);

	dis_tripulante *tripulante;
	bool tripulante_procesado(dis_tripulante * t)
	{
		return t->id == tripulante->id;
	}

	while (1)
	{
		// anomalia fue vencida con esta guasada pero funciona asi que no cuestiono los resultados
		// algun dia le encontraremos la explicacion
		// Por algún motivo ignoraba la primera vez al semaforo y no hacia el sem wait. Por que? kcsho
		//chequear_planificacion_pausada(&sem_procesador, id);
		sem_wait(&sem_contador_cola_de_ready);
		chequear_planificacion_pausada(&sem_procesador, id);

		//chequeo si alguno fue expulsado
		int res = chequear_expulsion_de_tripulante(cola_de_ready, &mutex_cola_de_ready);
		if( res ){
			log_warning(logger, "[ Procesador %i ] : antes de exec Tripulante %i expulsado con exito!", id, res);
			continue;
		}

		tripulante = seleccionar_tripulante_a_ejecutar();
		if(!tripulante){
			continue;
		}
		
		log_info(logger, "[ Procesador %i ] Elije Tripulante %i para ejecutar", id, tripulante->id);
		while (tripulante->estado == EXEC)
		{
			printf("[ Procesador %i ] Estado antes del ciclo %i. El de EXEC es %i.\n", id, tripulante->estado, EXEC);
			sem_post(&(tripulante->sem_tri));
			sem_wait(&(tripulante->procesador));
			printf("[ Procesador %i ] 1 ciclo ejecutado. Estado nuevo %i\n", id, tripulante->estado);

			// ATENDIENDO INTERRUPCIONES : Se hacen al final de 1 cilco de ejecucion
			atender_interrupciones(&sem_procesador, id, tripulante);
		}
		// (un caso muy borde)
		// SI justo termina su rafago y recien se cambia el estado del sabotaje(un caso muy borde) agrregarle un && !planificacion_pausada
		if (!sabotaje)
		{
			log_info(logger, "[ Procesador %i ] Rafaga finalizada. Tripulante %i ejecutado. %i tareas hechas", id, tripulante->id, tripulante->tareas_realizadas);

			pthread_mutex_lock(&mutex_cola_de_exec);
			list_remove_by_condition(cola_de_exec, (void *)tripulante_procesado);
			pthread_mutex_unlock(&mutex_cola_de_exec);
		}

		switch (tripulante->estado)
		{
		case BLOCKED_E_S:
			agregar_elemento_a_cola(cola_de_block, &mutex_cola_de_block, tripulante);
			sem_post(&sem_contador_cola_de_block);
			break;
		case EXIT:
			log_info(logger, "[ Procesador %i ] El Tripulante %i ha finalizado", id, tripulante->id);
			break;
		case EXPULSADO:
			log_info(logger, "[ Procesador %i ] El Tripulante %i ha sido EXPULSADO", id, tripulante->id);
			break;
		case BLOCKED_SABOTAJE:
			log_info(logger, "[ Procesador %i ] Mandando al Tripulante %i a BLOQUEADO_SABOTAJE", id, tripulante->id);
			break;
		default:
			log_error(logger, "[ Procesador %i ] Que verga pasó, no debería estar aca. Discordiador.c linea 281", id);
			break;
		}
	}
	return NULL;
}

void *procesar_tripulante_rr(void *temp)
{
	int id = *((int *)temp);
	free(temp);
	int quantum_actual = 0;
	sem_t sem_procesador;
	sem_init(&sem_procesador, 0, 0);
	list_add(hilos_pausables, &sem_procesador);

	dis_tripulante *tripulante;
	bool tripulante_procesado(dis_tripulante * t)
	{
		return t->id == tripulante->id;
	}
	
	while (1)
	{
		//chequear_planificacion_pausada(&sem_procesador, id);
		sem_wait(&sem_contador_cola_de_ready);
		chequear_planificacion_pausada(&sem_procesador, id);

		//chequeo si alguno fue expulsado
		int res = chequear_expulsion_de_tripulante(cola_de_ready, &mutex_cola_de_ready);
		if( res ){
			log_warning(logger, "[ Procesador %i ] : Tripulante %i expulsado con exito!", id, res);
			continue;
		}

		tripulante = seleccionar_tripulante_a_ejecutar();
		if(!tripulante){
			continue;
		}

		log_info(logger, "[ Procesador %i ] Elije Tripulante %i para ejecutar. Quantum actual: %i", id, tripulante->id, quantum_actual);
		while (quantum_actual < quantum_rr && tripulante->estado == EXEC)
		{
			sem_post(&(tripulante->sem_tri));
			sem_wait(&(tripulante->procesador));
			printf("[ Procesador %i ] 1 ciclo ejecutado\n", id);
			quantum_actual++;
			// ATENDIENDO INTERRUPCIONES : Se hacen al final de 1 cilco de ejecucion
			atender_interrupciones(&sem_procesador, id, tripulante);
		}

		if (!sabotaje)
		{
			log_info(logger, "[ Procesador %i ] Rafaga finalizada. Tripulante %i ejecutado. %i tareas hechas. Quantum actual: %i", id, tripulante->id, tripulante->tareas_realizadas, quantum_actual);
			printf("[ Procesador %i ] Estado %i. El de Exit es %i\n", id, tripulante->estado, EXIT);

			pthread_mutex_lock(&mutex_cola_de_exec);
			list_remove_by_condition(cola_de_exec, (void *)tripulante_procesado);
			pthread_mutex_unlock(&mutex_cola_de_exec);
		}

		switch (tripulante->estado)
		{
		case BLOCKED_E_S:
			agregar_elemento_a_cola(cola_de_block, &mutex_cola_de_block, tripulante);
			sem_post(&sem_contador_cola_de_block);
			break;
		case EXIT:
			log_info(logger, "[ Procesador %i ] El Tripulante %i ha finalizado. Quantum actual: %i", id, tripulante->id, quantum_actual);
			break;
		case EXEC: //quiere decir que todavia no terminó de ejecutar, por lo que lo envío al final de ready
			tripulante->estado = READY;
			
			actualizar_estado_miriam(tripulante->id,tripulante->estado);
			
			agregar_elemento_a_cola(cola_de_ready, &mutex_cola_de_ready, tripulante);
			sem_post(&sem_contador_cola_de_ready);
			break;
		case EXPULSADO:
			log_info(logger, "[ Procesador %i ] El Tripulante %i ha sido EXPULSADO", id, tripulante->id);
			break;
		case BLOCKED_SABOTAJE:
			log_info(logger, "[ Procesador %i ] Mandando al Tripulante %i a BLOQUEADO_SABOTAJE", id, tripulante->id);
			break;
		default:
			log_error(logger, "[ Procesador %i ] No debería estar aca. Discordiador.c linea 342\n", id);
			break;
		}
		if (quantum_actual == quantum_rr)
			quantum_actual = 0;
	}
	return NULL;
}

void *ejecutar_tripulantes_bloqueados()
{
	while (1)
	{
		sem_wait(&sem_contador_cola_de_block);

		int res = chequear_expulsion_de_tripulante(cola_de_block, &mutex_cola_de_block); 
		if( res ){
			log_warning(logger, "[ Dispositivo E/S ] : Tripulante %i expulsado con exito!", res);
			continue;
		}

		dis_tripulante *trip = (dis_tripulante *)list_get(cola_de_block, 0);
		loggear_estado_de_cola(cola_de_block, "Dispositivo E/S", "block antes de procesar tripulante");
		while (trip->estado == BLOCKED_E_S)
		{
			sem_post(&(trip->sem_tri));
			sem_wait(&operacion_entrada_salida_finalizada);
			chequear_planificacion_pausada(&sem_hilo_de_bloqueados, -2);
			printf("[ Dispositivo E/S ] 1 ciclo de E/S completado\n");

			int res = chequear_expulsion_de_tripulante(cola_de_block, &mutex_cola_de_block);
			if( res ){
				log_warning(logger, "[ Dispositivo E/S ] : Tripulante %i expulsado con exito!", res);
				continue;
			}
		}

		log_info(logger, "[ Dispositivo E/S ] Rafaga finalizada. Tripulante %i ejecutado. %i tareas hechas",trip->id, trip->tareas_realizadas);

		switch (trip->estado)
		{
		case READY:
			trip = (dis_tripulante *)quitar_primer_elemento_de_cola(cola_de_block, &mutex_cola_de_block);
			agregar_elemento_a_cola(cola_de_ready, &mutex_cola_de_ready, trip);
			sem_post(&sem_contador_cola_de_ready);
			break;
		case EXPULSADO:
			log_info(logger, "[ Dispositivo E/S ] El Tripulante %i ha sido EXPULSADO", trip->id);
			break;
		default:
			log_info(logger, "[ Dispositivo E/S ] No debería estar aca. Discordiador.c Linea 319");
			break;
		}
		loggear_estado_de_cola(cola_de_block, "Dispositivo E/S", "block despues de procesar tripulante");
	}
	return NULL;
}

dis_tripulante *seleccionar_tripulante_a_ejecutar()
{
	dis_tripulante *tripulante = (dis_tripulante *)quitar_primer_elemento_de_cola(cola_de_ready, &mutex_cola_de_ready);

	if(!tripulante)
		return NULL;
	
	tripulante->estado = EXEC;
	actualizar_estado_miriam(tripulante->id,tripulante->estado);
	agregar_elemento_a_cola(cola_de_exec, &mutex_cola_de_exec, tripulante);
	return tripulante;
}

void atender_interrupciones(sem_t *sem, int id, dis_tripulante *tripulante)
{
	int res = chequear_expulsion_de_tripulante(cola_de_exec, &mutex_cola_de_exec);

	if( res ){
		log_warning(logger, "[ Procesador %i ] : atender interrupciones Tripulante %i expulsado.", id, res);
	}
	res = chequear_expulsion_de_tripulante(cola_de_ready, &mutex_cola_de_ready);
	if( res ){
		log_warning(logger, "[ Procesador %i ] : atender interrupciones Tripulante %i expulsado.", id, res);
	}

	if (planificacion_pausada)
	{
		int value;
		sem_getvalue(sem, &value);
		if (value  == 1)
			sem_wait(sem);

		if (sabotaje)
		{
			tripulante->estado = BLOCKED_SABOTAJE;
			actualizar_estado_miriam(tripulante->id,tripulante->estado);
		}
		else
		{
			log_warning(logger, "[ Procesador %i ] Pausado", id);
			sem_wait(sem);
		}
	}
}
//----------- FIN PLANIFICADORES ---------

void agregar_elemento_a_cola(t_list *cola, pthread_mutex_t *mutex, void *elemento)
{
	pthread_mutex_lock(mutex);
	list_add(cola, elemento);
	pthread_mutex_unlock(mutex);
}

void *quitar_primer_elemento_de_cola(t_list *cola, pthread_mutex_t *mutex)
{
	void *elemento;
	pthread_mutex_lock(mutex);
	if(list_size(cola) > 0)
		elemento = list_remove(cola, 0);
	else
		elemento = NULL;
	
	pthread_mutex_unlock(mutex);
	return elemento;
}

void chequear_planificacion_pausada(sem_t *sem, int id)
{
	if (planificacion_pausada)
	{
		int value;
		sem_getvalue(sem, &value);
		if (value  == 1)
			sem_wait(sem);

		sem_getvalue(sem, &value);
		
		if (id >= 0)
		{
			if (sabotaje)
				log_warning(logger, "[ Procesador %i ] Pausado por sabotaje | cont %i", id,value);
			else
				log_warning(logger, "[ Procesador %i ] Pausado | cont %i", id,value);
			
		}
		if (id == -1)
			log_warning(logger, "[ Planificador ] Pausado | cont %i",value);
			
		if (id == -2)
			log_warning(logger, "[ Dispositivo E/S ] Pausado | cont %i",value);
		
		// sem_post(semaforo_sabotaje)
		sem_wait(sem);
	}
}

void pausar_planificacion()
{
	planificacion_pausada = true;
}

void iniciar_planificacion()
{
	void reanudar_hilo(sem_t * sem)
	{	
		int value;
		sem_getvalue(sem, &value);
		if (value <= 0){
			sem_post(sem);
		}
	}
	planificacion_pausada = false;
	list_iterate(hilos_pausables, (void *)reanudar_hilo);
}
//---------- Funciones para ver el estado de las colas ---------

void loggear_estado_de_cola(t_list *cola, char *thread, char *tipo_de_cola)
{
	log_info(logger, "[ %s ] Estado de la cola %s: %i elementos.", thread, tipo_de_cola, list_size(cola));
}
//---------- FIN Funciones para ver el estado de las colas ---------

void catch_sigint_signal(int signal)
{
	terminar_programa();
}

void terminar_programa()
{
	log_warning(logger, "QUIT: Finalizando el programa...");
	log_destroy(logger);
	config_destroy(config);
	liberar_listas();
	exit(0);
}


//----------FUNCION HECHA POR DAMI PASAR A DONDE CORRESPONDA ----------//
void actualizar_estado_miriam(int tid,estado est){
	int conexion_mi_ram_hq = crear_conexion(ip_mi_ram_hq, puerto_mi_ram_hq);
    void *info = serializar_estado_tcb(est,tid); 
    uint32_t size_paquete = sizeof(uint32_t)+sizeof(estado);
    enviar_paquete(conexion_mi_ram_hq, ACTUALIZAR_ESTADO, size_paquete, info); 
    t_paquete *paquete_recibido = recibir_paquete(conexion_mi_ram_hq);
    respuesta_ok_fail respuesta = deserializar_respuesta_ok_fail(paquete_recibido->stream);
	if(respuesta == RESPUESTA_FAIL)
	 	printf("\033[1;31m[RESPUESTA_FAIL] discordiador.c Linea 781 \033[0m\n");
	
	liberar_paquete(paquete_recibido);
    close(conexion_mi_ram_hq);
}

void liberar_listas(){
	// Estas colas deberian estar vacias
	list_destroy(cola_de_new);
	list_destroy(cola_de_ready);
	list_destroy(cola_de_exec);
	list_destroy(cola_de_block);
	//list_destroy(cola_de_iniciar_patotas); es un t_queue

	// Estas estan llenas, aqui ya se termino la ejecucion del proceso	
	list_destroy_and_destroy_elements(lista_de_patotas, free); // solo tiene (ints)
	list_destroy_and_destroy_elements(list_total_tripulantes, free);
}

// MENSAJES PARA IMONGO

// void notificar_atencion_sabotaje_imongo(int id_trip , char* tarea){// podriamos solo serializar el tid y luego desde imongo crear y guardar el log(decidan)
//     /*      id_trip : id del Tripulante
//             tarea : "Fin de Tarea SABOTAJE"
//     */
//     int conexion_i_mongo_store = crear_conexion(ip_i_mongo_store, puerto_i_mongo_store);
//     void * info = serializar_trip_con_char(id_trip,tarea);
//     uint32_t size_paquete = 2*sizeof(uint32_t) + strlen(tarea) + 1;
//     enviar_paquete(conexion_i_mongo_store, ACTUALIZAR_UBICACION, size_paquete, info); // Ver el opcode si hay uno mejor
//     //[TODO] recibir el ok
// 	close(conexion_i_mongo_store);
// }

void notificar_fin_sabotaje_imongo(int id_trip){// podriamos solo serializar el tid y luego desde imongo crear y guardar el log(decidan)
    /*      id_trip : id del Tripulante		*/
    
    int conexion_i_mongo_store = crear_conexion(ip_i_mongo_store, puerto_i_mongo_store);
	op_code codigo = REGISTRAR_SABOTAJE_RESUELTO;
	void* stream = serializar_pid(id_trip);
	size_t size = sizeof(uint32_t);
	enviar_paquete(conexion_i_mongo_store,codigo,size,stream);
	// [ISSUE] recv()
    // [TODO] recibir el ok
    close(conexion_i_mongo_store);
}