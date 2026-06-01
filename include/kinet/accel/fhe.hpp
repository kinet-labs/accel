// kinet/accel/fhe.hpp - Fully Homomorphic Encryption Acceleration

#pragma once

#include <kinet/accel/tensor.hpp>

namespace kinet::accel::fhe {

/// FHE operations interface
class Ops {
public:
    explicit Ops(gpu::Device& device, gpu::Queue& queue);
    ~Ops();

    // BFV/BGV scheme operations
    gpu::Status bfv_keygen(TensorU64& public_key, TensorU64& secret_key, TensorU64& relin_key,
                           size_t poly_degree, uint64_t plain_modulus);
    gpu::Status bfv_encrypt(const TensorU64& plaintext, const TensorU64& public_key,
                            TensorU64& ciphertext, uint64_t plain_modulus);
    gpu::Status bfv_decrypt(const TensorU64& ciphertext, const TensorU64& secret_key,
                            TensorU64& plaintext, uint64_t plain_modulus);
    gpu::Status bfv_add(const TensorU64& ct1, const TensorU64& ct2, TensorU64& result);
    gpu::Status bfv_multiply(const TensorU64& ct1, const TensorU64& ct2,
                             const TensorU64& relin_key, TensorU64& result);
    gpu::Status bfv_relinearize(const TensorU64& ciphertext, const TensorU64& relin_key,
                                TensorU64& result);
    gpu::Status bfv_mod_switch(const TensorU64& ciphertext, TensorU64& result);

    // CKKS scheme operations
    gpu::Status ckks_keygen(TensorU64& public_key, TensorU64& secret_key, TensorU64& relin_key,
                            size_t poly_degree, double scale);
    gpu::Status ckks_encode(const TensorF64& values, TensorU64& plaintext, double scale);
    gpu::Status ckks_decode(const TensorU64& plaintext, TensorF64& values, double scale);
    gpu::Status ckks_encrypt(const TensorU64& plaintext, const TensorU64& public_key,
                             TensorU64& ciphertext);
    gpu::Status ckks_decrypt(const TensorU64& ciphertext, const TensorU64& secret_key,
                             TensorU64& plaintext);
    gpu::Status ckks_add(const TensorU64& ct1, const TensorU64& ct2, TensorU64& result);
    gpu::Status ckks_multiply(const TensorU64& ct1, const TensorU64& ct2,
                              const TensorU64& relin_key, TensorU64& result);
    gpu::Status ckks_rescale(const TensorU64& ciphertext, TensorU64& result);
    gpu::Status ckks_rotate(const TensorU64& ciphertext, const TensorU64& galois_key,
                            int steps, TensorU64& result);

    // TFHE operations (for Boolean circuits)
    gpu::Status tfhe_keygen(TensorU64& secret_key, TensorU64& bootstrap_key, TensorU64& keyswitch_key);
    gpu::Status tfhe_encrypt_bit(bool value, const TensorU64& secret_key, TensorU64& ciphertext);
    gpu::Status tfhe_decrypt_bit(const TensorU64& ciphertext, const TensorU64& secret_key, bool& value);
    gpu::Status tfhe_and(const TensorU64& ct1, const TensorU64& ct2,
                         const TensorU64& bootstrap_key, TensorU64& result);
    gpu::Status tfhe_or(const TensorU64& ct1, const TensorU64& ct2,
                        const TensorU64& bootstrap_key, TensorU64& result);
    gpu::Status tfhe_xor(const TensorU64& ct1, const TensorU64& ct2,
                         const TensorU64& bootstrap_key, TensorU64& result);
    gpu::Status tfhe_not(const TensorU64& ct, TensorU64& result);
    gpu::Status tfhe_bootstrap(const TensorU64& ciphertext, const TensorU64& bootstrap_key,
                               TensorU64& result);

    // Common operations
    gpu::Status ntt_forward(const TensorU64& input, TensorU64& output, const TensorU64& roots);
    gpu::Status ntt_inverse(const TensorU64& input, TensorU64& output, const TensorU64& inv_roots);
    gpu::Status poly_multiply(const TensorU64& a, const TensorU64& b, TensorU64& c,
                              const TensorU64& modulus);

    // Noise management
    gpu::Status estimate_noise(const TensorU64& ciphertext, const TensorU64& secret_key,
                               double& noise_budget);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Type alias for double precision tensor
using TensorF64 = Tensor<double>;

} // namespace kinet::accel::fhe
