#include "discordiador.h"
#include "consola.h"
#include "funciones.h"

int main(void)
{
	char *ip_mi_ram_hq;
	char *puerto_mi_ram_hq;
	int conexion_i_mongo_store;
	char *ip_i_mongo_store;
	char *puerto_i_mongo_store;
	char *puerto_escucha_sabotaje;

	/*	tarea1.cantidad_parametro = 1;
  tarea1.cantidad_parametro = 1;
	tarea1.nombre_tarea = "tarea1";
	tarea1.parametro = 1;
	tarea1.pos_x = 10;
	tarea1.pos_y = 12;
	tarea1.tiempo = 5;
	tarea2.cantidad_parametro = 1;
	tarea2.nombre_tarea = "tarea2";
	tarea2.parametro = 1;
	tarea2.pos_x = 25;
	tarea2.pos_y = 27;
	tarea2.tiempo = 10;
	t_list* tareas = list_create();
	tarea* auxiliar1 = malloc(sizeof(tarea));
	tarea* auxiliar2 = malloc(sizeof(tarea));
	*auxiliar1 = tarea1;
	*auxiliar2 = tarea2;
	list_add(tareas,auxiliar1);
	list_add(tareas,auxiliar2);
*/

	/*  [TODO] lo comente por que aun no se a usado
	uint32_t pid = 100;

	pid_con_tareas mensaje;
	mensaje.pid = pid;
	mensaje.tareas = tareas;
	*/

	logger = iniciar_logger();
	config = leer_config();

	//puerto de escucha para los sabotajes
	puerto_escucha_sabotaje = config_get_string_value(config, "PUERTO_ESCUCHA_SABOTAJE");
	pthread_t hilo_server;
	pthread_create(&hilo_server, NULL, esperar_conexiones, atoi(puerto_escucha_sabotaje));

	//crear conexiones
	ip_mi_ram_hq = config_get_string_value(config, "IP_MI_RAM_HQ");
	puerto_mi_ram_hq = config_get_string_value(config, "PUERTO_MI_RAM_HQ");
	conexion_mi_ram_hq = crear_conexion(ip_mi_ram_hq, puerto_mi_ram_hq);

	// ss
	ip_i_mongo_store = config_get_string_value(config, "IP_I_MONGO_STORE");
	puerto_i_mongo_store = config_get_string_value(config, "PUERTO_I_MONGO_STORE");
	conexion_i_mongo_store = crear_conexion(ip_i_mongo_store, puerto_i_mongo_store);
  
  crear_tareas();

	pthread_t hilo_consola;
	pthread_create(&hilo_consola, NULL, (void *)leer_consola_prueba, (void *)logger);
	pthread_join(hilo_consola, NULL);

	terminar_programa(conexion_mi_ram_hq, conexion_i_mongo_store, logger, config);
}

t_log *iniciar_logger(void)
{
	return log_create("cfg/discordiador.log", "discordiador.log", 1, LOG_LEVEL_INFO);
}

t_config *leer_config(void)
{
	return config_create("cfg/discordiador.config");
}

void *esperar_conexiones(int puerto)
{
	t_list *hilos = list_create();
	int socket_escucha = iniciar_servidor_discordiador(puerto);
	int socket_cliente;
	log_info(logger, "SERVIDOR LEVANTADO! ESCUCHANDO EN %i", puerto);
	struct sockaddr cliente;
	socklen_t len = sizeof(cliente);
	do
	{
		socket_cliente = accept(socket_escucha, &cliente, &len);
		if (socket_cliente > 0)
		{
			log_info(logger, "NUEVA CONEXIÓN!");
			crear_hilo_para_manejar_suscripciones(hilos, socket_cliente);
		}
		else
		{
			log_error(logger, "ERROR ACEPTANDO CONEXIONES: %s", strerror(errno));
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

void crear_hilo_para_manejar_suscripciones(t_list *lista_hilos, int socket)
{
	int *socket_hilo = malloc(sizeof(int));
	*socket_hilo = socket;
	pthread_t hilo_conectado;
	pthread_create(&hilo_conectado, NULL, (void *)manejar_suscripciones_discordiador, socket_hilo);
	list_add(lista_hilos, &hilo_conectado);
}

void *manejar_suscripciones_discordiador(int *socket_hilo)
{
	log_info(logger, "Se creó el hilo correctamente. Socket fd %i", *socket_hilo);
	//LLENAME
	//CON AGUA
	close(*socket_hilo);
	free(socket_hilo);
}

void leer_consola(t_log *logger)
{
	char *leido;

	leido = readline(">");
	while (*leido != '\0')
	{
		log_info(logger, leido);

		free(leido);
		leido = readline(">");
	}

	free(leido);
}

void leer_consola_prueba(t_log *logger)
{
	char *leido;
	leido = readline(">");
	while (*leido != '\0')
	{

		log_info(logger, leido);
		validacion_sintactica(leido); // VALIDACION SINTACTICA

		//struct_prueba* una_prueba = malloc(sizeof(struct_prueba));
		//una_prueba->tamanio = strlen(leido)+1;
		//una_prueba->contenido = leido;
		//enviar_paquete(conexion_mi_ram_hq,PRUEBA,una_prueba->tamanio,una_prueba->contenido);
		free(leido);
		leido = readline(">");
	}
	
	free(leido);
}

void terminar_programa(int conexion_mi_ram_hq, int conexion_i_mongo_store, t_log *logger, t_config *config)
{
	//Y por ultimo, para cerrar, hay que liberar lo que utilizamos (conexion, log y config) con las funciones de las commons y del TP mencionadas en el enunciado
	log_destroy(logger);
	config_destroy(config);
	close(conexion_mi_ram_hq);
	close(conexion_i_mongo_store);
}
