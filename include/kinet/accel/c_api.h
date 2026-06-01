// kinet/accel/c_api.h - C ABI for kinet-accel
//
// This is the ONLY public interface for Go/FFI consumers.
// All symbols are exported with C linkage and stable ABI.

#ifndef KINET_ACCEL_C_API_H
#define KINET_ACCEL_C_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Symbol visibility
// =============================================================================

#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef KINET_ACCEL_BUILDING
    #define KINET_API __declspec(dllexport)
  #else
    #define KINET_API __declspec(dllimport)
  #endif
#elif defined(__GNUC__) || defined(__clang__)
  #define KINET_API __attribute__((visibility("default")))
#else
  #define KINET_API
#endif

// =============================================================================
// Opaque handles
// =============================================================================

typedef struct kinet_session_t* kinet_session;
typedef struct kinet_tensor_t* kinet_tensor;
typedef struct kinet_buffer_t* kinet_buffer;

// =============================================================================
// Status codes
// =============================================================================

typedef enum kinet_status {
    KINET_OK = 0,
    KINET_ERROR = 1,
    KINET_OUT_OF_MEMORY = 2,
    KINET_INVALID_ARGUMENT = 3,
    KINET_NOT_SUPPORTED = 4,
    KINET_NO_BACKEND = 5,
    KINET_KERNEL_ERROR = 6,
    KINET_DISPATCH_FAILED = 7,
} kinet_status;

// =============================================================================
// Backend types
// =============================================================================

typedef enum kinet_backend_type {
    KINET_BACKEND_AUTO = 0,
    KINET_BACKEND_METAL = 1,
    KINET_BACKEND_WEBGPU = 2,
    KINET_BACKEND_CUDA = 3,
} kinet_backend_type;

// =============================================================================
// Data types
// =============================================================================

typedef enum kinet_dtype {
    KINET_DTYPE_F32 = 0,
    KINET_DTYPE_F16 = 1,
    KINET_DTYPE_F64 = 2,
    KINET_DTYPE_I32 = 3,
    KINET_DTYPE_I64 = 4,
    KINET_DTYPE_U8 = 5,
    KINET_DTYPE_U32 = 6,
    KINET_DTYPE_U64 = 7,
} kinet_dtype;

// =============================================================================
// Device info
// =============================================================================

typedef struct kinet_device_info {
    const char* name;
    const char* vendor;
    kinet_backend_type backend;
    int is_discrete;
    int is_unified_memory;
    uint64_t total_memory;
    uint32_t max_workgroup_size;
    uint32_t simd_width;
} kinet_device_info;

// =============================================================================
// Library initialization
// =============================================================================

// Initialize the library (call once at startup)
KINET_API kinet_status kinet_init(void);

// Shutdown and release all resources
KINET_API void kinet_shutdown(void);

// Get library version string
KINET_API const char* kinet_version(void);

// Get last error message (thread-local)
KINET_API const char* kinet_get_error(void);

// =============================================================================
// Backend management
// =============================================================================

// Load a backend plugin from path (e.g., "libkinet-metal.dylib")
KINET_API kinet_status kinet_load_backend(const char* path);

// Get number of available backends
KINET_API int kinet_backend_count(void);

// Get backend type at index
KINET_API kinet_backend_type kinet_backend_type_at(int index);

// Get number of devices for a backend
KINET_API int kinet_device_count(kinet_backend_type backend);

// Get device info
KINET_API kinet_status kinet_get_device_info(kinet_backend_type backend, int index, kinet_device_info* info);

// =============================================================================
// Session management
// =============================================================================

// Create session with auto-detected best backend
KINET_API kinet_status kinet_session_create(kinet_session* session);

// Create session with specific backend
KINET_API kinet_status kinet_session_create_with_backend(kinet_backend_type backend, kinet_session* session);

// Create session with specific device
KINET_API kinet_status kinet_session_create_with_device(kinet_backend_type backend, int device_index, kinet_session* session);

// Destroy session
KINET_API void kinet_session_destroy(kinet_session session);

// Synchronize all pending operations
KINET_API kinet_status kinet_session_sync(kinet_session session);

// Get session device info
KINET_API kinet_status kinet_session_get_device_info(kinet_session session, kinet_device_info* info);

// =============================================================================
// Tensor operations
// =============================================================================

// Create tensor with shape
KINET_API kinet_status kinet_tensor_create(kinet_session session, kinet_dtype dtype,
                                      const size_t* shape, size_t ndim,
                                      kinet_tensor* tensor);

// Create tensor with data
KINET_API kinet_status kinet_tensor_create_with_data(kinet_session session, kinet_dtype dtype,
                                                const size_t* shape, size_t ndim,
                                                const void* data, size_t data_bytes,
                                                kinet_tensor* tensor);

// Destroy tensor
KINET_API void kinet_tensor_destroy(kinet_tensor tensor);

// Get tensor shape
KINET_API size_t kinet_tensor_ndim(kinet_tensor tensor);
KINET_API size_t kinet_tensor_shape(kinet_tensor tensor, size_t dim);
KINET_API size_t kinet_tensor_numel(kinet_tensor tensor);
KINET_API size_t kinet_tensor_bytes(kinet_tensor tensor);
KINET_API kinet_dtype kinet_tensor_dtype(kinet_tensor tensor);

// Copy data to/from tensor
KINET_API kinet_status kinet_tensor_to_host(kinet_tensor tensor, void* dst, size_t dst_bytes);
KINET_API kinet_status kinet_tensor_from_host(kinet_tensor tensor, const void* src, size_t src_bytes);

// =============================================================================
// ML operations
// =============================================================================

KINET_API kinet_status kinet_matmul(kinet_session session, kinet_tensor a, kinet_tensor b, kinet_tensor c);
KINET_API kinet_status kinet_relu(kinet_session session, kinet_tensor input, kinet_tensor output);
KINET_API kinet_status kinet_gelu(kinet_session session, kinet_tensor input, kinet_tensor output);
KINET_API kinet_status kinet_softmax(kinet_session session, kinet_tensor input, kinet_tensor output, int axis);
KINET_API kinet_status kinet_layer_norm(kinet_session session, kinet_tensor input,
                                   kinet_tensor gamma, kinet_tensor beta,
                                   kinet_tensor output, float eps);
KINET_API kinet_status kinet_attention(kinet_session session, kinet_tensor q, kinet_tensor k, kinet_tensor v,
                                  kinet_tensor output, float scale);

// =============================================================================
// Crypto operations
// =============================================================================

KINET_API kinet_status kinet_sha256(kinet_session session, kinet_tensor input, kinet_tensor output);
KINET_API kinet_status kinet_keccak256(kinet_session session, kinet_tensor input, kinet_tensor output);
KINET_API kinet_status kinet_poseidon(kinet_session session, kinet_tensor input, kinet_tensor output);
KINET_API kinet_status kinet_ecdsa_verify_batch(kinet_session session, kinet_tensor messages,
                                           kinet_tensor signatures, kinet_tensor pubkeys,
                                           kinet_tensor results);
KINET_API kinet_status kinet_ed25519_verify_batch(kinet_session session, kinet_tensor messages,
                                             kinet_tensor signatures, kinet_tensor pubkeys,
                                             kinet_tensor results);
KINET_API kinet_status kinet_bls_verify_batch(kinet_session session, kinet_tensor messages,
                                         kinet_tensor signatures, kinet_tensor pubkeys,
                                         kinet_tensor results);
KINET_API kinet_status kinet_merkle_root(kinet_session session, kinet_tensor leaves, kinet_tensor root);

// =============================================================================
// ZK operations
// =============================================================================

KINET_API kinet_status kinet_ntt(kinet_session session, kinet_tensor input, kinet_tensor output,
                            kinet_tensor roots, uint64_t modulus);
KINET_API kinet_status kinet_intt(kinet_session session, kinet_tensor input, kinet_tensor output,
                             kinet_tensor inv_roots, uint64_t modulus);
KINET_API kinet_status kinet_msm(kinet_session session, kinet_tensor scalars, kinet_tensor bases,
                            kinet_tensor result);
KINET_API kinet_status kinet_poly_mul(kinet_session session, kinet_tensor a, kinet_tensor b,
                                 kinet_tensor c, uint64_t modulus);

// =============================================================================
// Lattice crypto operations
// =============================================================================

KINET_API kinet_status kinet_kyber_keygen(kinet_session session, kinet_tensor pk, kinet_tensor sk);
KINET_API kinet_status kinet_kyber_encaps(kinet_session session, kinet_tensor pk,
                                     kinet_tensor ct, kinet_tensor ss);
KINET_API kinet_status kinet_kyber_decaps(kinet_session session, kinet_tensor ct,
                                     kinet_tensor sk, kinet_tensor ss);
KINET_API kinet_status kinet_dilithium_sign(kinet_session session, kinet_tensor msg,
                                       kinet_tensor sk, kinet_tensor sig);
KINET_API kinet_status kinet_dilithium_verify(kinet_session session, kinet_tensor msg,
                                         kinet_tensor sig, kinet_tensor pk, int* valid);

// =============================================================================
// SLH-DSA / Comet (FIPS 205) operations
// =============================================================================
//
// SLH-DSA is a stateless hash-based signature scheme (FIPS 205, formerly
// SPHINCS+). The Comet protocol slot (`0x012207`) lifts SLH-DSA into Kinet's
// PQ-GPU dispatch path: per-validator verify batched across the cert quorum.
//
// Mode encoding mirrors the kinetcpp/crypto/slhdsa C ABI:
//   2  -> SLH-DSA-SHA2-128f  (NIST L1)
//   3  -> SLH-DSA-SHA2-192f  (NIST L3)   <-- canonical for Comet cert profile
//   5  -> SLH-DSA-SHA2-256f  (NIST L5)
//   12 -> SLH-DSA-SHAKE-128f (NIST L1)
//   13 -> SLH-DSA-SHAKE-192f (NIST L3)
//   15 -> SLH-DSA-SHAKE-256f (NIST L5)
//
// Tensor shapes (n = batch size):
//   msgs    : KINET_DTYPE_U8, shape [n, msg_width]
//   sigs    : KINET_DTYPE_U8, shape [n, sig_bytes]   (sig_bytes per mode)
//   pks     : KINET_DTYPE_U8, shape [n, pk_bytes]    (pk_bytes per mode)
//   results : KINET_DTYPE_U8, shape [n]              (1 = valid, 0 = invalid)
//
// Batch verify dispatches the FIPS 205 verify per element. Result vector is
// dense (no early abort) so consumers can audit per-signer failures. Sign
// batch is provided symmetrically; deterministic per FIPS 205 (no nonces).

KINET_API kinet_status kinet_slhdsa_sign_batch(kinet_session session, int mode,
                                          kinet_tensor msgs, kinet_tensor sks,
                                          kinet_tensor sigs);
KINET_API kinet_status kinet_slhdsa_verify_batch(kinet_session session, int mode,
                                            kinet_tensor msgs, kinet_tensor sigs,
                                            kinet_tensor pks, kinet_tensor results);

// =============================================================================
// FHE operations
// =============================================================================

KINET_API kinet_status kinet_bfv_encrypt(kinet_session session, kinet_tensor plaintext,
                                    kinet_tensor pk, kinet_tensor ciphertext);
KINET_API kinet_status kinet_bfv_decrypt(kinet_session session, kinet_tensor ciphertext,
                                    kinet_tensor sk, kinet_tensor plaintext);
KINET_API kinet_status kinet_bfv_add(kinet_session session, kinet_tensor ct1,
                                kinet_tensor ct2, kinet_tensor result);
KINET_API kinet_status kinet_bfv_multiply(kinet_session session, kinet_tensor ct1,
                                     kinet_tensor ct2, kinet_tensor relin_key,
                                     kinet_tensor result);

// =============================================================================
// DEX operations
// =============================================================================

KINET_API kinet_status kinet_constant_product_swap(kinet_session session,
                                              kinet_tensor reserve_x, kinet_tensor reserve_y,
                                              kinet_tensor amount_in, int x_to_y,
                                              kinet_tensor amount_out, float fee);
KINET_API kinet_status kinet_compute_twap(kinet_session session, kinet_tensor prices,
                                     kinet_tensor timestamps, uint64_t start, uint64_t end,
                                     kinet_tensor twap);
KINET_API kinet_status kinet_match_orders(kinet_session session, kinet_tensor bids, kinet_tensor asks,
                                     kinet_tensor matches, kinet_tensor prices, kinet_tensor amounts);

#ifdef __cplusplus
}
#endif

#endif // KINET_ACCEL_C_API_H
