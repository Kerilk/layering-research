#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include "spec.h"
#include "dispatch.h"
#include "layer.h"

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

static int
getPlatforms_unsup(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret);
static int
platformAddLayer_unsup(platform_t platform, const char *layer_name);
static int
platformCreateDevice_unsup(platform_t platform, device_t *device_ret);
static int
deviceFunc1_unsup(device_t device, int param);
static int
deviceFunc2_unsup(device_t device, int param);
static int
deviceDestroy_unsup(device_t device);

static struct dispatch_s _unsup_dispatch = {
	&getPlatforms_unsup,
	&platformAddLayer_unsup,
	&platformCreateDevice_unsup,
	&deviceFunc1_unsup,
	&deviceFunc2_unsup,
	&deviceDestroy_unsup
};

struct layer_s;
struct layer_s {
	struct dispatch_s  dispatch;
	struct layer_s    *next;
	void              *library;
	pfn_layerDeinit_t  layerDeinit;
};

struct instance_layer_s;
struct instance_layer_s {
#if FFI_INSTANCE_LAYERS
	struct dispatch_s          dispatch;
#else
	struct instance_dispatch_s dispatch;
	struct layer_dispatch_s    layer_dispatch;
#endif
	void                      *data;
	struct instance_layer_s   *next;
	void                      *library;
	pfn_layerInstanceDeinit_t  layerInstanceDeinit;
};

#if !FFI_INSTANCE_LAYERS
static int platformCreateDevice_inst(struct instance_layer_s *layer, platform_t platform, device_t *device_ret);
static int deviceFunc1_inst(struct instance_layer_s *layer, device_t device, int param);
static int deviceFunc2_inst(struct instance_layer_s *layer, device_t device, int param);
static int deviceDestroy_inst(struct instance_layer_s *layer, device_t device);

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
#endif

#if FFI_INSTANCE_LAYERS
struct multiplex_s {
	struct instance_layer_s *first_layer;
};
#else
struct multiplex_s {
	struct dispatch_s        dispatch;
	struct instance_layer_s *first_layer;
	struct layer_dispatch_s  layer_dispatch;
};
#endif

struct plt_s;
struct plt_s {
	platform_t          platform;
	struct multiplex_s  multiplex;
	struct plt_s       *next;
};

typedef int (*pfn_getPaltfomsExt_t)(size_t num_platform, platform_t *platforms, size_t *num_platform_ret);
typedef void * (*pfn_platformGetFunc_t)(platform_t platform, const char *name);

struct driver_s;
struct driver_s {
	void                  *library;
	pfn_getPaltfomsExt_t   getPaltfomsExt;
	pfn_platformGetFunc_t  platformGetFunc;
	size_t                 num_platforms;
	platform_t            *platforms;
	struct driver_s       *next;
};

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
static struct layer_s  *_first_layer = &_layer_terminator;
static struct plt_s    *_first_platform = NULL;
static size_t           _num_platforms = 0;
static struct driver_s *_first_driver = NULL;

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

#if FFI_INSTANCE_LAYERS
#define SET_API(api) do { \
	plt->multiplex.first_layer->dispatch.api = (pfn_ ## api ## _t)(intptr_t)pfn; \
} while (0)
#else
#define SET_API(api) do { \
	plt->multiplex.dispatch.api = (pfn_ ## api ## _t)(intptr_t)pfn; \
} while (0)
#endif

#define GET_API(api) do { \
	void *pfn = driver->platformGetFunc(platform, #api); \
	if (pfn) \
		SET_API(api); \
} while (0)

static void
loadPlatforms(struct driver_s *driver) {
	for (size_t i = 0; i < driver->num_platforms; i++) {
		struct plt_s *plt = (struct plt_s *)
			calloc(1, sizeof(struct plt_s) + sizeof(struct instance_layer_s));
		platform_t platform = driver->platforms[i];
		plt->platform = platform;
#if FFI_INSTANCE_LAYERS
		plt->multiplex.first_layer = (struct instance_layer_s*)
			((char *)plt + sizeof(struct plt_s));
		plt->multiplex.first_layer->dispatch = _unsup_dispatch;
#else
		plt->multiplex.first_layer = &_instance_layer_terminator;
		plt->multiplex.dispatch = _unsup_dispatch;
		plt->multiplex.layer_dispatch.platformCreateDevice_next =
			(struct instance_layer_proxy_s *)plt->multiplex.first_layer;
		plt->multiplex.layer_dispatch.deviceFunc1_next =
			(struct instance_layer_proxy_s *)plt->multiplex.first_layer;
		plt->multiplex.layer_dispatch.deviceFunc2_next =
			(struct instance_layer_proxy_s *)plt->multiplex.first_layer;
		plt->multiplex.layer_dispatch.deviceDestroy_next =
			(struct instance_layer_proxy_s *)plt->multiplex.first_layer;
#endif
		GET_API(platformCreateDevice);
		GET_API(deviceFunc1);
		GET_API(deviceFunc2);
		GET_API(deviceDestroy);
		plt->next = _first_platform;
		plt->platform->multiplex = &plt->multiplex;
		_first_platform = plt;
		_num_platforms++;
	}
}

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
	pfn_platformGetFunc_t p_platformGetFunc = (pfn_platformGetFunc_t)(intptr_t)dlsym(lib, "platformGetFunc");
	if (!p_platformGetFunc)
		goto error;
	if (p_getPaltfomsExt(0, NULL, &num_platforms) || !num_platforms)
		goto error;
	driver = (struct driver_s *)calloc(1, sizeof(struct driver_s) + num_platforms * sizeof(platform_t));
	driver->library = lib;
	driver->getPaltfomsExt = p_getPaltfomsExt;
	driver->platformGetFunc = p_platformGetFunc;
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
	size_t num_entries = 0;
	struct dispatch_s *p_dispatch = NULL;
	if (p_layerInit(NUM_DISPATCH_ENTRIES, &_first_layer->dispatch, &num_entries, &p_dispatch))
		goto error;
	size_t count = num_entries < NUM_DISPATCH_ENTRIES ? num_entries : NUM_DISPATCH_ENTRIES;
	for (size_t i = 0; i < count; i++) {
		((void **)&(layer->dispatch))[i] = ((void **)p_dispatch)[i] ? ((void **)p_dispatch)[i] : ((void **)&(_first_layer->dispatch))[i];
	}
	for (size_t i = count; i < NUM_DISPATCH_ENTRIES; i++) {
		((void **)&(layer->dispatch))[i] = ((void **)&(_first_layer->dispatch))[i];
	}
	layer->next = _first_layer;
	layer->layerDeinit = (pfn_layerDeinit_t)(intptr_t)dlsym(lib, "layerDeinit");
	_first_layer = layer;
	return;
error:
	if (layer)
		free(layer);
	dlclose(lib);
}

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
#if FFI_INSTANCE_LAYERS
	const size_t num_entries = NUM_DISPATCH_ENTRIES;
	res = p_layerInstanceInit(num_entries, &multiplex->first_layer->dispatch, &layer->dispatch, &layer->data);
#else
	const size_t num_entries = NUM_INSTANCE_DISPATCH_ENTRIES;
	res = p_layerInstanceInit(num_entries,  &layer->dispatch, &layer->data);
#endif
	if (res)
		goto error;
#if FFI_INSTANCE_LAYERS
	for (size_t i = 0; i < num_entries; i++)
		((void **)&(layer->dispatch))[i] =
			((void **)&(layer->dispatch))[i] ?
				((void **)&(layer->dispatch))[i] :
				((void **)&(multiplex->first_layer->dispatch))[i];
#else
	layer->layer_dispatch = multiplex->layer_dispatch;
	for (size_t i = 0; i < num_entries; i++)
		if (((void **)&(layer->dispatch))[i])
			((struct instance_layer_s **)&(multiplex->layer_dispatch))[i] =
				layer;
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

int
getPlatforms(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret) {
	return _first_layer->dispatch.getPlatforms(num_platforms, platforms, num_platforms_ret);
}

static int
getPlatforms_disp(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret) {
	initOnce();
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
getPlatforms_unsup(size_t num_platforms, platform_t *platforms, size_t *num_platforms_ret) {
	(void)num_platforms;
	(void)platforms;
	(void)num_platforms_ret;
	return SPEC_UNSUPPORTED;
}

int
platformAddLayer(platform_t platform, const char *layer_name) {
	return _first_layer->dispatch.platformAddLayer(platform, layer_name);
}

static int
platformAddLayer_disp(platform_t platform, const char *layer_name) {
	return loadInstanceLayer(platform->multiplex, layer_name);
}

static int
platformAddLayer_unsup(platform_t platform, const char *layer_name) {
	(void)platform;
	(void)layer_name;
	return SPEC_UNSUPPORTED;
}

#if FFI_INSTANCE_LAYERS
#define NEXT_LAYER(handle, api) (handle->multiplex->first_layer)
#define NEXT_ENTRY(handle, api) NEXT_LAYER(handle, api)->dispatch.api
#define CALL_FIRST_LAYER(handle, api, ...) NEXT_ENTRY(handle, api)(__VA_ARGS__)
#else
#define NEXT_LAYER(handle, api) (handle->multiplex->layer_dispatch.api ## _next)
#define NEXT_ENTRY(handle, api) NEXT_LAYER(handle, api)->dispatch.api ## _instance
#define CALL_FIRST_LAYER(handle, api, ...) NEXT_ENTRY(handle, api)(NEXT_LAYER(handle, api), __VA_ARGS__)
#endif

int
platformCreateDevice(platform_t platform, device_t *device_ret) {
	return _first_layer->dispatch.platformCreateDevice(platform, device_ret);
}

static int
platformCreateDevice_disp(platform_t platform, device_t *device_ret) {
	if (!platform)
		return SPEC_ERROR;
	int result = CALL_FIRST_LAYER(platform, platformCreateDevice, platform, device_ret);
	if (result == SPEC_SUCCESS)
		(*device_ret)->multiplex = platform->multiplex;
	return result;
}

static int
platformCreateDevice_unsup(platform_t platform, device_t *device_ret) {
	(void)platform;
	(void)device_ret;
	return SPEC_UNSUPPORTED;
}

int deviceFunc1(device_t device, int param) {
	return _first_layer->dispatch.deviceFunc1(device, param);
}

static int
deviceFunc1_disp(device_t device, int param) {
	if (!device)
		return SPEC_ERROR;
	return CALL_FIRST_LAYER(device, deviceFunc1, device, param);
}

static int
deviceFunc1_unsup(device_t device, int param) {
	(void)device;
	(void)param;
	return  SPEC_UNSUPPORTED;
}

int
deviceFunc2(device_t device, int param) {
	return _first_layer->dispatch.deviceFunc2(device, param);
}

static int
deviceFunc2_disp(device_t device, int param) {
	if (!device)
		return SPEC_ERROR;
	return CALL_FIRST_LAYER(device, deviceFunc2, device, param);
}

static int
deviceFunc2_unsup(device_t device, int param) {
	(void)device;
	(void)param;
	return SPEC_UNSUPPORTED;
}

int
deviceDestroy(device_t device) {
	return _first_layer->dispatch.deviceDestroy(device);
}

static int
deviceDestroy_disp(device_t device) {
	if (!device)
		return SPEC_ERROR;
	return CALL_FIRST_LAYER(device, deviceDestroy, device);
}

static int
deviceDestroy_unsup(device_t device) {
	(void)device;
	return SPEC_UNSUPPORTED;
}

#if !FFI_INSTANCE_LAYERS
static int platformCreateDevice_inst(struct instance_layer_s *layer, platform_t platform, device_t *device_ret) {
	(void)layer;
	return platform->multiplex->dispatch.platformCreateDevice(platform, device_ret);
}

static int deviceFunc1_inst(struct instance_layer_s *layer, device_t device, int param) {
	(void)layer;
	return device->multiplex->dispatch.deviceFunc1(device, param);
}

static int deviceFunc2_inst(struct instance_layer_s *layer, device_t device, int param) {
	(void)layer;
	return device->multiplex->dispatch.deviceFunc2(device, param);
}

static int deviceDestroy_inst(struct instance_layer_s *layer, device_t device) {
	(void)layer;
	return device->multiplex->dispatch.deviceDestroy(device);
}
#endif

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
