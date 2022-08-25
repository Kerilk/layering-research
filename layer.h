#define NUM_DISPATCH_ENTRIES (sizeof(struct dispatch_s)/sizeof(pfn_layerInit_t))

typedef int layerInit_t(
	size_t              num_entries,
	struct dispatch_s  *target_dispatch,
	size_t             *num_entries_ret,
	struct dispatch_s **layer_dispatch_ret);

typedef layerInit_t *pfn_layerInit_t;

typedef int layerInstanceInit_t(
	size_t              num_entries,
	struct dispatch_s  *target_dispatch,
	struct dispatch_s  *layer_instance_dispatch,
	void              **layer_data_ret);

typedef layerInstanceInit_t *pfn_layerInstanceInit_t;

typedef int layerInstanceDeinit_t(
	void *layer_data);

typedef layerInstanceDeinit_t *pfn_layerInstanceDeinit_t;
