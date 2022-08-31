#define NUM_DISPATCH_ENTRIES (sizeof(struct dispatch_s)/sizeof(pfn_layerInit_t))

#ifndef FFI_INSTANCE_LAYERS
#define FFI_INSTANCE_LAYERS 1
#endif

typedef int layerInit_t(
	size_t              num_entries,
	struct dispatch_s  *target_dispatch,
	struct dispatch_s  *layer_dispatch);

typedef layerInit_t *pfn_layerInit_t;

typedef int layerDeinit_t();

typedef layerDeinit_t *pfn_layerDeinit_t;

#if FFI_INSTANCE_LAYERS

typedef int layerInstanceInit_t(
	size_t              num_entries,
	struct dispatch_s  *target_dispatch,
	struct dispatch_s  *layer_instance_dispatch,
	void              **layer_data_ret);

#else //!FFI_INSTANCE_LAYERS

struct instance_dispatch_s;
struct instance_layer_proxy_s;

struct layer_dispatch_s {
	struct instance_layer_proxy_s *platformCreateDevice_next;
	struct instance_layer_proxy_s *deviceFunc1_next;
	struct instance_layer_proxy_s *deviceFunc2_next;
	struct instance_layer_proxy_s *deviceDestroy_next;
};

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

struct instance_dispatch_s {
	pfn_platformCreateDevice_instance_t platformCreateDevice_instance;
	pfn_deviceFunc1_instance_t          deviceFunc1_instance;
	pfn_deviceFunc2_instance_t          deviceFunc2_instance;
	pfn_deviceDestroy_instance_t        deviceDestroy_instance;
};

struct instance_layer_proxy_s {
	struct instance_dispatch_s     dispatch;
	struct layer_dispatch_s        layer_dispatch;
	void                          *data;
};

#define NUM_INSTANCE_DISPATCH_ENTRIES (sizeof(struct instance_dispatch_s)/sizeof(pfn_layerInit_t))

typedef int layerInstanceInit_t(
	size_t                       num_entries,
	struct instance_dispatch_s  *layer_instance_dispatch,
	void                       **layer_data_ret);

#endif //FFI_INSTANCE_LAYERS

typedef layerInstanceInit_t *pfn_layerInstanceInit_t;

typedef int layerInstanceDeinit_t(
	void *layer_data);

typedef layerInstanceDeinit_t *pfn_layerInstanceDeinit_t;
