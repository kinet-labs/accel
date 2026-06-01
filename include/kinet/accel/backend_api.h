// kinet/accel/backend_api.h - Backend Plugin API
//
// This header defines the interface between kinet-accel and backend plugins.
// Plugins (kinet-metal, kinet-webgpu, kinet-cuda) implement this interface.
//
// Pattern: kinet-accel exports kinet_backend_api_v1() which returns a function table.
// Plugins call these functions to interact with the core.

#ifndef KINET_BACKEND_API_H
#define KINET_BACKEND_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// API version
// =============================================================================

#define KINET_BACKEND_API_VERSION 1

// =============================================================================
// Symbol visibility for plugins
// =============================================================================

#if defined(_WIN32) || defined(__CYGWIN__)
  #define KINET_PLUGIN_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
  #define KINET_PLUGIN_EXPORT __attribute__((visibility("default")))
#else
  #define KINET_PLUGIN_EXPORT
#endif

// =============================================================================
// Backend type enum (matches c_api.h)
// =============================================================================

typedef enum {
    KINET_BACKEND_TYPE_AUTO = 0,
    KINET_BACKEND_TYPE_METAL = 1,
    KINET_BACKEND_TYPE_WEBGPU = 2,
    KINET_BACKEND_TYPE_CUDA = 3,
} kinet_backend_type_t;

// =============================================================================
// Opaque handles for plugin use
// =============================================================================

typedef void* kinet_core_buffer;
typedef void* kinet_core_device;
typedef void* kinet_core_queue;

// =============================================================================
// Device capabilities reported by backends
// =============================================================================

typedef struct {
    const char* name;
    const char* vendor;
    int is_discrete;
    int is_unified_memory;
    uint64_t total_memory;
    uint64_t max_buffer_size;
    uint32_t max_workgroup_size;
    uint32_t simd_width;
    int supports_fp16;
    int supports_subgroups;
} kinet_device_caps_t;

// =============================================================================
// Buffer description
// =============================================================================

typedef struct {
    size_t size;
    uint32_t usage;  // Bitmask of usage flags
    void* native_handle;  // Backend-specific handle
} kinet_buffer_desc_t;

// Usage flags
#define KINET_BUFFER_USAGE_STORAGE    (1 << 0)
#define KINET_BUFFER_USAGE_UNIFORM    (1 << 1)
#define KINET_BUFFER_USAGE_MAP_READ   (1 << 2)
#define KINET_BUFFER_USAGE_MAP_WRITE  (1 << 3)
#define KINET_BUFFER_USAGE_COPY_SRC   (1 << 4)
#define KINET_BUFFER_USAGE_COPY_DST   (1 << 5)

// =============================================================================
// Dispatch description
// =============================================================================

typedef struct {
    uint32_t grid_x;
    uint32_t grid_y;
    uint32_t grid_z;
    uint32_t block_x;
    uint32_t block_y;
    uint32_t block_z;
} kinet_dispatch_desc_t;

// =============================================================================
// Backend interface (implemented by plugins)
// =============================================================================

typedef struct kinet_backend_interface {
    // Backend identification
    kinet_backend_type_t type;
    const char* name;
    int api_version;

    // Lifecycle
    int (*is_available)(void);
    int (*init)(void);
    void (*shutdown)(void);

    // Device enumeration
    int (*get_device_count)(void);
    int (*get_device_caps)(int index, kinet_device_caps_t* caps);

    // Device management
    void* (*create_device)(int index);
    void (*destroy_device)(void* device);

    // Queue management
    void* (*create_queue)(void* device);
    void (*destroy_queue)(void* queue);
    int (*queue_submit)(void* queue);
    int (*queue_wait)(void* queue);

    // Buffer management
    void* (*create_buffer)(void* device, const kinet_buffer_desc_t* desc);
    void* (*create_buffer_with_data)(void* device, const kinet_buffer_desc_t* desc,
                                      const void* data);
    void (*destroy_buffer)(void* buffer);
    void* (*map_buffer)(void* buffer);
    void (*unmap_buffer)(void* buffer);

    // Kernel management (from pre-compiled bundles or source)
    void* (*create_kernel_from_source)(void* device, const char* source,
                                        const char* entry_point);
    void* (*create_kernel_from_bundle)(void* device, const void* bundle_data,
                                        size_t bundle_size, const char* entry_point);
    void (*destroy_kernel)(void* kernel);
    void (*kernel_set_buffer)(void* kernel, uint32_t index, void* buffer, size_t offset);
    void (*kernel_set_bytes)(void* kernel, uint32_t index, const void* data, size_t size);
    void (*kernel_set_workgroup_size)(void* kernel, uint32_t x, uint32_t y, uint32_t z);

    // Dispatch
    int (*dispatch)(void* queue, void* kernel, const kinet_dispatch_desc_t* desc);

    // Memory operations
    int (*copy_buffer)(void* queue, void* src, size_t src_offset,
                       void* dst, size_t dst_offset, size_t size);
    int (*fill_buffer)(void* queue, void* buffer, size_t offset, size_t size, uint8_t value);

} kinet_backend_interface_t;

// =============================================================================
// Core API provided by kinet-accel to plugins
// =============================================================================

typedef struct kinet_core_api {
    int api_version;

    // Logging
    void (*log_debug)(const char* msg);
    void (*log_info)(const char* msg);
    void (*log_warn)(const char* msg);
    void (*log_error)(const char* msg);

    // Memory allocation (for backend internal use)
    void* (*alloc)(size_t size);
    void (*free)(void* ptr);

    // Kernel registry access
    const void* (*get_kernel_bundle)(const char* name, size_t* size);
    const char* (*get_kernel_source)(const char* name);

} kinet_core_api_t;

// =============================================================================
// Plugin entry point
// =============================================================================

// Every backend plugin must export this function
// It receives the core API and returns the backend interface
typedef kinet_backend_interface_t* (*kinet_backend_init_fn)(const kinet_core_api_t* core_api);

// Plugin entry point name (must be exported by all plugins)
#define KINET_BACKEND_INIT_SYMBOL "kinet_backend_init"

// =============================================================================
// Helper macro for plugin implementation
// =============================================================================

#define KINET_BACKEND_PLUGIN(backend_type, backend_name) \
    static kinet_backend_interface_t backend_interface = { \
        .type = backend_type, \
        .name = backend_name, \
        .api_version = KINET_BACKEND_API_VERSION, \
    }; \
    KINET_PLUGIN_EXPORT kinet_backend_interface_t* kinet_backend_init(const kinet_core_api_t* core_api)

#ifdef __cplusplus
}
#endif

#endif // KINET_BACKEND_API_H
