/**
 * A simple toy API that defines a set of platform available,
 * and APIs to create objects (called device here) and invoke
 * methods on those objects.
 */

/**
 * A set of error codes to be returned by API entry points.
 */
#define SPEC_SUCCESS 0 // API call was a success
#define SPEC_ERROR -1 // API call failed
#define SPEC_UNSUPPORTED -2 // API call is not supported by the platform

/**
 * This API uses opaque handle to transfer ownership of objects to the user.
 */
typedef struct platform_s * platform_t;
typedef struct device_s * device_t;

/**
 * Query available platforms (see OpenCL clGetPlatformIDs).
 */
extern int
getPlatforms(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret);


/**
 * Programmatically attach an instance layer to a platform.
 */
extern int
platformAddLayer(platform_t platform, const char *layer_name);

/**
 * Create a new device and return it in the variable pointed to by device_ret.
 */
extern int
platformCreateDevice(platform_t platform, device_t *device_ret);

/**
 * A function on a device.
 */
extern int
deviceFunc1(device_t device, int param);

/**
 * Another function on a device.
 */
extern int
deviceFunc2(device_t device, int param);

/**
 * Destroy the given device.
 */
extern int
deviceDestroy(device_t device);
