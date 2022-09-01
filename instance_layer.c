#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "spec.h"
#include "dispatch.h"
#include "layer.h"
#include "instance_layer.h"

/**
 * This file contains an implementation of the instance layer API defined in
 * layer.h.  Several different layers can be created through the use of the
 * LAYER_NUMBER macro definition.  The layer will only intercept deviceFunc1
 * when LAYER_NUMBER == 1, showcasing the use of partial layering. Also none of
 * them intercept platformAddLayer (not a limitation, they could). Depending on
 * the FFI_INSTANCE_LAYERS macro definition the FFI or regular flavor of the
 * layer will be built.
 * The instance_layer.h file contains definitions for FFI layers that would be
 * shared between layers written in FFI. Several other helper functions written
 * here could also be included in this file.
 */

#ifndef LAYER_NUMBER
#define LAYER_NUMBER 1
#endif

#define LAYER_LOG(format, ...) \
do  { \
	printf("INSTANCE LAYER %d: " format "\n", LAYER_NUMBER, __VA_ARGS__); \
} while (0)

#if FFI_INSTANCE_LAYERS

/**
 * FFI layers rely on closures to join wrapper functions with a context.  The
 * closures, and the target dispatch to call into, are the context of these
 * instance layers, but this could be augmented with other information.
 */
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

/**
 * Non FFI instance layers rely on a convention between the loader and the
 * layers that a chain of instance layer will be composed of structures
 * containing:
 *  - an (possibly incomplete) instance dispatch table containing the layer
 *    supported APIs wrappers
 *  - a complete layer dispatch table containing links to the next layer for
 *    each instance APIs
 *  - a pointer to this layer internal instance data 
 */
typedef struct instance_layer_proxy_s instance_layer_t;
#define NEXT_LAYER(layer, api) (layer->layer_dispatch.api ## _next)
#define NEXT_ENTRY(layer, api) NEXT_LAYER(layer, api)->dispatch.api ## _instance
#define CALL_NEXT_LAYER(layer, api, ...) NEXT_ENTRY(layer, api)(NEXT_LAYER(layer, api), __VA_ARGS__)

#endif //!FFI_INSTANCE_LAYERS

/**
 * These are the wrapper functions the layers should implement, their signature
 * only differs from instance API calls in the extra first parameter.
 */
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

/**
 * Using FFI requires an additional closure around the wrapper to capture the
 * context. These closures will have the exact same interface as the API
 * entry points being wrapped.
 */
#define WRAPPER_NAME(api) api ## _ffi_wrap

#define DECLARE_WRAPPER(api) \
static int \
WRAPPER_NAME(api)(struct ffi_layer_data *layer_data, struct ffi_wrap_data *wrap_data, pfn_ ## api ## _t *f_ptr_ret)

DECLARE_WRAPPER(platformCreateDevice);
static platformCreateDevice_ffi_t platformCreateDevice_ffi;
#if LAYER_NUMBER == 1
DECLARE_WRAPPER(deviceFunc1);
static deviceFunc1_ffi_t deviceFunc1_ffi;
#endif
DECLARE_WRAPPER(deviceFunc2);
static deviceFunc2_ffi_t deviceFunc2_ffi;
DECLARE_WRAPPER(deviceDestroy);
static deviceDestroy_ffi_t deviceDestroy_ffi;

/**
 * This function realizes the ffi closures with the given argument types and
 * return value type, plus a function to call (`pfun_ffi`) and the layer
 * instance context `layer_data`. The layer entry point is returned in `pfun_ret`.
 * For more details refer to the ffi documentation:
 * http://www.chiark.greenend.org.uk/doc/libffi-dev/html/The-Closure-API.html
 * http://www.chiark.greenend.org.uk/doc/libffi-dev/html/Closure-Example.html
 *
 * As it doesn't require cooperation between loader and layers (instance or
 * global), FFI also allows wrapping arbitrary extension functions that would
 * be obtained through calls similar to
 * clGetExtensionFunctionAddressForPlatform.  This property is difficult to
 * reach with other schemes.
 */
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


/**
 * Create ffi wrapper funcions for each supported APIs using the data defined in
 * inastance_layer.h.
 */
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

/**
 * Destroy closures when cleaning up.
 */
static inline void
cleanup_closures(struct ffi_layer_data *layer_data) {
	UNWRAP(platformCreateDevice);
#if LAYER_NUMBER == 1
	UNWRAP(deviceFunc1);
#endif
	UNWRAP(deviceFunc2);
	UNWRAP(deviceDestroy);
}

/**
 * Instantiating FFI instance layers requires the same information as when
 * instantiating  global layers. An additional context is returned that will
 * provided back by the loader when this layer instance is deinited.
 * Similarly, the dispatch table provided to write for the layer will be
 * completed by the loader so doesn't need to be fully filled.
 */
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

/**
 * Context clean up.
 */
int layerInstanceDeinit(
		void *layer_data) {
	LAYER_LOG("entering layerInstanceDeinit(layer_data = %p)", (void *)layer_data);
	cleanup_closures((struct ffi_layer_data *)layer_data);
	free(layer_data);
	return SPEC_SUCCESS;
}

/**
 * FFI specific wrappers. Could be provided in instance_layer.h.
 */
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

/** Instance layer without FFI are more straightforward, the loader/layer chain
 * is responsible for correctly managing the cibtext along the chain.
 */

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

/**
 * Here the target dispatch is not required as it will be provided by the layer
 * chain during layer invocation, so the layer reports its supported APis the
 * usual way.
 */
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
	// for debug purposes here
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
