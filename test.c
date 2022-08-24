#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "spec.h"

int main() {
	size_t num_platforms = 0;
	platform_t *platforms = NULL;
	int err = getPlatforms(0, NULL, &num_platforms);
	printf("Found %zu platforms, err = %d\n", num_platforms, err);
	assert(!err);
	if (!num_platforms)
		return 0;
	platforms = (platform_t *)malloc(num_platforms * sizeof(platform_t));
	err = getPlatforms(num_platforms, platforms, NULL);
	printf("Got platforms, err = %d\n", err);
	assert(!err);
	device_t device;
	err = platformCreateDevice(platforms[0], &device);
	printf("Created device = %p, err = %d\n", (void *)device, err);
	assert(!err);
	err = deviceFunc1(device, 0);
	printf("Called deviceFunc1, err = %d\n", err);
	err = deviceFunc2(device, 1);
	printf("Called deviceFunc2, err = %d\n", err);
	err = deviceDestroy(device);
	printf("Destroyed device = %p, err = %d\n", (void *)device, err);
	assert(!err);
	free(platforms);
	return 0;
}
