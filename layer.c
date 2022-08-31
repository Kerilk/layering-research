#include <stddef.h>
#include "spec.h"
#include "dispatch.h"
#include "layer.h"
#include <stdio.h>
#include <string.h>

#ifndef LAYER_NUMBER
#define LAYER_NUMBER 1
#endif

#define LAYER_LOG(format, ...) \
do  { \
	printf("LAYER %d: " format "\n", LAYER_NUMBER, __VA_ARGS__); \
} while (0)

#define LAYER_LOG_NO_ARGS(format) \
do  { \
	printf("LAYER %d: " format "\n", LAYER_NUMBER); \
} while (0)

static int
getPlatforms_wrap(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret);
static int
platformCreateDevice_wrap(platform_t platform, device_t *device_ret);
#if LAYER_NUMBER == 1
static int
deviceFunc1_wrap(device_t device, int param);
#endif
static int
deviceFunc2_wrap(device_t device, int param);
static int
deviceDestroy_wrap(device_t device);

static struct dispatch_s _dispatch = {
	&getPlatforms_wrap,
	NULL,
	&platformCreateDevice_wrap,
#if LAYER_NUMBER == 1
	&deviceFunc1_wrap,
#else
	NULL,
#endif
	&deviceFunc2_wrap,
	&deviceDestroy_wrap
};

static struct dispatch_s *_target_dispatch = NULL;

int layerInit(
		size_t              num_entries,
		struct dispatch_s  *target_dispatch,
		struct dispatch_s  *layer_dispatch) {
	LAYER_LOG("entering layerInit(num_entries = %zu, target_dispatch = %p, layer_dispatch = %p)",
		num_entries, (void *)target_dispatch, (void *)layer_dispatch);
	if (num_entries < NUM_DISPATCH_ENTRIES)
		return SPEC_ERROR;
	if (!target_dispatch || !layer_dispatch)
		return SPEC_ERROR;
	_target_dispatch = target_dispatch;
	memcpy(layer_dispatch, &_dispatch, sizeof(_dispatch));
	return SPEC_SUCCESS;
}

#if LAYER_NUMBER == 1
int layerDeinit() {
	LAYER_LOG_NO_ARGS("entering layerDeinit()");
	return SPEC_SUCCESS;
}
#endif

static int
getPlatforms_wrap(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret) {
	LAYER_LOG("entering getPlatforms(num_platforms = %zu, platforms = %p, num_platforms_ret = %p)",
		num_platforms, (void *)platforms, (void *)num_platforms_ret);
	int res = _target_dispatch->getPlatforms(num_platforms, platforms, num_platforms_ret);
	LAYER_LOG("leaving getPlatforms, result = %d", res);
	return res;
}

static int
platformCreateDevice_wrap(platform_t platform, device_t *device_ret) {
	LAYER_LOG("entering platformCreateDevice(platform = %p, device_ret = %p)",
		(void *)platform, (void *)device_ret);
	int res = _target_dispatch->platformCreateDevice(platform, device_ret);
	LAYER_LOG("leaving platformCreateDevice, result = %d, device_ret_val = %p",
		res, device_ret ? (void *)*device_ret : NULL);
	return res;
}

#if LAYER_NUMBER == 1
static int
deviceFunc1_wrap(device_t device, int param) {
	LAYER_LOG("entering deviceFunc1(device = %p, param %d)", (void *)device, param);
	int res = _target_dispatch->deviceFunc1(device, param);
	LAYER_LOG("leaving deviceFunc1, result = %d", res);
	return res;
}
#endif

static int
deviceFunc2_wrap(device_t device, int param) {
	LAYER_LOG("entering deviceFunc2(device = %p, param %d)", (void *)device, param);
	int res = _target_dispatch->deviceFunc2(device, param);
	LAYER_LOG("leaving deviceFunc2, result = %d", res);
	return res;
}

static int
deviceDestroy_wrap(device_t device) {
	LAYER_LOG("entering deviceDestroy(device = %p)", (void *)device);
	int res = _target_dispatch->deviceDestroy(device);
	LAYER_LOG("leaving deviceDestroy, result = %d", res);
	return res;
}
