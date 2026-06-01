#define KINET_ACCEL_BUILDING
#include <kinet/accel/c_api.h>
#include <kinet/accel/backend_api.h>
#include <kinet/accel/accel.hpp>

#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

// Thread-local error message
thread_local std::string g_last_error;

// Global state
std::mutex g_mutex;
bool g_initialized = false;
std::vector<void*> g_loaded_plugins;
std::vector<kinet_backend_interface_t*> g_backends;

// Core API provided to plugins
kinet_core_api_t g_core_api;

void set_error(const char* msg) {
    g_last_error = msg ? msg : "Unknown error";
}

kinet_status to_status(const kinet::gpu::Status& status) {
    if (status.ok()) return KINET_OK;
    switch (status.code) {
        case kinet::gpu::StatusCode::OutOfMemory: return KINET_OUT_OF_MEMORY;
        case kinet::gpu::StatusCode::InvalidArgument: return KINET_INVALID_ARGUMENT;
        case kinet::gpu::StatusCode::NotSupported: return KINET_NOT_SUPPORTED;
        case kinet::gpu::StatusCode::BackendNotAvailable: return KINET_NO_BACKEND;
        case kinet::gpu::StatusCode::KernelCompilationFailed:
        case kinet::gpu::StatusCode::KernelNotFound: return KINET_KERNEL_ERROR;
        case kinet::gpu::StatusCode::DispatchFailed: return KINET_DISPATCH_FAILED;
        default: return KINET_ERROR;
    }
}

// Core API implementations
void core_log_debug(const char* msg) { /* TODO: proper logging */ (void)msg; }
void core_log_info(const char* msg) { /* TODO: proper logging */ (void)msg; }
void core_log_warn(const char* msg) { /* TODO: proper logging */ (void)msg; }
void core_log_error(const char* msg) { set_error(msg); }
void* core_alloc(size_t size) { return std::malloc(size); }
void core_free(void* ptr) { std::free(ptr); }
const void* core_get_kernel_bundle(const char* name, size_t* size) {
    (void)name; (void)size;
    return nullptr; // TODO: implement kernel bundle registry
}
const char* core_get_kernel_source(const char* name) {
    (void)name;
    return nullptr; // TODO: implement kernel source registry
}

void init_core_api() {
    g_core_api.api_version = KINET_BACKEND_API_VERSION;
    g_core_api.log_debug = core_log_debug;
    g_core_api.log_info = core_log_info;
    g_core_api.log_warn = core_log_warn;
    g_core_api.log_error = core_log_error;
    g_core_api.alloc = core_alloc;
    g_core_api.free = core_free;
    g_core_api.get_kernel_bundle = core_get_kernel_bundle;
    g_core_api.get_kernel_source = core_get_kernel_source;
}

} // anonymous namespace

// =============================================================================
// Internal session wrapper
// =============================================================================

struct kinet_session_t {
    std::unique_ptr<kinet::accel::Session> session;
    std::unordered_map<kinet_tensor, std::unique_ptr<kinet::gpu::Buffer>> tensors;
};

struct kinet_tensor_t {
    kinet::gpu::BufferPtr buffer;
    std::vector<size_t> shape;
    kinet_dtype dtype;
};

// =============================================================================
// Library initialization
// =============================================================================

extern "C" {

KINET_API kinet_status kinet_init(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_initialized) return KINET_OK;

    init_core_api();
    g_initialized = true;
    return KINET_OK;
}

KINET_API void kinet_shutdown(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_initialized) return;

    // Unload plugins
    for (auto* backend : g_backends) {
        if (backend && backend->shutdown) {
            backend->shutdown();
        }
    }
    g_backends.clear();

    for (auto handle : g_loaded_plugins) {
#ifdef _WIN32
        FreeLibrary((HMODULE)handle);
#else
        dlclose(handle);
#endif
    }
    g_loaded_plugins.clear();

    g_initialized = false;
}

KINET_API const char* kinet_version(void) {
    return "0.1.0";
}

KINET_API const char* kinet_get_error(void) {
    return g_last_error.c_str();
}

// =============================================================================
// Backend management
// =============================================================================

KINET_API kinet_status kinet_load_backend(const char* path) {
    if (!path) {
        set_error("Invalid plugin path");
        return KINET_INVALID_ARGUMENT;
    }

#ifdef _WIN32
    HMODULE handle = LoadLibraryA(path);
    if (!handle) {
        set_error("Failed to load plugin");
        return KINET_ERROR;
    }
    auto init_fn = (kinet_backend_init_fn)GetProcAddress(handle, KINET_BACKEND_INIT_SYMBOL);
#else
    void* handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        set_error(dlerror());
        return KINET_ERROR;
    }
    auto init_fn = (kinet_backend_init_fn)dlsym(handle, KINET_BACKEND_INIT_SYMBOL);
#endif

    if (!init_fn) {
        set_error("Plugin missing kinet_backend_init symbol");
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        return KINET_ERROR;
    }

    kinet_backend_interface_t* backend = init_fn(&g_core_api);
    if (!backend) {
        set_error("Plugin initialization failed");
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        return KINET_ERROR;
    }

    if (backend->api_version != KINET_BACKEND_API_VERSION) {
        set_error("Plugin API version mismatch");
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        return KINET_ERROR;
    }

    if (backend->init && !backend->init()) {
        set_error("Backend initialization failed");
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        return KINET_ERROR;
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    g_loaded_plugins.push_back(handle);
    g_backends.push_back(backend);

    return KINET_OK;
}

KINET_API int kinet_backend_count(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return static_cast<int>(g_backends.size());
}

KINET_API kinet_backend_type kinet_backend_type_at(int index) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (index < 0 || index >= static_cast<int>(g_backends.size())) {
        return KINET_BACKEND_AUTO;
    }
    return static_cast<kinet_backend_type>(g_backends[index]->type);
}

KINET_API int kinet_device_count(kinet_backend_type backend) {
    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto* b : g_backends) {
        if (static_cast<kinet_backend_type>(b->type) == backend) {
            return b->get_device_count ? b->get_device_count() : 0;
        }
    }
    return 0;
}

KINET_API kinet_status kinet_get_device_info(kinet_backend_type backend, int index, kinet_device_info* info) {
    if (!info) return KINET_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto* b : g_backends) {
        if (static_cast<kinet_backend_type>(b->type) == backend) {
            if (!b->get_device_caps) return KINET_NOT_SUPPORTED;

            kinet_device_caps_t caps;
            if (!b->get_device_caps(index, &caps)) {
                return KINET_ERROR;
            }

            info->name = caps.name;
            info->vendor = caps.vendor;
            info->backend = backend;
            info->is_discrete = caps.is_discrete;
            info->is_unified_memory = caps.is_unified_memory;
            info->total_memory = caps.total_memory;
            info->max_workgroup_size = caps.max_workgroup_size;
            return KINET_OK;
        }
    }
    return KINET_NO_BACKEND;
}

// =============================================================================
// Session management
// =============================================================================

KINET_API kinet_status kinet_session_create(kinet_session* session) {
    if (!session) return KINET_INVALID_ARGUMENT;

    auto result = kinet::accel::Session::create();
    if (!result) {
        set_error(result.error().message.c_str());
        return to_status(result.error());
    }

    auto* s = new kinet_session_t();
    s->session = std::move(*result);
    *session = s;
    return KINET_OK;
}

KINET_API kinet_status kinet_session_create_with_backend(kinet_backend_type backend, kinet_session* session) {
    if (!session) return KINET_INVALID_ARGUMENT;

    kinet::gpu::BackendType type;
    switch (backend) {
        case KINET_BACKEND_METAL: type = kinet::gpu::BackendType::Metal; break;
        case KINET_BACKEND_WEBGPU: type = kinet::gpu::BackendType::WebGPU; break;
        case KINET_BACKEND_CUDA: type = kinet::gpu::BackendType::CUDA; break;
        default: type = kinet::gpu::BackendType::Auto; break;
    }

    auto result = kinet::accel::Session::create(type);
    if (!result) {
        set_error(result.error().message.c_str());
        return to_status(result.error());
    }

    auto* s = new kinet_session_t();
    s->session = std::move(*result);
    *session = s;
    return KINET_OK;
}

KINET_API kinet_status kinet_session_create_with_device(kinet_backend_type backend, int device_index,
                                                   kinet_session* session) {
    // TODO: implement device selection
    (void)device_index;
    return kinet_session_create_with_backend(backend, session);
}

KINET_API void kinet_session_destroy(kinet_session session) {
    delete session;
}

KINET_API kinet_status kinet_session_sync(kinet_session session) {
    if (!session || !session->session) return KINET_INVALID_ARGUMENT;
    auto status = session->session->sync();
    if (!status.ok()) {
        set_error(status.message.c_str());
        return to_status(status);
    }
    return KINET_OK;
}

KINET_API kinet_status kinet_session_get_device_info(kinet_session session, kinet_device_info* info) {
    if (!session || !session->session || !info) return KINET_INVALID_ARGUMENT;

    const auto& dev_info = session->session->device().info();
    info->name = dev_info.name.c_str();
    info->vendor = dev_info.vendor.c_str();
    info->backend = static_cast<kinet_backend_type>(dev_info.backend);
    info->is_discrete = dev_info.is_discrete ? 1 : 0;
    info->is_unified_memory = dev_info.is_unified_memory ? 1 : 0;
    info->total_memory = dev_info.total_memory;
    info->max_workgroup_size = dev_info.features.max_workgroup_size;
    return KINET_OK;
}

// =============================================================================
// Tensor operations
// =============================================================================

static size_t dtype_size(kinet_dtype dtype) {
    switch (dtype) {
        case KINET_DTYPE_F32: return 4;
        case KINET_DTYPE_F16: return 2;
        case KINET_DTYPE_F64: return 8;
        case KINET_DTYPE_I32: return 4;
        case KINET_DTYPE_I64: return 8;
        case KINET_DTYPE_U8: return 1;
        case KINET_DTYPE_U32: return 4;
        case KINET_DTYPE_U64: return 8;
        default: return 0;
    }
}

KINET_API kinet_status kinet_tensor_create(kinet_session session, kinet_dtype dtype,
                                      const size_t* shape, size_t ndim,
                                      kinet_tensor* tensor) {
    if (!session || !session->session || !shape || !tensor) return KINET_INVALID_ARGUMENT;

    size_t numel = 1;
    for (size_t i = 0; i < ndim; i++) numel *= shape[i];
    size_t bytes = numel * dtype_size(dtype);

    auto result = session->session->device().createBuffer(
        bytes, kinet::gpu::BufferUsage::Storage | kinet::gpu::BufferUsage::MapRead |
               kinet::gpu::BufferUsage::MapWrite);

    if (!result) {
        set_error(result.error().message.c_str());
        return to_status(result.error());
    }

    auto* t = new kinet_tensor_t();
    t->buffer = std::move(*result);
    t->shape.assign(shape, shape + ndim);
    t->dtype = dtype;
    *tensor = t;
    return KINET_OK;
}

KINET_API kinet_status kinet_tensor_create_with_data(kinet_session session, kinet_dtype dtype,
                                                const size_t* shape, size_t ndim,
                                                const void* data, size_t data_bytes,
                                                kinet_tensor* tensor) {
    if (!session || !session->session || !shape || !data || !tensor) return KINET_INVALID_ARGUMENT;

    size_t numel = 1;
    for (size_t i = 0; i < ndim; i++) numel *= shape[i];
    size_t expected_bytes = numel * dtype_size(dtype);

    if (data_bytes != expected_bytes) return KINET_INVALID_ARGUMENT;

    auto result = session->session->device().createBuffer(
        std::span<const uint8_t>(static_cast<const uint8_t*>(data), data_bytes),
        kinet::gpu::BufferUsage::Storage | kinet::gpu::BufferUsage::MapRead |
        kinet::gpu::BufferUsage::MapWrite);

    if (!result) {
        set_error(result.error().message.c_str());
        return to_status(result.error());
    }

    auto* t = new kinet_tensor_t();
    t->buffer = std::move(*result);
    t->shape.assign(shape, shape + ndim);
    t->dtype = dtype;
    *tensor = t;
    return KINET_OK;
}

KINET_API void kinet_tensor_destroy(kinet_tensor tensor) {
    delete tensor;
}

KINET_API size_t kinet_tensor_ndim(kinet_tensor tensor) {
    return tensor ? tensor->shape.size() : 0;
}

KINET_API size_t kinet_tensor_shape(kinet_tensor tensor, size_t dim) {
    if (!tensor || dim >= tensor->shape.size()) return 0;
    return tensor->shape[dim];
}

KINET_API size_t kinet_tensor_numel(kinet_tensor tensor) {
    if (!tensor) return 0;
    size_t n = 1;
    for (auto d : tensor->shape) n *= d;
    return n;
}

KINET_API size_t kinet_tensor_bytes(kinet_tensor tensor) {
    return tensor ? kinet_tensor_numel(tensor) * dtype_size(tensor->dtype) : 0;
}

KINET_API kinet_dtype kinet_tensor_dtype(kinet_tensor tensor) {
    return tensor ? tensor->dtype : KINET_DTYPE_F32;
}

KINET_API kinet_status kinet_tensor_to_host(kinet_tensor tensor, void* dst, size_t dst_bytes) {
    if (!tensor || !dst) return KINET_INVALID_ARGUMENT;

    size_t bytes = kinet_tensor_bytes(tensor);
    if (dst_bytes < bytes) return KINET_INVALID_ARGUMENT;

    auto mapped = tensor->buffer->map();
    if (!mapped) {
        set_error(mapped.error().message.c_str());
        return to_status(mapped.error());
    }

    std::memcpy(dst, *mapped, bytes);
    tensor->buffer->unmap();
    return KINET_OK;
}

KINET_API kinet_status kinet_tensor_from_host(kinet_tensor tensor, const void* src, size_t src_bytes) {
    if (!tensor || !src) return KINET_INVALID_ARGUMENT;

    size_t bytes = kinet_tensor_bytes(tensor);
    if (src_bytes != bytes) return KINET_INVALID_ARGUMENT;

    auto mapped = tensor->buffer->map();
    if (!mapped) {
        set_error(mapped.error().message.c_str());
        return to_status(mapped.error());
    }

    std::memcpy(*mapped, src, bytes);
    tensor->buffer->unmap();
    return KINET_OK;
}

// =============================================================================
// Stub implementations for ops (TODO: implement with real kernels)
// =============================================================================

KINET_API kinet_status kinet_matmul(kinet_session session, kinet_tensor a, kinet_tensor b, kinet_tensor c) {
    (void)session; (void)a; (void)b; (void)c;
    set_error("matmul not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_relu(kinet_session session, kinet_tensor input, kinet_tensor output) {
    (void)session; (void)input; (void)output;
    set_error("relu not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_gelu(kinet_session session, kinet_tensor input, kinet_tensor output) {
    (void)session; (void)input; (void)output;
    set_error("gelu not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_softmax(kinet_session session, kinet_tensor input, kinet_tensor output, int axis) {
    (void)session; (void)input; (void)output; (void)axis;
    set_error("softmax not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_layer_norm(kinet_session session, kinet_tensor input,
                                   kinet_tensor gamma, kinet_tensor beta,
                                   kinet_tensor output, float eps) {
    (void)session; (void)input; (void)gamma; (void)beta; (void)output; (void)eps;
    set_error("layer_norm not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_attention(kinet_session session, kinet_tensor q, kinet_tensor k, kinet_tensor v,
                                  kinet_tensor output, float scale) {
    (void)session; (void)q; (void)k; (void)v; (void)output; (void)scale;
    set_error("attention not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_sha256(kinet_session session, kinet_tensor input, kinet_tensor output) {
    (void)session; (void)input; (void)output;
    set_error("sha256 not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_keccak256(kinet_session session, kinet_tensor input, kinet_tensor output) {
    (void)session; (void)input; (void)output;
    set_error("keccak256 not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_poseidon(kinet_session session, kinet_tensor input, kinet_tensor output) {
    (void)session; (void)input; (void)output;
    set_error("poseidon not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_ecdsa_verify_batch(kinet_session session, kinet_tensor messages,
                                           kinet_tensor signatures, kinet_tensor pubkeys,
                                           kinet_tensor results) {
    (void)session; (void)messages; (void)signatures; (void)pubkeys; (void)results;
    set_error("ecdsa_verify_batch not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_ed25519_verify_batch(kinet_session session, kinet_tensor messages,
                                             kinet_tensor signatures, kinet_tensor pubkeys,
                                             kinet_tensor results) {
    (void)session; (void)messages; (void)signatures; (void)pubkeys; (void)results;
    set_error("ed25519_verify_batch not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_bls_verify_batch(kinet_session session, kinet_tensor messages,
                                         kinet_tensor signatures, kinet_tensor pubkeys,
                                         kinet_tensor results) {
    (void)session; (void)messages; (void)signatures; (void)pubkeys; (void)results;
    set_error("bls_verify_batch not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_merkle_root(kinet_session session, kinet_tensor leaves, kinet_tensor root) {
    (void)session; (void)leaves; (void)root;
    set_error("merkle_root not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_ntt(kinet_session session, kinet_tensor input, kinet_tensor output,
                            kinet_tensor roots, uint64_t modulus) {
    (void)session; (void)input; (void)output; (void)roots; (void)modulus;
    set_error("ntt not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_intt(kinet_session session, kinet_tensor input, kinet_tensor output,
                             kinet_tensor inv_roots, uint64_t modulus) {
    (void)session; (void)input; (void)output; (void)inv_roots; (void)modulus;
    set_error("intt not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_msm(kinet_session session, kinet_tensor scalars, kinet_tensor bases,
                            kinet_tensor result) {
    (void)session; (void)scalars; (void)bases; (void)result;
    set_error("msm not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_poly_mul(kinet_session session, kinet_tensor a, kinet_tensor b,
                                 kinet_tensor c, uint64_t modulus) {
    (void)session; (void)a; (void)b; (void)c; (void)modulus;
    set_error("poly_mul not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_kyber_keygen(kinet_session session, kinet_tensor pk, kinet_tensor sk) {
    (void)session; (void)pk; (void)sk;
    set_error("kyber_keygen not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_kyber_encaps(kinet_session session, kinet_tensor pk,
                                     kinet_tensor ct, kinet_tensor ss) {
    (void)session; (void)pk; (void)ct; (void)ss;
    set_error("kyber_encaps not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_kyber_decaps(kinet_session session, kinet_tensor ct,
                                     kinet_tensor sk, kinet_tensor ss) {
    (void)session; (void)ct; (void)sk; (void)ss;
    set_error("kyber_decaps not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_dilithium_sign(kinet_session session, kinet_tensor msg,
                                       kinet_tensor sk, kinet_tensor sig) {
    (void)session; (void)msg; (void)sk; (void)sig;
    set_error("dilithium_sign not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_dilithium_verify(kinet_session session, kinet_tensor msg,
                                         kinet_tensor sig, kinet_tensor pk, int* valid) {
    (void)session; (void)msg; (void)sig; (void)pk; (void)valid;
    set_error("dilithium_verify not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_bfv_encrypt(kinet_session session, kinet_tensor plaintext,
                                    kinet_tensor pk, kinet_tensor ciphertext) {
    (void)session; (void)plaintext; (void)pk; (void)ciphertext;
    set_error("bfv_encrypt not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_bfv_decrypt(kinet_session session, kinet_tensor ciphertext,
                                    kinet_tensor sk, kinet_tensor plaintext) {
    (void)session; (void)ciphertext; (void)sk; (void)plaintext;
    set_error("bfv_decrypt not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_bfv_add(kinet_session session, kinet_tensor ct1,
                                kinet_tensor ct2, kinet_tensor result) {
    (void)session; (void)ct1; (void)ct2; (void)result;
    set_error("bfv_add not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_bfv_multiply(kinet_session session, kinet_tensor ct1,
                                     kinet_tensor ct2, kinet_tensor relin_key,
                                     kinet_tensor result) {
    (void)session; (void)ct1; (void)ct2; (void)relin_key; (void)result;
    set_error("bfv_multiply not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_constant_product_swap(kinet_session session,
                                              kinet_tensor reserve_x, kinet_tensor reserve_y,
                                              kinet_tensor amount_in, int x_to_y,
                                              kinet_tensor amount_out, float fee) {
    (void)session; (void)reserve_x; (void)reserve_y; (void)amount_in;
    (void)x_to_y; (void)amount_out; (void)fee;
    set_error("constant_product_swap not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_compute_twap(kinet_session session, kinet_tensor prices,
                                     kinet_tensor timestamps, uint64_t start, uint64_t end,
                                     kinet_tensor twap) {
    (void)session; (void)prices; (void)timestamps; (void)start; (void)end; (void)twap;
    set_error("compute_twap not yet implemented");
    return KINET_NOT_SUPPORTED;
}

KINET_API kinet_status kinet_match_orders(kinet_session session, kinet_tensor bids, kinet_tensor asks,
                                     kinet_tensor matches, kinet_tensor prices, kinet_tensor amounts) {
    (void)session; (void)bids; (void)asks; (void)matches; (void)prices; (void)amounts;
    set_error("match_orders not yet implemented");
    return KINET_NOT_SUPPORTED;
}

} // extern "C"
