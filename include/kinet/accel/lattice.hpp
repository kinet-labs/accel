// kinet/accel/lattice.hpp - Post-Quantum Lattice Cryptography Acceleration

#pragma once

#include <kinet/accel/tensor.hpp>

namespace kinet::accel::lattice {

/// Lattice crypto operations interface
class Ops {
public:
    explicit Ops(gpu::Device& device, gpu::Queue& queue);
    ~Ops();

    // Ring polynomial operations
    gpu::Status ring_ntt(const TensorI32& input, TensorI32& output, int32_t modulus);
    gpu::Status ring_intt(const TensorI32& input, TensorI32& output, int32_t modulus);
    gpu::Status ring_mul(const TensorI32& a, const TensorI32& b, TensorI32& c, int32_t modulus);
    gpu::Status ring_add(const TensorI32& a, const TensorI32& b, TensorI32& c, int32_t modulus);

    // Kyber (ML-KEM) operations
    gpu::Status kyber_keygen(TensorU8& public_key, TensorU8& secret_key);
    gpu::Status kyber_encaps(const TensorU8& public_key, TensorU8& ciphertext, TensorU8& shared_secret);
    gpu::Status kyber_decaps(const TensorU8& ciphertext, const TensorU8& secret_key, TensorU8& shared_secret);
    gpu::Status kyber_cbd_sample(const TensorU8& seed, TensorI32& poly, int eta);
    gpu::Status kyber_compress(const TensorI32& poly, TensorU8& output, int bits);
    gpu::Status kyber_decompress(const TensorU8& input, TensorI32& poly, int bits);

    // Dilithium (ML-DSA) operations
    gpu::Status dilithium_keygen(TensorU8& public_key, TensorU8& secret_key);
    gpu::Status dilithium_sign(const TensorU8& message, const TensorU8& secret_key, TensorU8& signature);
    gpu::Status dilithium_verify(const TensorU8& message, const TensorU8& signature,
                                 const TensorU8& public_key, bool& valid);
    gpu::Status dilithium_expand_a(const TensorU8& seed, TensorI32& matrix);
    gpu::Status dilithium_sample_y(const TensorU8& seed, int kappa, TensorI32& y);

    // NTRU operations
    gpu::Status ntru_keygen(TensorU8& public_key, TensorU8& secret_key, int n, int q);
    gpu::Status ntru_encrypt(const TensorU8& message, const TensorU8& public_key, TensorU8& ciphertext);
    gpu::Status ntru_decrypt(const TensorU8& ciphertext, const TensorU8& secret_key, TensorU8& message);

    // Module-LWE operations
    gpu::Status mlwe_sample_error(const TensorU8& seed, TensorI32& error, int dimension, int bound);
    gpu::Status mlwe_encrypt(const TensorI32& message, const TensorI32& public_key,
                             const TensorI32& error, TensorI32& ciphertext, int32_t modulus);
    gpu::Status mlwe_decrypt(const TensorI32& ciphertext, const TensorI32& secret_key,
                             TensorI32& message, int32_t modulus);

    // Gaussian/discrete Gaussian sampling
    gpu::Status sample_gaussian(const TensorU8& seed, TensorI32& output, double sigma);
    gpu::Status sample_centered_binomial(const TensorU8& seed, TensorI32& output, int eta);

    // Lattice reduction helpers (for analysis)
    gpu::Status gram_schmidt(const TensorF32& basis, TensorF32& orthogonal);
    gpu::Status compute_norm(const TensorI32& vector, float& norm);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kinet::accel::lattice
