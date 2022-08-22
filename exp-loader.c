#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <assert.h>

void *handle = NULL;

void initLoader() {
	printf("Initing\n");
	handle = dlopen("libdriver.so", RTLD_LAZY|RTLD_LOCAL);
	assert(handle);
}

void deinitLoader() {
	dlclose(handle);
	handle = NULL;
}

__attribute__((destructor))
void my_fini(void) {
	printf("Deiniting\n");
	if (handle)
		deinitLoader();
}
