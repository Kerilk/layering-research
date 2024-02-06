#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "spec.h"

#ifdef NO_PROTOTYPES
#include <dlfcn.h>
#include <inttypes.h>
static getPlatforms_t         *getPlatforms;
static platformAddLayer_t     *platformAddLayer;
static platformCreateDevice_t *platformCreateDevice;
static deviceFunc1_t          *deviceFunc1;
static deviceFunc2_t          *deviceFunc2;
static deviceDestroy_t        *deviceDestroy;

#define GET_SYM(sym) \
do { \
 sym = (sym ## _t *)(intptr_t)dlsym(handle, #sym); \
 assert(sym); \
} while (0)
#endif

void test_platform(platform_t platform) {
	device_t device;
	int err;
	printf("Testing platform %p\n", (void *)platform);
	err = platformCreateDevice(platform, &device);
	printf("Created device = %p, err = %d\n", (void *)device, err);
	assert(!err);
	err = deviceFunc1(device, 0);
	printf("Called deviceFunc1, err = %d\n", err);
	err = deviceFunc2(device, 1);
	printf("Called deviceFunc2, err = %d\n", err);
	err = deviceDestroy(device);
	printf("Destroyed device = %p, err = %d\n", (void *)device, err);
	assert(!err);
}

int main() {
	size_t num_platforms = 0;
	platform_t *platforms = NULL;
#ifdef NO_PROTOTYPES
	void *handle = dlopen("libexp-loader.so", RTLD_LAZY|RTLD_LOCAL);
	assert(handle);
	GET_SYM(getPlatforms);
	GET_SYM(platformAddLayer);
	GET_SYM(platformCreateDevice);
	GET_SYM(deviceFunc1);
	GET_SYM(deviceFunc2);
	GET_SYM(deviceDestroy);
	printf("Opened loader %p\n", handle);
#endif
	int err = getPlatforms(0, NULL, &num_platforms);
	printf("Found %zu platforms, err = %d\n", num_platforms, err);
	assert(!err);
	if (!num_platforms)
		return 0;
	platforms = (platform_t *)malloc(num_platforms * sizeof(platform_t));
	err = getPlatforms(num_platforms, platforms, NULL);
	printf("Got platforms, err = %d\n", err);
	assert(!err);
	err = platformAddLayer(platforms[0], "libinstance_layer1.so");
	printf("Added instance layer1, err = %d\n", err);
	err = platformAddLayer(platforms[0], "libinstance_layer2.so");
	printf("Added instance layer2, err = %d\n", err);
	for (size_t i = 0; i < num_platforms; i++)
		test_platform(platforms[i]);
	free(platforms);
#ifdef NO_PROTOTYPES
	int res = dlclose(handle);
	assert(!res);
	printf("Closed loader %p\n", handle);
#endif
	return 0;
}
