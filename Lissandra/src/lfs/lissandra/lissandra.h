#ifndef LFS_LISSANDRA_LISSANDRA_H_
#define LFS_LISSANDRA_LISSANDRA_H_

int manejar_create(void* create_request, void* create_response);

int manejar_single_describe(void* single_describe_request, void* single_describe_response);

int manejar_drop(void* drop_request, void* drop_response);

#endif
