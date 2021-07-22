#include "mi-ram-hq-lib.h"

	
int main(int argc, char*argv[])
{
	//Al levantar el modulo, primer argumento: path del archivo de config       //segundo argumento: path donde quiero crear el log del modulo
	mi_ram_hq_configuracion = leer_config_mi_ram_hq(argv[1]);
	logger_ram_hq = iniciar_logger_mi_ram_hq(argv[2]);
	memoria_principal = malloc(mi_ram_hq_configuracion->TAMANIO_MEMORIA);

	crear_estructuras_administrativas();
	
	esperar_conexion(mi_ram_hq_configuracion->PUERTO);
	
}