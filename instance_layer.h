#if FFI_INSTANCE_LAYERS
#include <ffi.h>

enum _exp_layer_func_nargs {
	platformCreateDevice_ffi_nargs = 2,
	deviceFunc1_ffi_nargs          = 2,
	deviceFunc2_ffi_nargs          = 2,
	deviceDestroy_ffi_nargs        = 1,
};

struct platformCreateDevice_ffi_args {
	platform_t  *p_platform;
	device_t   **p_device_ret;
};
static __attribute__((unused))
ffi_type *platformCreateDevice_ffi_types[platformCreateDevice_ffi_nargs] = {
	&ffi_type_pointer,
	&ffi_type_pointer
};
static __attribute__((unused))
ffi_type *platformCreateDevice_ffi_ret = &ffi_type_sint;
typedef void platformCreateDevice_ffi_t(
	ffi_cif                              *cif,
	int                                  *ffi_ret,
	struct platformCreateDevice_ffi_args *args,
	void                                 *data);

struct deviceFunc1_ffi_args {
	device_t *p_device;
	int      *p_param;
};
static __attribute__((unused))
ffi_type *deviceFunc1_ffi_types[deviceFunc1_ffi_nargs] = {
	&ffi_type_pointer,
	&ffi_type_sint
};
static __attribute__((unused))
ffi_type *deviceFunc1_ffi_ret = &ffi_type_sint;
typedef void deviceFunc1_ffi_t(
	ffi_cif                     *cif,
	int                         *ffi_ret,
	struct deviceFunc1_ffi_args *args,
	void                        *data);

struct deviceFunc2_ffi_args {
	device_t *p_device;
	int      *p_param;
};
static __attribute__((unused))
ffi_type *deviceFunc2_ffi_types[deviceFunc2_ffi_nargs] = {
	&ffi_type_pointer,
	&ffi_type_sint
};
static __attribute__((unused))
ffi_type *deviceFunc2_ffi_ret = &ffi_type_sint;
typedef void deviceFunc2_ffi_t(
	ffi_cif                     *cif,
	int                         *ffi_ret,
	struct deviceFunc2_ffi_args *args,
	void                        *data);

struct deviceDestroy_ffi_args {
	device_t *p_device;
};
static __attribute__((unused))
ffi_type *deviceDestroy_ffi_types[deviceDestroy_ffi_nargs] = {
	&ffi_type_pointer
};
static __attribute__((unused))
ffi_type *deviceDestroy_ffi_ret = &ffi_type_sint;
typedef void deviceDestroy_ffi_t(
	ffi_cif                       *cif,
	int                           *ffi_ret,
	struct deviceDestroy_ffi_args *args,
	void                          *data);

#endif //FFI_INSTANCE_LAYERS
