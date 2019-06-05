#ifndef LFS_LISSANDRA_DUMP_H_
#define LFS_LISSANDRA_DUMP_H_

int inicializar_dumper();

void *dumpear(void *entrada);

void destruir_dumper();

int instanciar_hilo_dumper(lissandra_thread_periodic_t *lp_thread);

#endif
