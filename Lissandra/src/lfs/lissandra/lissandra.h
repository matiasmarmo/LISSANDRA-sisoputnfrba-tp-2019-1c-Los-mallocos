#ifndef LFS_LISSANDRA_LISSANDRA_H_
#define LFS_LISSANDRA_LISSANDRA_H_

int manejar_create(void* create_request, void* create_response);

int manejar_single_describe(void* describe_request, void* single_describe_response);

int manejar_global_describe(void* global_describe_response);

int manejar_drop(void* drop_request, void* drop_response);

int manejar_insert(void* insert_request, void* insert_response);

int manejar_select(void* select_request, void* select_response);

#endif
