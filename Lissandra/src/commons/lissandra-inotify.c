#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <sys/inotify.h>
#include <sys/select.h>

#include "lissandra-threads.h"

typedef struct entrada_watch {
	int inotify_fd;
	int watch_fd;
	int (*on_event)();
} entrada_watch_t;

void watch_cleanup(void *entrada) {
	entrada_watch_t *entrada_watch = (entrada_watch_t*) entrada;
	inotify_rm_watch(entrada_watch->inotify_fd, entrada_watch->watch_fd);
	close(entrada_watch->inotify_fd);
	free(entrada);
}

void *watch(void *entrada) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) entrada;
	int inotify_fd = ((entrada_watch_t*) l_thread->entrada)->inotify_fd;
	int (*on_event)() = ((entrada_watch_t*) l_thread->entrada)->on_event;
	pthread_cleanup_push(&watch_cleanup, l_thread->entrada);
	// El tamaño del buffer es el suficiente para almacenar un evento de inotify
	int tamanio_buffer = sizeof(struct inotify_event) + NAME_MAX + 1;
	uint8_t buffer[tamanio_buffer];
	int select_ret, read_ret;
	struct timeval timeout;
	fd_set saved_fd_set;
	FD_ZERO(&saved_fd_set);
	FD_SET(inotify_fd, &saved_fd_set);
	while (!l_thread_debe_finalizar(l_thread)) {
		fd_set set = saved_fd_set;
		timeout.tv_sec = 0;
		timeout.tv_usec = 50000;
		select_ret = select(inotify_fd + 1, &set, NULL, NULL, &timeout);
		if (select_ret <= 0) {
			continue;
		}
		if ((read_ret = read(inotify_fd, buffer, tamanio_buffer)) < 0) {
			continue;
		}
		on_event();
	}
	pthread_cleanup_pop(1);
	pthread_exit(NULL);
}

int iniciar_inotify_watch(char *path, int (*on_event)(),
		lissandra_thread_t *l_thread) {
	int inotify_fd, watch_fd;
	entrada_watch_t *entrada_watch;

	if ((inotify_fd = inotify_init()) < 0) {
		return -1;
	}

	if ((watch_fd = inotify_add_watch(inotify_fd, path, IN_MODIFY)) < 0) {
		close(inotify_fd);
		return -1;
	}

	if ((entrada_watch = malloc(sizeof(entrada_watch_t))) == NULL) {
		inotify_rm_watch(inotify_fd, watch_fd);
		close(inotify_fd);
		return -1;
	}

	entrada_watch->inotify_fd = inotify_fd;
	entrada_watch->watch_fd = watch_fd;
	entrada_watch->on_event = on_event;

	if (l_thread_create(l_thread, &watch, entrada_watch) != 0) {
		free(entrada_watch);
		inotify_rm_watch(inotify_fd, watch_fd);
		close(inotify_fd);
		return -1;
	}

	return 0;
}
