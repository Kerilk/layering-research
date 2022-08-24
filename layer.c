#include <stddef.h>
#include "spec.h"
#include "dispatch.h"
#include "layer.h"
#include <stdio.h>

#ifndef LAYER_NUMBER
#define LAYER_NUMBER 1
#endif

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
		size_t             *num_entries_ret,
		struct dispatch_s **layer_dispatch_ret) {
	printf("LAYER %d: entering layerInit(num_entries = %zu, target_dispatch = %p, num_entries_ret = %p, layer_dispatch_ret = %p)\n",
		LAYER_NUMBER, num_entries, (void *)target_dispatch, (void *)num_entries_ret, (void *)layer_dispatch_ret);
	if (num_entries < NUM_DISPATCH_ENTRIES)
		return SPEC_ERROR;
	if (!target_dispatch || !num_entries_ret || !layer_dispatch_ret)
		return SPEC_ERROR;
	_target_dispatch = target_dispatch;
	*num_entries_ret = NUM_DISPATCH_ENTRIES;
	*layer_dispatch_ret = &_dispatch;
	return SPEC_SUCCESS;
}

static int
getPlatforms_wrap(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret) {
	printf("LAYER %d: entering getPlatforms(num_platforms = %zu, platforms = %p, num_platforms_ret = %p)\n",
		LAYER_NUMBER, num_platforms, (void *)platforms, (void *)num_platforms_ret);
	int res = _target_dispatch->getPlatforms(num_platforms, platforms, num_platforms_ret);
	printf("LAYER %d: leaving getPlatforms, result = %d\n", LAYER_NUMBER, res);
	return res;
}

static int
platformCreateDevice_wrap(platform_t platform, device_t *device_ret) {
	printf("LAYER %d: entering platformCreateDevice(platform = %p, device_ret = %p)\n",
		LAYER_NUMBER, (void *)platform, (void *)device_ret);
	int res = _target_dispatch->platformCreateDevice(platform, device_ret);
	printf("LAYER %d: leaving platformCreateDevice, result = %d, device_ret_val = %p\n",
		LAYER_NUMBER, res, device_ret ? (void *)*device_ret : NULL);
	return res;
}

#if LAYER_NUMBER == 1
static int
deviceFunc1_wrap(device_t device, int param) {
	printf("LAYER %d: entering deviceFunc1(device = %p, param %d)\n",
		LAYER_NUMBER, (void *)device, param);
	int res = _target_dispatch->deviceFunc1(device, param);
	printf("LAYER %d: leaving deviceFunc1, result = %d\n", LAYER_NUMBER, res);
	return res;
}
#endif

static int
deviceFunc2_wrap(device_t device, int param) {
	printf("LAYER %d: entering deviceFunc2(device = %p, param %d)\n",
		LAYER_NUMBER, (void *)device, param);
	int res = _target_dispatch->deviceFunc2(device, param);
	printf("LAYER %d: leaving deviceFunc2, result = %d\n", LAYER_NUMBER, res);
	return res;
}

static int
deviceDestroy_wrap(device_t device) {
	printf("LAYER %d: entering deviceDestroy(device = %p)\n",
		LAYER_NUMBER, (void *)device);
	int res = _target_dispatch->deviceDestroy(device);
	printf("LAYER %d: leaving deviceDestroy, result = %d\n", LAYER_NUMBER, res);
	return res;
}
