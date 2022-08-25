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

struct ffi_wrap_data {
	ffi_closure *closure;
	ffi_cif      cif;
};

#define WRAPPER_NAME(api) api ## _ffi_wrap

struct ffi_layer_data {
	struct dispatch_s    *target_dispatch;
	struct ffi_wrap_data  platformCreateDevice;
	struct ffi_wrap_data  deviceFunc1;
	struct ffi_wrap_data  deviceFunc2;
	struct ffi_wrap_data  deviceDestroy;
};

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
	printf("INSTANCE LAYER %d: entering layerInit(num_entries = %zu, target_dispatch = %p, layer_instance_dispatch = %p, layer_data_ret = %p)\n",
		LAYER_NUMBER, num_entries, (void *)target_dispatch, (void *)layer_instance_dispatch, (void *)layer_data_ret);
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
	printf("INSTANCE LAYER %d: entering layerInstanceDeinit(layer_data = %p)\n",
		LAYER_NUMBER, (void *)layer_data);
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
	struct ffi_layer_data *layer_data = (struct ffi_layer_data *)data;
	platform_t  platform = *args->p_platform;
	device_t   *device_ret = *args->p_device_ret;
	printf("INSTANCE LAYER %d: entering platformCreateDevice(platform = %p, device_ret = %p)\n",
		LAYER_NUMBER, (void *)platform, (void *)device_ret);
	int res = layer_data->target_dispatch->platformCreateDevice(platform, device_ret);
	printf("INSTANCE LAYER %d: leaving platformCreateDevice, result = %d, device_ret_val = %p\n",
		LAYER_NUMBER, res, device_ret ? (void *)*device_ret : NULL);
	*ffi_ret = res;
}

#if LAYER_NUMBER == 1
static void
deviceFunc1_ffi(
		ffi_cif                     *cif,
		int                         *ffi_ret,
		struct deviceFunc1_ffi_args *args,
		void                        *data) {
	(void)cif;
	struct ffi_layer_data *layer_data = (struct ffi_layer_data *)data;
	device_t device = *args->p_device;
	int      param  = *args->p_param;
	printf("INSTANCE LAYER %d: entering deviceFunc1(device = %p, param %d)\n",
		LAYER_NUMBER, (void *)device, param);
	int res = layer_data->target_dispatch->deviceFunc1(device, param);
	printf("INSTANCE LAYER %d: leaving deviceFunc1, result = %d\n", LAYER_NUMBER, res);
	*ffi_ret = res;
}
#endif

static void
deviceFunc2_ffi(
		ffi_cif                     *cif,
		int                         *ffi_ret,
		struct deviceFunc2_ffi_args *args,
		void                        *data) {
	(void)cif;
	struct ffi_layer_data *layer_data = (struct ffi_layer_data *)data;
	device_t device = *args->p_device;
	int      param  = *args->p_param;
	printf("INSTANCE LAYER %d: entering deviceFunc2(device = %p, param %d)\n",
		LAYER_NUMBER, (void *)device, param);
	int res = layer_data->target_dispatch->deviceFunc2(device, param);
	printf("INSTANCE LAYER %d: leaving deviceFunc2, result = %d\n", LAYER_NUMBER, res);
	*ffi_ret = res;
}

static void
deviceDestroy_ffi(
		ffi_cif                       *cif,
		int                           *ffi_ret,
		struct deviceDestroy_ffi_args *args,
		void                          *data) {
	(void)cif;
	struct ffi_layer_data *layer_data = (struct ffi_layer_data *)data;
	device_t device = *args->p_device;
	printf("INSTANCE LAYER %d: entering deviceDestroy(device = %p)\n",
		LAYER_NUMBER, (void *)device);
	int res = layer_data->target_dispatch->deviceDestroy(device);
	printf("INSTANCE LAYER %d: leaving deviceDestroy, result = %d\n", LAYER_NUMBER, res);
	*ffi_ret = res;
}
