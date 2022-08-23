typedef int (*pfn_getPlatforms_t)(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret);
typedef int (*pfn_platformCreateDevice_t)(platform_t plt, device_t *dev);
typedef int (*pfn_deviceFunc1_t)(device_t dev, int param);
typedef int (*pfn_deviceFunc2_t)(device_t dev, int param);
typedef int (*pfn_deviceDestroy_t)(device_t dev);

struct dispatch_s {
	pfn_getPlatforms_t         getPlatforms;
	pfn_platformCreateDevice_t platformCreateDevice;
	pfn_deviceFunc1_t          deviceFunc1;
	pfn_deviceFunc2_t          deviceFunc2;
	pfn_deviceDestroy_t        deviceDestroy;
};


