#define SPEC_SUCCESS 0
#define SPEC_ERROR -1
#define SPEC_UNSUPPORTED -2

typedef struct platform_s * platform_t;
typedef struct device_s * device_t;

extern int
getPlatforms(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret);

extern int
platformAddLayer(platform_t platform, const char *layer_name);

extern int
platformCreateDevice(platform_t platform, device_t *device_ret);

extern int
deviceFunc1(device_t device, int param);

extern int
deviceFunc2(device_t device, int param);

extern int
deviceDestroy(device_t device);
