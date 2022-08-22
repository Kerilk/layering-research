#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <dlfcn.h>
#include <assert.h>

int main() {
	void (*initLoader)();
	void *handle = dlopen("libexp-loader.so", RTLD_LAZY|RTLD_LOCAL);
	assert(handle);
	initLoader = (void (*)())(intptr_t)dlsym(handle, "initLoader");
	assert(initLoader);
	printf("Opened loader %p\n", handle);
	initLoader();
	if (getenv("FIX")) {
		void (*deinitLoader)();
		deinitLoader = (void (*)())(intptr_t)dlsym(handle, "deinitLoader");
		assert(deinitLoader);
		deinitLoader();
	}
	int res = dlclose(handle);
	assert(!res);
	printf("Closed loader %p\n", handle);
	return 0;
}
