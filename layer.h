/**
 * The API exposed by the layers. Layers that implement the instance layer API
 * have a different signature if they use the ffi or non-ffi strategy.
 */

#define NUM_DISPATCH_ENTRIES (sizeof(struct dispatch_s)/sizeof(pfn_layerInit_t))

#ifndef FFI_INSTANCE_LAYERS
#define FFI_INSTANCE_LAYERS 1
#endif

/**
 * Global layer initialization API. The number of entries into the next
 * dispatch table to call into is provided in num_entries, while the tbale
 * itself is pointed to by target_dispatch. The layer's own dispatch table is
 * to be copied into layer_dispatch. A layer should not write more entries than
 * num_entries, and must write NULL for unsupported APIs.
 */
typedef int layerInit_t(
	size_t              num_entries,
	struct dispatch_s  *target_dispatch,
	struct dispatch_s  *layer_dispatch);

typedef layerInit_t *pfn_layerInit_t;

/**
 * Optional deinitialization API for global layers.
 */
typedef int layerDeinit_t();

typedef layerDeinit_t *pfn_layerDeinit_t;

#if FFI_INSTANCE_LAYERS

/**
 * Instance Layer initialization for FFI instance layers.  Similar to layerInit
 * but the layer also return a pointer to its internal data for this instance
 * in layer_data_ret.
 */
typedef int layerInstanceInit_t(
	size_t              num_entries,
	struct dispatch_s  *target_dispatch,
	struct dispatch_s  *layer_instance_dispatch,
	void              **layer_data_ret);

#else //!FFI_INSTANCE_LAYERS

/**
 * Non FFI layers must realize the closure through other means.  Here we are
 * providing the layer data pointer as the first arguments of the API calls,
 * while still providing a full dispatch table to call into.
 */
struct instance_dispatch_s;
struct instance_layer_proxy_s;

/**
 * A table indication the next layer intercepting a given API entry point.
 */
struct layer_dispatch_s {
	struct instance_layer_proxy_s *platformCreateDevice_next;
	struct instance_layer_proxy_s *deviceFunc1_next;
	struct instance_layer_proxy_s *deviceFunc2_next;
	struct instance_layer_proxy_s *deviceDestroy_next;
};

/**
 * The layer API wrapper type, with the layer as the first parameter.
 * The loader and layer must use compatible representations.
 */
typedef int platformCreateDevice_instance_t(
	struct instance_layer_proxy_s *layer,
	platform_t                     platform,
	device_t                      *device_ret);
typedef platformCreateDevice_instance_t *pfn_platformCreateDevice_instance_t;

typedef int deviceFunc1_instance_t(
	struct instance_layer_proxy_s *layer,
	device_t                       device,
	int                            param);
typedef deviceFunc1_instance_t *pfn_deviceFunc1_instance_t;

typedef int deviceFunc2_instance_t(
	struct instance_layer_proxy_s *layer,
	device_t                       device,
	int                            param);
typedef deviceFunc2_instance_t *pfn_deviceFunc2_instance_t;

typedef int deviceDestroy_instance_t(
	struct instance_layer_proxy_s *layer,
	device_t                       device);
typedef deviceDestroy_instance_t *pfn_deviceDestroy_instance_t;

/**
 * Functions that are not dispatchable through objects do not need to be in
 * instance layers.
 */
struct instance_dispatch_s {
	pfn_platformCreateDevice_instance_t platformCreateDevice_instance;
	pfn_deviceFunc1_instance_t          deviceFunc1_instance;
	pfn_deviceFunc2_instance_t          deviceFunc2_instance;
	pfn_deviceDestroy_instance_t        deviceDestroy_instance;
};

/**
 * This structure must map to its equivalent in the loader.
 */
struct instance_layer_proxy_s {
	struct instance_dispatch_s     dispatch;
	struct layer_dispatch_s        layer_dispatch;
	void                          *data;
};

#define NUM_INSTANCE_DISPATCH_ENTRIES (sizeof(struct instance_dispatch_s)/sizeof(pfn_layerInit_t))

/**
 * Initialization if a non FFI instance layer. Here, the loader is responsible
 * for maintaining the call chain through the instance layers, so the target
 * dispatch is not required. Similarly to previous initialization functions, no
 * more than num_entries must be written in layer_instance_dispatch.
 */
typedef int layerInstanceInit_t(
	size_t                       num_entries,
	struct instance_dispatch_s  *layer_instance_dispatch,
	void                       **layer_data_ret);

#endif //FFI_INSTANCE_LAYERS

typedef layerInstanceInit_t *pfn_layerInstanceInit_t;

/**
 * Deinitialization function for instance layers, allowing the layer to free its
 * data for a particular instance.
 */
typedef int layerInstanceDeinit_t(
	void *layer_data);

typedef layerInstanceDeinit_t *pfn_layerInstanceDeinit_t;
