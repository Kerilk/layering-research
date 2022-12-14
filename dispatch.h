/**
 * Definition of the dispatch table used by the loader and global layers.
 * It gathers all the API entry points defined inn spec.h in a structure.
 */

typedef int (*pfn_getPlatforms_t)(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret);
typedef int (*pfn_platformAddLayer_t)(platform_t platform, const char *layer_name);
typedef int (*pfn_platformCreateDevice_t)(platform_t platform, device_t *device_ret);
typedef int (*pfn_deviceFunc1_t)(device_t device, int param);
typedef int (*pfn_deviceFunc2_t)(device_t device, int param);
typedef int (*pfn_deviceDestroy_t)(device_t device);

struct dispatch_s {
	pfn_getPlatforms_t         getPlatforms;
	pfn_platformAddLayer_t     platformAddLayer;
	pfn_platformCreateDevice_t platformCreateDevice;
	pfn_deviceFunc1_t          deviceFunc1;
	pfn_deviceFunc2_t          deviceFunc2;
	pfn_deviceDestroy_t        deviceDestroy;
};

/**
 * Dispatch tables that gather APIs that drivers implement.
 * Use by the loader to dispatch driver calls.
 */
struct driver_dispatch_s {
	pfn_platformCreateDevice_t platformCreateDevice;
	pfn_deviceFunc1_t          deviceFunc1;
	pfn_deviceFunc2_t          deviceFunc2;
	pfn_deviceDestroy_t        deviceDestroy;
};
