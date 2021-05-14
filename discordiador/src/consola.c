#include "consola.h"
//#include "funciones.h"

Comando_Discordiador obtener_comando(char *comando)
{
    if (strcmp(comando, "INICIAR_PATOTA") == 0)
        return INIT_PATOTA;
    if (strcmp(comando, "LISTAR_TRIPULANTES") == 0)
        return LIST_CREWS;
    if (strcmp(comando, "EXPULSAR_TRIPULANTE") == 0)
        return EJECT_CREW;
    if (strcmp(comando, "INICIAR_PLANIFICACION") == 0)
        return INIT_PLANIFICATION;
    if (strcmp(comando, "PAUSAR_PLANIFICACION") == 0)
        return PAUSE_PLANIFICATION;
    if (strcmp(comando, "OBTENER_BITACORA") == 0)
        return GET_BINNACLE;
    return NO_VALIDO; //Para el warning
}

/*  Podria quitar esta funcion dado que tambien esta definida en "funciones.h"
void liberar_lista_string(char **lista)
{
    int i = 0;
    while (lista[i] != NULL)
    {
        free(lista[i]);
        i++;
    }
    free(lista);
}*/

bool posicion_validas_tripulantes(char **str_split)
{
    int i = 3;
    while (str_split[i] != NULL)
    {
        char **posiciones = string_split(str_split[i], "|");

        if (posiciones[0] == NULL || posiciones[1] == NULL)
        {
            printf("Esta mal a partir del %d parametro, formato '|' no encontrado\n", i);
            return false;
        }
        else if (atoi(posiciones[0]) == 0 || atoi(posiciones[1]) == 0)
        {
            printf("ejx:%s | ejy:%s\n", posiciones[0], posiciones[1]);
            printf("Las posiciones deben ser [int] en el parametro %d\n", i);
            return false;
        }
        liberar_lista_string(posiciones);
        i++;
    }
    return true;
}

void validacion_sintactica(char *text)
{
    char **str_split = string_split(text, " ");
    Comando_Discordiador comando = obtener_comando(str_split[0]);

    switch (comando)
    {
    //el nombre del enum lo puse en ingles porque en español ya estaba utilizado para los
    //op_codes
    case INIT_PATOTA:
        printf("Posible intento de comando iniciar patota\n");
        if (str_split[1] != NULL && str_split[2] != NULL && posicion_validas_tripulantes(str_split))
        {
            if (atoi(str_split[1]) != 0)
            {
                printf("INICIAR_PATOTA : OK\n");
                // [TODO]

                //existe patota?
                //crear patota y guardarla
                //armar mensaje inicializar_patota

                //for(cantidad de tripulantes)
                //crear tripulante
                //enviar tripilante

                //Por ahora voy a enviar mensaje de que "cree" una patota y enviar un mensaje por cada tripulante que creo
                uint32_t pid = crear_tareas_enviar_patota_test();
                for (int i = 0; i < atoi(str_split[1]); i++)
                {
                    //le paso i para que tenga un tid, pero habria que pasarle el de el hilo
                    crear_tripulante_test(pid, i);
                }
            }
            else
                printf("INICIAR_PATOTA : id tripulante no es un [int]\n");
        }
        else
            printf("Parametros incorrectos/faltantes\n");
        break;

    case LIST_CREWS:
        printf("Posible intento de listar tripulante\n");
        if (str_split[1] == NULL)
        {
            printf("LISTAR_TRIPULANTES : OK\n");
            // [TODO]
            // no encontre mensaje listar tripulantes, lo tenemos en el discordiador ya esto?
            
        }
        else
            printf("LISTAR_TRIPULANTES : no contiene parametros\n");
        break;

    case EJECT_CREW:
        printf("Posible intento de expulsar tripulante\n");
        if (str_split[1] != NULL && str_split[2] == NULL)
        {
            if (atoi(str_split[1]) != 0)
            {
                printf("EXPULSAR_TRIPULANTE : OK\n");
                // [TODO]
            }
            else
                printf("EXPULSAR_TRIPULANTE : id tripulante no es un [int]\n");
        }
        else
            printf("Parametros incorrectos/faltantes\n");
        break;

    case INIT_PLANIFICATION:
        printf("Posible intento de iniciar planificacion\n");
        if (str_split[1] == NULL)
        {
            printf("INICIAR_PLANIFICACION : OK\n");
            // [TODO]
        }
        else
            printf("INICIAR_PLANIFICACION : no contiene parametros\n");
        break;

    case PAUSE_PLANIFICATION:
        printf("Posible intento de pausar planificacion\n");
        if (str_split[1] == NULL)
        {
            printf("PAUSAR_PLANIFICACION : OK\n");
            // [TODO]
        }
        else
            printf("PAUSAR_PLANIFICACION : no contiene parametros\n");
        break;

    case GET_BINNACLE:
        printf("Posible intento de obtener bitacora\n");
        if (str_split[1] != NULL && str_split[2] == NULL)
        {
            if (atoi(str_split[1]) != 0)
            {
                printf("OBTENER_BITACORA : OK\n");
                // [TODO]
            }
            else
                printf("OBTENER_BITACORA : id tripulante no es un [int]\n");
        }
        else
            printf("Parametros incorrectos/faltantes\n");
        break;

    default:
        printf("COMANDO NO VALIDO\n");
        break;
    }
    // Para liberar una ves se termino el case
    liberar_lista_string(str_split);
}

uint32_t crear_tareas_enviar_patota_test()
{
    //creo tareas
    //creo 2 a mano para probar la funcionalidad

    tarea *nueva_tarea = malloc(sizeof(tarea));
    char *nombre_de_tarea = "GENERAR_OXÍGENO";
    nueva_tarea->nombre_tarea = nombre_de_tarea;
    //Esto no se si es esta longitud de toda la tarea o solo la del nombre de la tarea
    nueva_tarea->cantidad_parametro = sizeof(*nombre_de_tarea);
    nueva_tarea->parametro = 12;
    nueva_tarea->pos_x = 2;
    nueva_tarea->pos_y = 3;
    nueva_tarea->tiempo = 5;

    t_list *lista_tareas = list_create();
    list_add(lista_tareas, nueva_tarea);

    tarea *nueva_tarea2 = malloc(sizeof(tarea));
    char *nombre_de_tarea2 = "CONSUMIR_OXÍGENO";
    nueva_tarea2->nombre_tarea = nombre_de_tarea2;
    //Esto no se si es esta longitud de toda la tarea o solo la del nombre de la tarea
    nueva_tarea2->cantidad_parametro = sizeof(*nombre_de_tarea2);
    nueva_tarea2->parametro = 10;
    nueva_tarea2->pos_x = 3;
    nueva_tarea2->pos_y = 2;
    nueva_tarea2->tiempo = 7;

    list_add(lista_tareas, nueva_tarea2);

    //creo patota que tenga la lista de esas 2 tareas

    pid_con_tareas nueva_patota;
    nueva_patota.pid = 1234;
    nueva_patota.tareas = lista_tareas;

    //serializo el paquete
    void *info = serializar_pid_con_tareas(nueva_patota);
    //guardo el tamaño en una variable para poder debugearlo
    uint32_t size_paquete = sizeof(uint32_t) + (lista_tareas->elements_count * sizeof(t_link_element));

    //envio el paquete
    enviar_paquete(conexion_mi_ram_hq, INICIAR_PATOTA, size_paquete, info);

    //recibo respuesta
    t_paquete *paquete_recibido = recibir_paquete(conexion_mi_ram_hq);

    if (paquete_recibido->codigo_operacion == RESPUESTA_INICIAR_PATOTA)
    {
        printf("Recibi opcode de respuesta okfail\n");
        //desserializo la respuesta
        respuesta_ok_fail respuesta = deserializar_respuesta_ok_fail(paquete_recibido->stream);

        //analizo respuesta
        if (respuesta == RESPUESTA_OK)
        {
            printf("Recibi respuesta OK\n");
        }
        else if (respuesta == RESPUESTA_FAIL)
        {
            printf("Recibi respuesta FAIL\n");
        }
        else
        {
            printf("Recibi respuesta INVALIDA\n");
        }
    }
    else
    {
        printf("Recibi opcode de respuesta INVALIDO\n");
    }
    //esto deberia ser el pid de la patota creada

    //free(nueva_tarea->nombre_tarea); esto primero
    free(nueva_tarea);
    free(nueva_tarea2);
    list_destroy(lista_tareas);//esto cuando lo generalizemos usaremos la funcion de las commons
    liberar_paquete(paquete_recibido);

    return 1234;
}
void crear_tripulante_test(uint32_t pid, int n)
{
    nuevo_tripulante tripulante;
    tripulante.pid = pid;
    tripulante.pos_x = 0;
    tripulante.pos_y = 0;
    tripulante.tid = n;

    void *info = serializar_nuevo_tripulante(tripulante);

    enviar_paquete(conexion_mi_ram_hq, INICIAR_TRIPULANTE, sizeof(nuevo_tripulante), info);

    t_paquete *paquete_recibido = recibir_paquete(conexion_mi_ram_hq);

    if (paquete_recibido->codigo_operacion == RESPUESTA_INICIAR_TRIPULANTE)
    {
        printf("Recibi opcode de respuesta okfail\n");
        //desserializo la respuesta
        respuesta_ok_fail respuesta = deserializar_respuesta_ok_fail(paquete_recibido->stream);

        //analizo respuesta
        if (respuesta == RESPUESTA_OK)
        {
            printf("Recibi respuesta OK\n");
        }
        else if (respuesta == RESPUESTA_FAIL)
        {
            printf("Recibi respuesta FAIL\n");
        }
        else
        {
            printf("Recibi respuesta INVALIDA\n");
        }
    }
    else
    {
        printf("Recibi opcode de respuesta INVALIDO\n");
    }
    liberar_paquete(paquete_recibido);
}
