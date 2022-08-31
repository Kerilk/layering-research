#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "driver-spec.h"

#define SPEC_SUCCESS 0
#define SPEC_ERROR -1

#ifndef DRIVER_NUMBER
#define DRIVER_NUMBER 1
#endif

#define DRIVER_LOG(format, ...) \
do  { \
	printf("DRIVER %d: " format "\n", DRIVER_NUMBER, __VA_ARGS__); \
} while (0)

static struct platform_s _platform;

int
getPlatformsExt(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret) {
	DRIVER_LOG("entering getPlatformsExt(num_platforms = %zu, platforms = %p, num_platforms_ret = %p)",
		num_platforms, (void *)platforms, (void *)num_platforms_ret);
	if (num_platforms_ret)
		*num_platforms_ret = 1;
	if (num_platforms && platforms) {
		if (num_platforms < 1)
			return SPEC_ERROR;
		*platforms = &_platform;
		for (size_t i = 1; i < num_platforms; i++)
			platforms[i] = NULL;
	}
	return SPEC_SUCCESS;
}

static int
platformCreateDevice(platform_t platform, device_t *device_ret) {
	DRIVER_LOG("entering platformCreateDevice(platform = %p, device_ret = %p)",
		(void *)platform, (void *)device_ret);
	if (platform != &_platform)
		return SPEC_ERROR;
	if (!device_ret)
		return SPEC_ERROR;
	*device_ret = (struct device_s *)calloc(1, sizeof(struct device_s));
	DRIVER_LOG("allocated device %p", (void *)*device_ret);
	return SPEC_SUCCESS;
}

static int
deviceFunc1(device_t device, int param) {
	DRIVER_LOG("entering deviceFunc1(device = %p, param %d)", (void *)device, param);
	return SPEC_SUCCESS;
}


#if DRIVER_NUMBER == 1
static int
deviceFunc2(device_t device, int param) {
	DRIVER_LOG("entering deviceFunc2(device = %p, param %d)", (void *)device, param);
	return SPEC_SUCCESS;
}
#endif

static int
deviceDestroy(device_t device) {
	DRIVER_LOG("entering deviceDestroy(device = %p)", (void *)device);
	free((void *)device);
	return SPEC_SUCCESS;
}

void *
platformGetFunc(platform_t platform, const char *name) {
	DRIVER_LOG("entering platformGetFunc(platform = %p, name = %s)",
		(void *)platform, name);
	if (platform != &_platform)
		return NULL;
	if (!name)
		return NULL;
	if (!strcmp(name, "platformCreateDevice"))
		return (void *)(intptr_t)&platformCreateDevice;
	if (!strcmp(name, "deviceFunc1"))
		return (void *)(intptr_t)&deviceFunc1;
#if DRIVER_NUMBER == 1
	if (!strcmp(name, "deviceFunc2"))
		return (void *)(intptr_t)&deviceFunc2;
#endif
	if (!strcmp(name, "deviceDestroy"))
		return (void *)(intptr_t)&deviceDestroy;
	return NULL;
}
