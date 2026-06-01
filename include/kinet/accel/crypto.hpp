// kinet/accel/crypto.hpp - Cryptographic Acceleration Operations

#pragma once

#include <kinet/accel/tensor.hpp>

namespace kinet::accel::crypto {

/// Crypto operations interface
class Ops {
public:
    explicit Ops(gpu::Device& device, gpu::Queue& queue);
    ~Ops();

    // Hash functions
    gpu::Status sha256(const TensorU8& input, TensorU8& output);
    gpu::Status sha256_batch(const TensorU8& inputs, TensorU8& outputs, size_t count);
    gpu::Status sha512(const TensorU8& input, TensorU8& output);
    gpu::Status keccak256(const TensorU8& input, TensorU8& output);
    gpu::Status keccak256_batch(const TensorU8& inputs, TensorU8& outputs, size_t count);
    gpu::Status blake2b(const TensorU8& input, TensorU8& output, size_t outlen = 32);
    gpu::Status blake3(const TensorU8& input, TensorU8& output);
    gpu::Status poseidon(const TensorU64& input, TensorU64& output);
    gpu::Status poseidon_batch(const TensorU64& inputs, TensorU64& outputs, size_t count);

    // Elliptic curve operations
    gpu::Status ec_point_add(const TensorU64& p1, const TensorU64& p2, TensorU64& result);
    gpu::Status ec_point_double(const TensorU64& p, TensorU64& result);
    gpu::Status ec_scalar_mul(const TensorU64& scalar, const TensorU64& point, TensorU64& result);
    gpu::Status ec_multi_scalar_mul(const TensorU64& scalars, const TensorU64& points, TensorU64& result);

    // Signature operations
    gpu::Status ecdsa_verify(const TensorU8& messages, const TensorU8& signatures,
                             const TensorU8& pubkeys, TensorU8& results, size_t count);
    gpu::Status ed25519_verify(const TensorU8& messages, const TensorU8& signatures,
                               const TensorU8& pubkeys, TensorU8& results, size_t count);
    gpu::Status schnorr_verify(const TensorU8& messages, const TensorU8& signatures,
                               const TensorU8& pubkeys, TensorU8& results, size_t count);
    gpu::Status bls_verify(const TensorU8& messages, const TensorU8& signatures,
                           const TensorU8& pubkeys, TensorU8& results, size_t count);
    gpu::Status bls_aggregate_verify(const TensorU8& messages, const TensorU8& signature,
                                     const TensorU8& pubkeys, bool& result);

    // Modular arithmetic
    gpu::Status mod_exp(const TensorU64& base, const TensorU64& exp, const TensorU64& mod, TensorU64& result);
    gpu::Status mod_mul(const TensorU64& a, const TensorU64& b, const TensorU64& mod, TensorU64& result);
    gpu::Status mod_inv(const TensorU64& a, const TensorU64& mod, TensorU64& result);
    gpu::Status montgomery_mul(const TensorU64& a, const TensorU64& b, const TensorU64& mod, TensorU64& result);

    // Pairing operations
    gpu::Status bn254_pairing(const TensorU64& g1, const TensorU64& g2, TensorU64& result);
    gpu::Status bls12_381_pairing(const TensorU64& g1, const TensorU64& g2, TensorU64& result);

    // Merkle tree operations
    gpu::Status merkle_root(const TensorU8& leaves, TensorU8& root);
    gpu::Status merkle_proof(const TensorU8& leaves, size_t index, TensorU8& proof);
    gpu::Status merkle_verify(const TensorU8& leaf, const TensorU8& proof,
                              const TensorU8& root, bool& valid);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kinet::accel::crypto
