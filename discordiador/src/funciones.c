#include "funciones.h"

void liberar_lista_string(char**lista){
    int i=0;
    while (lista[i]!= NULL){
        free( lista[i]);
        i++;
    } 
    free(lista); 
}
void tarea_destroy(tarea* t) {
	free(t->nombre_tarea);
	free(t);
}

void iterator(tarea* t){// [ITERAR USANDO COMMONS]
   printf("%-18s | %3d | %3d | %3d | %3d\n",t->nombre_tarea,t->parametro,t->pos_x,t->pos_y,t->tiempo);
}

void crear_tareas(){
    // TAREA PARAMETROS;POS X;POS Y;TIEMPO  

    FILE *archivo = fopen("src/prueba.txt","r");// ESTO SE CAMBIARA POR UNA RESPUESTA DEL FILE SYSTEM CREO
    char caracteres[100];
    
    // [LISTA]
 	t_list * lista_tareas = list_create();

    while (feof(archivo) == 0){
 		fgets(caracteres,100,archivo);
        char** str_spl = string_split(caracteres, ";");
        char** tarea_p = string_split(str_spl[0]," ");

        tarea *nueva_tarea = malloc(sizeof(tarea));

        char *nombre_de_tarea = string_duplicate(tarea_p[0]);

        nueva_tarea->nombre_tarea = nombre_de_tarea;
        nueva_tarea->cantidad_parametro = 1;//Esto no se si es esta longitud de toda la tarea o solo la del nombre de la tarea
        nueva_tarea->parametro = atoi(tarea_p[1]);
        nueva_tarea->pos_x = atoi(str_spl[1]);
        nueva_tarea->pos_y = atoi(str_spl[2]);
        nueva_tarea->tiempo = atoi(str_spl[3]);

        list_add(lista_tareas, nueva_tarea);
        liberar_lista_string(str_spl);
        liberar_lista_string(tarea_p);
 	}
    fclose(archivo);

   //Vemos que esta en memoria
   printf("TAREA              |PARAM|POS_X|POS_Y| TIEMPO\n");
   list_iterate(lista_tareas, (void*)iterator);
   list_destroy_and_destroy_elements(lista_tareas, (void*) tarea_destroy);
}