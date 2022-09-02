#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include "spec.h"
#include "dispatch.h"
#include "layer.h"

/**
 * Terminators for the global layer chain, responsible for calling into the
 * instance layer chain.
 */
static int
getPlatforms_disp(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret);
static int
platformAddLayer_disp(platform_t platform, const char *layer_name);
static int
platformCreateDevice_disp(platform_t platform, device_t *device_ret);
static int
deviceFunc1_disp(device_t device, int param);
static int
deviceFunc2_disp(device_t device, int param);
static int
deviceDestroy_disp(device_t device);

/**
 * Stub functions for unimplemented APIs.
 */
static int
platformCreateDevice_unsup(platform_t platform, device_t *device_ret);
static int
deviceFunc1_unsup(device_t device, int param);
static int
deviceFunc2_unsup(device_t device, int param);
static int
deviceDestroy_unsup(device_t device);

/**
 * A dispatch table to initialize platform dispatch table with.
 * NULL entries are loader only APIs and should never be called.
 */
static struct driver_dispatch_s _unsup_dispatch = {
	&platformCreateDevice_unsup,
	&deviceFunc1_unsup,
	&deviceFunc2_unsup,
	&deviceDestroy_unsup
};

/**
 * Global layer linked list element.
 */
struct layer_s;
struct layer_s {
	// dispatch table of the layer
	struct dispatch_s  dispatch;
	// next layer in the chain
	struct layer_s    *next;
	void              *library;
	pfn_layerDeinit_t  layerDeinit;
};

/**
 * Instance layer linked list element.
 */
struct instance_layer_s;
struct instance_layer_s {
	struct instance_dispatch_s dispatch;
#if !FFI_INSTANCE_LAYERS
	struct layer_dispatch_s    layer_dispatch;
#endif
	void                      *data;
	struct instance_layer_s   *next;
	void                      *library;
	pfn_layerInstanceDeinit_t  layerInstanceDeinit;
};

/**
 * Terminators for driver provided API entry points.  These will effectively
 * dispatch to the correct driver after the instance layer chain.
 */
static inline int platformCreateDevice_term(platform_t platform, device_t *device_ret);
static inline int deviceFunc1_term(device_t device, int param);
static inline int deviceFunc2_term(device_t device, int param);
static inline int deviceDestroy_term(device_t device);

#if FFI_INSTANCE_LAYERS
/* The terminator for ffi instance layer */
static struct instance_layer_s _instance_layer_terminator = {
	{
		&platformCreateDevice_term,
		&deviceFunc1_term,
		&deviceFunc2_term,
		&deviceDestroy_term
	},
	NULL,
	NULL,
	NULL,
	NULL
};
#else
static int platformCreateDevice_inst(struct instance_layer_s *layer, platform_t platform, device_t *device_ret);
static int deviceFunc1_inst(struct instance_layer_s *layer, device_t device, int param);
static int deviceFunc2_inst(struct instance_layer_s *layer, device_t device, int param);
static int deviceDestroy_inst(struct instance_layer_s *layer, device_t device);

/* The terminator for non FFI instance layer */
static struct instance_layer_s _instance_layer_terminator = {
	{
		(pfn_platformCreateDevice_instance_t)&platformCreateDevice_inst,
		(pfn_deviceFunc1_instance_t)&deviceFunc1_inst,
		(pfn_deviceFunc2_instance_t)&deviceFunc2_inst,
		(pfn_deviceDestroy_instance_t)&deviceDestroy_inst
	},
	{ NULL, NULL, NULL, NULL },
	NULL,
	NULL,
	NULL,
	NULL
};

/* The initializer for the layer dispatch */
static struct layer_dispatch_s _instance_layer_dispatch_head = {
	(struct instance_layer_proxy_s *)&_instance_layer_terminator,
	(struct instance_layer_proxy_s *)&_instance_layer_terminator,
	(struct instance_layer_proxy_s *)&_instance_layer_terminator,
	(struct instance_layer_proxy_s *)&_instance_layer_terminator
};
#endif

/**
 * Every opaque handle from the API will be set to point to the multiplex_s
 * structure. This structure will contain the object dispatch table, as well as
 * a pointer to the start of the instance layer chain.
 * For non FFI instance layers, it will also contain the first layer dispatch
 * indirection table.
 */
struct multiplex_s {
	struct driver_dispatch_s  dispatch;
	struct instance_layer_s  *first_layer;
#if !FFI_INSTANCE_LAYERS
	struct layer_dispatch_s   layer_dispatch;
#endif
};

/**
 * Platform linked list element. Object created from platform and their descendants will reference
 * the platform's multiplexing structure.
 */
struct plt_s;
struct plt_s {
	platform_t          platform;
	struct multiplex_s  multiplex;
	struct plt_s       *next;
};

/**
 * Definition of driver entry points.
 */
typedef int (*pfn_getPaltfomsExt_t)(size_t num_platform, platform_t *platforms, size_t *num_platform_ret);
typedef void * (*pfn_platformGetFuncExt_t)(platform_t platform, const char *name);

/**
 * Driver linked list element.
 */
struct driver_s;
struct driver_s {
	void                     *library;
	pfn_getPaltfomsExt_t      getPaltfomsExt;
	pfn_platformGetFuncExt_t  platformGetFuncExt;
	size_t                    num_platforms;
	platform_t               *platforms;
	struct driver_s          *next;
};

/**
 * Global layer terminator.
 */
static struct layer_s _layer_terminator = {
	{
		&getPlatforms_disp,
		&platformAddLayer_disp,
		&platformCreateDevice_disp,
		&deviceFunc1_disp,
		&deviceFunc2_disp,
		&deviceDestroy_disp
	},
	NULL,
	NULL,
	NULL
};

/**
 * Linked lists entry points.
 */
static struct layer_s  *_first_layer = &_layer_terminator;
static struct driver_s *_first_driver = NULL;
static struct plt_s    *_first_platform = NULL;
static size_t           _num_platforms = 0;

/**
 * (Opaque) will be made to point to platform multiplexing structure.
 */
struct platform_s {
	struct multiplex_s *multiplex;
};

struct device_s {
	struct multiplex_s *multiplex;
};

static pthread_once_t initialized = PTHREAD_ONCE_INIT;

static void *
loadLibrary(const char *libraryName) {
	return dlopen(libraryName, RTLD_LAZY|RTLD_LOCAL);
}

char *get_next(char *paths) {
	char *next;
	next = strchr(paths, ':');
	if (next) {
		*next = '\0';
		next++;
	}
	return next;
}

#define SET_API(api) do { \
	plt->multiplex.dispatch.api = (pfn_ ## api ## _t)(intptr_t)pfn; \
} while (0)

#define GET_API(api) do { \
	void *pfn = driver->platformGetFuncExt(platform, #api); \
	if (pfn) \
		SET_API(api); \
} while (0)

/**
 * Load platforms from a driver, and insert them into the platform list.
 */
static void
loadPlatforms(struct driver_s *driver) {
	for (size_t i = 0; i < driver->num_platforms; i++) {
		struct plt_s *plt = (struct plt_s *)
			calloc(1, sizeof(struct plt_s));
		platform_t platform = driver->platforms[i];
		plt->platform = platform;
		/* Initialize dispatch table and instance layer chains */
		plt->multiplex.dispatch = _unsup_dispatch;
		plt->multiplex.first_layer = &_instance_layer_terminator;
#if !FFI_INSTANCE_LAYERS
		plt->multiplex.layer_dispatch = _instance_layer_dispatch_head;
#endif
		/* fill dispatch table */
		GET_API(platformCreateDevice);
		GET_API(deviceFunc1);
		GET_API(deviceFunc2);
		GET_API(deviceDestroy);
		/* setup multiplex reference */
		plt->platform->multiplex = &plt->multiplex;
		/* Insert platform into platform list */
		plt->next = _first_platform;
		_first_platform = plt;
		_num_platforms++;
	}
}

/**
 * Load a driver given it's library path, checking driver provide the two apis
 * defined in driver-spec.h, and that getPaltfomsExt does indeed return a
 * platform.
 */
static void
loadDriver(const char *path) {
	struct driver_s *driver = NULL;
	size_t num_platforms;
	void *lib = loadLibrary(path);
	if (!lib)
		return;
	pfn_getPaltfomsExt_t p_getPaltfomsExt = (pfn_getPaltfomsExt_t)(intptr_t)dlsym(lib, "getPlatformsExt");
	if (!p_getPaltfomsExt)
		goto error;
	pfn_platformGetFuncExt_t p_platformGetFuncExt = (pfn_platformGetFuncExt_t)(intptr_t)dlsym(lib, "platformGetFuncExt");
	if (!p_platformGetFuncExt)
		goto error;
	if (p_getPaltfomsExt(0, NULL, &num_platforms) || !num_platforms)
		goto error;
	driver = (struct driver_s *)calloc(1, sizeof(struct driver_s) + num_platforms * sizeof(platform_t));
	driver->library = lib;
	driver->getPaltfomsExt = p_getPaltfomsExt;
	driver->platformGetFuncExt = p_platformGetFuncExt;
	driver->num_platforms = num_platforms;
	driver->platforms = (platform_t *)((intptr_t)driver + sizeof(struct platform_s));
	if (p_getPaltfomsExt(num_platforms, driver->platforms, NULL))
		goto error;
	loadPlatforms(driver);
	driver->next = _first_driver;
	_first_driver = driver;
	return;
error:
	if (driver)
		free(driver);
	dlclose(lib);
}

/**
 * Load a global layer library given its path, and try to initialize it. If
 * successful insert it into the global layer list.
 */
static void
loadLayer(const char *path) {
	struct layer_s *layer = NULL;
	void *lib = loadLibrary(path);
	if (!lib)
		return;
	pfn_layerInit_t p_layerInit = (pfn_layerInit_t)(intptr_t)dlsym(lib, "layerInit");
	if (!p_layerInit)
		goto error;
	layer = (struct layer_s *)calloc(1, sizeof(struct layer_s));
	layer->library = lib;
	if (p_layerInit(NUM_DISPATCH_ENTRIES, &_first_layer->dispatch, &layer->dispatch))
		goto error;
	for (size_t i = 0; i < NUM_DISPATCH_ENTRIES; i++)
		if (!((void **)&(layer->dispatch))[i])
			((void **)&(layer->dispatch))[i] = ((void **)&(_first_layer->dispatch))[i];
	layer->next = _first_layer;
	layer->layerDeinit = (pfn_layerDeinit_t)(intptr_t)dlsym(lib, "layerDeinit");
	_first_layer = layer;
	return;
error:
	if (layer)
		free(layer);
	dlclose(lib);
}

/**
 * Load an instance layer library into a multiplexing structure, inserting it
 * in front of the instance layer chain.
 */
static int
loadInstanceLayer(struct multiplex_s *multiplex, const char *path) {
	struct instance_layer_s *layer = NULL;
	void *lib = loadLibrary(path);
	if (!lib)
		goto error;
	pfn_layerInstanceInit_t p_layerInstanceInit =
		(pfn_layerInstanceInit_t)(intptr_t)dlsym(lib, "layerInstanceInit");
	if (!p_layerInstanceInit)
		goto error;
	pfn_layerInstanceDeinit_t p_layerInstanceDeinit =
		(pfn_layerInstanceDeinit_t)(intptr_t)dlsym(lib, "layerInstanceDeinit");
	if (!p_layerInstanceDeinit)
		goto error;
	layer = (struct instance_layer_s *)calloc(1, sizeof(struct instance_layer_s));
	layer->library = lib;
	layer->layerInstanceDeinit = p_layerInstanceDeinit;
	int res;
	const size_t num_entries = NUM_INSTANCE_DISPATCH_ENTRIES;
#if FFI_INSTANCE_LAYERS
	res = p_layerInstanceInit(num_entries, &multiplex->first_layer->dispatch, &layer->dispatch, &layer->data);
#else
	res = p_layerInstanceInit(num_entries, &layer->dispatch, &layer->data);
#endif
	if (res)
		goto error;
#if FFI_INSTANCE_LAYERS
	/**
	 * FFI instance layer's dispatch tables are completed so that the next
 	 * layer can benefit from a full table
	 */
	for (size_t i = 0; i < num_entries; i++)
		if (!((void **)&(layer->dispatch))[i])
			((void **)&(layer->dispatch))[i] =
				((void **)&(multiplex->first_layer->dispatch))[i];
#else
	/**
	 * Non FFI instance layer need to copy then update the layer_dispatch
	 * table with the entries they provide.  This allows the layer chain to
	 * provide the correct context to each layer.
	 */
	layer->layer_dispatch = multiplex->layer_dispatch;
	for (size_t i = 0; i < num_entries; i++)
		if (((void **)&(layer->dispatch))[i])
			((struct instance_layer_s **)&(multiplex->layer_dispatch))[i] = layer;
#endif
	layer->next = multiplex->first_layer;
	multiplex->first_layer = layer;
	return SPEC_SUCCESS;
error:
	if (layer)
		free(layer);
	dlclose(lib);
	return SPEC_ERROR;
}

/**
 * Load drivers and global layers, both lists provided in colon separated list
 * given by environment variables.
 */
static void
initReal() {
	char *drivers = getenv("DRIVERS");
	if (drivers) {
		char *next_file = drivers;
		while (NULL != next_file && *next_file != '\0') {
			char *cur_file = next_file;
			next_file = get_next(cur_file);
			loadDriver(cur_file);
		}
	}
	char *layers = getenv("LAYERS");
	if (layers) {
		char *next_file = layers;
		while (NULL != next_file && *next_file != '\0') {
			char *cur_file = next_file;
			next_file = get_next(cur_file);
			loadLayer(cur_file);
		}
	}
}

static void
initOnce(void) {
	pthread_once(&initialized, initReal);
}

/**
 * API entry points
 */
int
getPlatforms(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret) {
	initOnce();
	return _first_layer->dispatch.getPlatforms(num_platforms, platforms, num_platforms_ret);
}

int
platformAddLayer(platform_t platform, const char *layer_name) {
	return _first_layer->dispatch.platformAddLayer(platform, layer_name);
}

int
platformCreateDevice(platform_t platform, device_t *device_ret) {
	return _first_layer->dispatch.platformCreateDevice(platform, device_ret);
}

int deviceFunc1(device_t device, int param) {
	return _first_layer->dispatch.deviceFunc1(device, param);
}

int
deviceFunc2(device_t device, int param) {
	return _first_layer->dispatch.deviceFunc2(device, param);
}

int
deviceDestroy(device_t device) {
	return _first_layer->dispatch.deviceDestroy(device);
}

/**
 * Global layer terminators.
 */

/**
 * The first two are loader implemented APIs and don't call into drivers.
 */
static int
getPlatforms_disp(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret) {
	if (num_platforms_ret)
		*num_platforms_ret = _num_platforms;
	if (num_platforms && platforms) {
		if (num_platforms < _num_platforms)
			return SPEC_ERROR;
		struct plt_s *plt = _first_platform;
		size_t i = 0;
		while (plt) {
			platforms[i] = plt->platform;
			plt = plt->next;
			i++;
		}
		while (i < num_platforms)
			platforms[i] = NULL;
	}
	return SPEC_SUCCESS;
}

static int
platformAddLayer_disp(platform_t platform, const char *layer_name) {
	return loadInstanceLayer(platform->multiplex, layer_name);
}

/**
 * For driver implemented APIs, the global terminator call into the instance
 * layer chain.
 */

#if FFI_INSTANCE_LAYERS
#define NEXT_LAYER(handle, api) (handle->multiplex->first_layer)
#define NEXT_ENTRY(handle, api) NEXT_LAYER(handle, api)->dispatch.api ## _instance
#define CALL_FIRST_LAYER(handle, api, ...) NEXT_ENTRY(handle, api)(__VA_ARGS__)
#else
#define NEXT_LAYER(handle, api) (handle->multiplex->layer_dispatch.api ## _next)
#define NEXT_ENTRY(handle, api) NEXT_LAYER(handle, api)->dispatch.api ## _instance
#define CALL_FIRST_LAYER(handle, api, ...) NEXT_ENTRY(handle, api)(NEXT_LAYER(handle, api), __VA_ARGS__)
#endif

static int
platformCreateDevice_disp(platform_t platform, device_t *device_ret) {
	if (!platform)
		return SPEC_ERROR;
	return CALL_FIRST_LAYER(platform, platformCreateDevice, platform, device_ret);
}

static int
deviceFunc1_disp(device_t device, int param) {
	if (!device)
		return SPEC_ERROR;
	return CALL_FIRST_LAYER(device, deviceFunc1, device, param);
}

static int
deviceFunc2_disp(device_t device, int param) {
	if (!device)
		return SPEC_ERROR;
	return CALL_FIRST_LAYER(device, deviceFunc2, device, param);
}

static int
deviceDestroy_disp(device_t device) {
	if (!device)
		return SPEC_ERROR;
	return CALL_FIRST_LAYER(device, deviceDestroy, device);
}

/**
 * Unsupported API stubs.
 */
static int
platformCreateDevice_unsup(platform_t platform, device_t *device_ret) {
	(void)platform;
	(void)device_ret;
	return SPEC_UNSUPPORTED;
}

static int
deviceFunc1_unsup(device_t device, int param) {
	(void)device;
	(void)param;
	return  SPEC_UNSUPPORTED;
}

static int
deviceFunc2_unsup(device_t device, int param) {
	(void)device;
	(void)param;
	return SPEC_UNSUPPORTED;
}

static int
deviceDestroy_unsup(device_t device) {
	(void)device;
	return SPEC_UNSUPPORTED;
}

/**
 * Instance layer terminators either directly (for FFI layers) or indirectly
 * (for non-FFI layers).
 */
static inline int
platformCreateDevice_term(platform_t platform, device_t *device_ret) {
	int result = platform->multiplex->dispatch.platformCreateDevice(platform, device_ret);
	/* Devices inherit from the platfom multiplex structure reference */
	if (result == SPEC_SUCCESS)
		(*device_ret)->multiplex = platform->multiplex;
	return result;
}

static inline int
deviceFunc1_term(device_t device, int param) {
	return device->multiplex->dispatch.deviceFunc1(device, param);
}

static inline int
deviceFunc2_term(device_t device, int param) {
	return device->multiplex->dispatch.deviceFunc2(device, param);
}

static inline int
deviceDestroy_term(device_t device) {
	return device->multiplex->dispatch.deviceDestroy(device);
}

/**
 * Non ffi instance layer terminators.
 */
#if !FFI_INSTANCE_LAYERS
static int platformCreateDevice_inst(struct instance_layer_s *layer, platform_t platform, device_t *device_ret) {
	(void)layer;
	return platformCreateDevice_term(platform, device_ret);
}

static int deviceFunc1_inst(struct instance_layer_s *layer, device_t device, int param) {
	(void)layer;
	return deviceFunc1_term(device, param);
}

static int deviceFunc2_inst(struct instance_layer_s *layer, device_t device, int param) {
	(void)layer;
	return deviceFunc2_term(device, param);
}

static int deviceDestroy_inst(struct instance_layer_s *layer, device_t device) {
	(void)layer;
	return deviceDestroy_term(device);
}
#endif

/**
 * Loader cleanup. If called explicitly at program termination, valgrind should
 * report no leak. as is in a destructor, valgrind reports leaks.
 */
__attribute__((destructor))
void my_fini(void) {
	printf("Deiniting loader\n");
	struct plt_s *platform = _first_platform;
	while(platform) {
		struct plt_s *next_platform = platform->next;
		struct instance_layer_s *layer = platform->multiplex.first_layer;
		while(layer->library) {
			struct instance_layer_s *next_layer = layer->next;
			layer->layerInstanceDeinit(layer->data);
			dlclose(layer->library);
			free(layer);
			layer = next_layer;
		}
		free(platform);
		platform = next_platform;
	}
	struct driver_s *driver = _first_driver;
	while(driver) {
		struct driver_s *next_driver = driver->next;
		dlclose(driver->library);
		free(driver);
		driver = next_driver;
	}
	struct layer_s *layer = _first_layer;
	while(layer != &_layer_terminator) {
		struct layer_s *next_layer = layer->next;
		if (layer->layerDeinit)
			layer->layerDeinit();
		dlclose(layer->library);
		free(layer);
		layer = next_layer;
	}
}
