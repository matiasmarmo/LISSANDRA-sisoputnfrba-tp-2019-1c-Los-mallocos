#ifndef LISSANDRA_INOTIFY_H_
#define LISSANDRA_INOTIFY_H_

#include "lissandra-threads.h"

int iniciar_inotify_watch(char*, int (*)(), lissandra_thread_t*);

#endif
