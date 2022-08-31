#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "spec.h"
#include "dispatch.h"
#include "layer.h"
#include "instance_layer.h"

#ifndef LAYER_NUMBER
#define LAYER_NUMBER 1
#endif

#define LAYER_LOG(format, ...) \
do  { \
	printf("INSTANCE LAYER %d: " format "\n", LAYER_NUMBER, __VA_ARGS__); \
} while (0)

#if FFI_INSTANCE_LAYERS
struct ffi_wrap_data {
	ffi_closure *closure;
	ffi_cif      cif;
};

struct ffi_layer_data {
	struct dispatch_s    *target_dispatch;
	struct ffi_wrap_data  platformCreateDevice;
	struct ffi_wrap_data  deviceFunc1;
	struct ffi_wrap_data  deviceFunc2;
	struct ffi_wrap_data  deviceDestroy;
};

typedef struct ffi_layer_data instance_layer_t;
#define CALL_NEXT_LAYER(layer, api, ...) layer->target_dispatch->api(__VA_ARGS__)

#else //!FFI_INSTANCE_LAYERS

typedef struct instance_layer_proxy_s instance_layer_t;
#define NEXT_LAYER(layer, api) (layer->layer_dispatch.api ## _next)
#define NEXT_ENTRY(layer, api) NEXT_LAYER(layer, api)->dispatch.api ## _instance
#define CALL_NEXT_LAYER(layer, api, ...) NEXT_ENTRY(layer, api)(NEXT_LAYER(layer, api), __VA_ARGS__)

#endif //!FFI_INSTANCE_LAYERS

static inline int
platformCreateDevice_instance(
		instance_layer_t *layer,
		platform_t        platform,
		device_t         *device_ret) {
	LAYER_LOG("entering platformCreateDevice(platform = %p, device_ret = %p)",
		(void *)platform, (void *)device_ret);
	int res = CALL_NEXT_LAYER(layer, platformCreateDevice, platform, device_ret);
	LAYER_LOG("leaving platformCreateDevice, result = %d, device_ret_val = %p",
		res, device_ret ? (void *)*device_ret : NULL);
	return res;
}

#if LAYER_NUMBER == 1
static inline int
deviceFunc1_instance(
		instance_layer_t *layer,
		device_t          device,
		int               param) {
	LAYER_LOG("entering deviceFunc1(device = %p, param %d)", (void *)device, param);
	int res = CALL_NEXT_LAYER(layer, deviceFunc1, device, param);
	LAYER_LOG("eaving deviceFunc1, result = %d", res);
	return res;
}
#endif

static inline int
deviceFunc2_instance(
		instance_layer_t *layer,
		device_t          device,
		int               param) {
	LAYER_LOG("entering deviceFunc2(device = %p, param %d)", (void *)device, param);
	int res = CALL_NEXT_LAYER(layer, deviceFunc2, device, param);
	LAYER_LOG("leaving deviceFunc2, result = %d", res);
	return res;
}

static inline int
deviceDestroy_instance(
		instance_layer_t *layer,
		device_t          device) {
	LAYER_LOG("entering deviceDestroy(device = %p)", (void *)device);
	int res = CALL_NEXT_LAYER(layer, deviceDestroy, device);
	LAYER_LOG("leaving deviceDestroy, result = %d", res);
	return res;
}

#if FFI_INSTANCE_LAYERS

#define WRAPPER_NAME(api) api ## _ffi_wrap

#define DECLARE_WRAPPER(api) \
static int \
WRAPPER_NAME(api)(struct ffi_layer_data *layer_data, struct ffi_wrap_data *wrap_data, pfn_ ## api ## _t *f_ptr_ret)

DECLARE_WRAPPER(platformCreateDevice);
#if LAYER_NUMBER == 1
DECLARE_WRAPPER(deviceFunc1);
#endif
DECLARE_WRAPPER(deviceFunc2);
DECLARE_WRAPPER(deviceDestroy);

static inline int
wrap_call(
		struct ffi_layer_data  *layer_data,
		void                   *pfun_ffi,
		unsigned int            nargs,
		ffi_type               *rtype,
		ffi_type              **atypes,
		struct ffi_wrap_data   *wrap_data,
		void                  **pfun_ret) {
	void *code;
	wrap_data->closure = ffi_closure_alloc(
		sizeof(ffi_closure), (void **)&code);
	if (!wrap_data->closure)
		goto error;
	ffi_status status = ffi_prep_cif(
		&wrap_data->cif, FFI_DEFAULT_ABI, nargs, rtype, atypes);
	if (FFI_OK != status)
		goto error_closure;
	status = ffi_prep_closure_loc(
		wrap_data->closure, &wrap_data->cif,
		(void (*)(ffi_cif *, void *, void **, void *))(intptr_t)pfun_ffi,
		(void *)layer_data, code);
	if (FFI_OK != status)
		goto error_closure;
	*pfun_ret = code;
	return SPEC_SUCCESS;
error_closure:
	ffi_closure_free(wrap_data->closure);
	wrap_data->closure = NULL;
error:
	return SPEC_ERROR;
}

#define WRAPPER(api) \
DECLARE_WRAPPER(api) { \
	return wrap_call( \
		layer_data, \
		(void *)(intptr_t)api ## _ffi, \
		api ## _ffi_nargs, api ## _ffi_ret, api ## _ffi_types, \
		wrap_data, (void **)f_ptr_ret); \
}

WRAPPER(platformCreateDevice)
#if LAYER_NUMBER == 1
WRAPPER(deviceFunc1)
#endif
WRAPPER(deviceFunc2)
WRAPPER(deviceDestroy)

#define WRAP(api) do { \
	int res = WRAPPER_NAME(api)( \
		layer_data, \
		&layer_data->api, \
		&layer_instance_dispatch->api); \
	if (SPEC_SUCCESS != res) \
		goto err_wrap; \
} while (0)


#define UNWRAP(api) do { \
	if (layer_data->api.closure) \
		ffi_closure_free(layer_data->api.closure); \
} while (0)

static inline void
cleanup_closures(struct ffi_layer_data *layer_data) {
	UNWRAP(platformCreateDevice);
#if LAYER_NUMBER == 1
	UNWRAP(deviceFunc1);
#endif
	UNWRAP(deviceFunc2);
	UNWRAP(deviceDestroy);
}

int layerInstanceInit(
		size_t              num_entries,
		struct dispatch_s  *target_dispatch,
		struct dispatch_s  *layer_instance_dispatch,
		void              **layer_data_ret) {
	LAYER_LOG("entering layerInit(num_entries = %zu, target_dispatch = %p, layer_instance_dispatch = %p, layer_data_ret = %p)",
		num_entries, (void *)target_dispatch, (void *)layer_instance_dispatch, (void *)layer_data_ret);
	if (num_entries < NUM_DISPATCH_ENTRIES)
		return SPEC_ERROR;
	if (!target_dispatch || !layer_instance_dispatch || !layer_data_ret)
		return SPEC_ERROR;
	struct ffi_layer_data *layer_data =
		(struct ffi_layer_data *)calloc(1, sizeof(struct ffi_layer_data));
	if (!layer_data)
		return SPEC_ERROR;
	WRAP(platformCreateDevice);
#if LAYER_NUMBER == 1
	WRAP(deviceFunc1);
#endif
	WRAP(deviceFunc2);
	WRAP(deviceDestroy);
	layer_data->target_dispatch = target_dispatch;
	*layer_data_ret = (void *)layer_data;
	return SPEC_SUCCESS;
err_wrap:
	cleanup_closures(layer_data);
	free(layer_data);
	return SPEC_ERROR;
}

int layerInstanceDeinit(
		void *layer_data) {
	LAYER_LOG("entering layerInstanceDeinit(layer_data = %p)", (void *)layer_data);
	cleanup_closures((struct ffi_layer_data *)layer_data);
	free(layer_data);
	return SPEC_SUCCESS;
}


static void
platformCreateDevice_ffi(
		ffi_cif                              *cif,
		int                                  *ffi_ret,
		struct platformCreateDevice_ffi_args *args,
		void                                 *data) {
	(void)cif;
	instance_layer_t *layer_data = (instance_layer_t *)data;
	platform_t  platform = *args->p_platform;
	device_t   *device_ret = *args->p_device_ret;
	*ffi_ret = platformCreateDevice_instance(layer_data, platform, device_ret);
}

#if LAYER_NUMBER == 1
static void
deviceFunc1_ffi(
		ffi_cif                     *cif,
		int                         *ffi_ret,
		struct deviceFunc1_ffi_args *args,
		void                        *data) {
	(void)cif;
	instance_layer_t *layer_data = (instance_layer_t *)data;
	device_t device = *args->p_device;
	int      param  = *args->p_param;
	*ffi_ret = deviceFunc1_instance(layer_data, device, param);
}
#endif

static void
deviceFunc2_ffi(
		ffi_cif                     *cif,
		int                         *ffi_ret,
		struct deviceFunc2_ffi_args *args,
		void                        *data) {
	(void)cif;
	instance_layer_t *layer_data = (instance_layer_t *)data;
	device_t device = *args->p_device;
	int      param  = *args->p_param;
	*ffi_ret = deviceFunc2_instance(layer_data, device, param);
}

static void
deviceDestroy_ffi(
		ffi_cif                       *cif,
		int                           *ffi_ret,
		struct deviceDestroy_ffi_args *args,
		void                          *data) {
	(void)cif;
	instance_layer_t *layer_data = (instance_layer_t *)data;
	device_t device = *args->p_device;
	*ffi_ret = deviceDestroy_instance(layer_data, device);
}

#else //!FFI_INSTANCE_LAYERS

static struct instance_dispatch_s _dispatch = {
	&platformCreateDevice_instance,
#if LAYER_NUMBER == 1
	&deviceFunc1_instance,
#else
	NULL,
#endif
	&deviceFunc2_instance,
	&deviceDestroy_instance
};

int layerInstanceInit(
		size_t                       num_entries,
		struct instance_dispatch_s  *layer_instance_dispatch,
		void                       **layer_data_ret) {
	LAYER_LOG("entering layerInit(num_entries = %zu, layer_instance_dispatch = %p, layer_data_ret = %p)",
		num_entries, (void *)layer_instance_dispatch, (void *)layer_data_ret);
	if (num_entries < NUM_INSTANCE_DISPATCH_ENTRIES)
		return SPEC_ERROR;
	if (!layer_instance_dispatch || !layer_data_ret)
		return SPEC_ERROR;
	*layer_instance_dispatch = _dispatch;
	// for debug purposes
	*layer_data_ret = malloc(0x16);
	return SPEC_SUCCESS;
}

int layerInstanceDeinit(
		void *layer_data) {
	LAYER_LOG("entering layerInstanceDeinit(layer_data = %p)", (void *)layer_data);
	free(layer_data);
	return SPEC_SUCCESS;
}

#endif //FFI_INSTANCE_LAYERS
