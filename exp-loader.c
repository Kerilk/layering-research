#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <assert.h>

static void *handle = NULL;

void initLoader() {
	printf("Initing\n");
	handle = dlopen("libdriver.so", RTLD_LAZY|RTLD_LOCAL);
	assert(handle);
	printf("Opened %p\n", handle);
}

void deinitLoader() {
	printf("Closing %p\n", handle);
	int res = dlclose(handle);
	assert(!res);
	handle = NULL;
}

__attribute__((destructor))
void my_fini(void) {
	printf("Deiniting\n");
	if (handle)
		deinitLoader();
}
