#include <stddef.h>
#include "spec.h"
#include "dispatch.h"
#include "layer.h"
#include <stdio.h>

/**
 * This file contains an implementation of the global layer API defined in
 * layer.h.  Several different layers can be created through the use of the
 * LAYER_NUMBER macro definition.  The layer will only intercept deviceFunc1
 * when LAYER_NUMBER == 1, showcasing the use of partial layering. Also none of
 * them intercept platformAddLayer (not a limitation, they could). Only when
 * LAYER_NUMBER == 1 is the layer implementing layerDeinit, showcasing its
 * optionality.
 */

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

/**
 * Global variable pointing to the next layer dispatch table (or loader
 * terminator).
 */
static struct dispatch_s *_target_dispatch = NULL;

/**
 * API wrappers of the layer. Teir signatures should be identical to the API calls thay intercept.
 */
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

/**
 * Dispatch table of the layer, can be incomplete, or shorter than the loader
 * dispatch tables, enabling older layer to be used on newer loaders.
 */
static struct dispatch_s _dispatch = {
	&getPlatforms_wrap,
	NULL, // platformAddLayer
	&platformCreateDevice_wrap,
#if LAYER_NUMBER == 1
	&deviceFunc1_wrap,
#else
	NULL,
#endif
	&deviceFunc2_wrap,
	&deviceDestroy_wrap
};

/**
 * This function receives two pointers to dispatch tables structures
 * (containing `num_entries` entries).  The first pointer is to the dispatch
 * table to call (it is guaranteed to be fully filled), While the second is
 * zero initialized and is to be fille by the layer. The layer is not required
 * to completely fill the second dispatch table, and the loader will complete
 * it with the pointer from the first dispatch table.
 * This particular layer implementation errors if the dispatch table provided by
 * the loader is shorter than it's own, but this is not a requirement, and the
 * layer can proceed if the methods it requires are available.
 */
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
	*layer_dispatch = _dispatch;
	return SPEC_SUCCESS;
}

#if LAYER_NUMBER == 1
/**
 * This showcase printing layer does not have an internal state, so this
 * function is only logging.
 */
int layerDeinit() {
	LAYER_LOG_NO_ARGS("entering layerDeinit()");
	return SPEC_SUCCESS;
}
#endif

