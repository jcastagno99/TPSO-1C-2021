#include "i-mongo-store-lib.h"

int main(int argc, char*argv[])
{
	//Al levantar el modulo, primer argumento: path del archivo de config
	//segundo argumento: path donde quiero crear el log del modulo
	i_mongo_store_configuracion = leer_config_i_mongo_store(argv[1]);
	logger_i_mongo_store = iniciar_logger_i_mongo_store(argv[2]);

	//memoria_principal = malloc(mi_ram_hq_configuracion->TAMANIO_MEMORIA);
	
	esperar_conexion(i_mongo_store_configuracion->PUERTO);
}