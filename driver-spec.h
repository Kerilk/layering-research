/**
 * The API exposed by driver implementing the toy API.
 */

/**
 * API objects should provide a writable pointer to an undefined structure.
 * This will enable the loader to set the required data for multiplexing and
 * layering.
 */
struct platform_s {
	struct multiplex_s *multiplex;
};

struct device_s {
	struct multiplex_s *multiplex;
};

typedef struct platform_s * platform_t;
typedef struct device_s * device_t;

/**
 * Query available platforms in this driver (see OpenCL clIcdGetPlatformIDsKHR).
 */
int
getPlatformsExt(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret);


/**
 * Return an API entry point a for a given platform of this driver. (see OpenCL
 * clGetExtensionFunctionAddressForPlatform)
 */
void *
platformGetFunc(platform_t platform, const char *name);
