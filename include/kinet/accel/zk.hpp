// kinet/accel/zk.hpp - Zero-Knowledge Proof Acceleration Operations

#pragma once

#include <kinet/accel/tensor.hpp>

namespace kinet::accel::zk {

/// ZK proof operations interface
class Ops {
public:
    explicit Ops(gpu::Device& device, gpu::Queue& queue);
    ~Ops();

    // Number Theoretic Transform (NTT)
    gpu::Status ntt(const TensorU64& input, TensorU64& output, const TensorU64& roots, uint64_t modulus);
    gpu::Status intt(const TensorU64& input, TensorU64& output, const TensorU64& inv_roots, uint64_t modulus);
    gpu::Status ntt_batch(const TensorU64& inputs, TensorU64& outputs, const TensorU64& roots,
                          uint64_t modulus, size_t count);

    // Polynomial operations
    gpu::Status poly_mul(const TensorU64& a, const TensorU64& b, TensorU64& c, uint64_t modulus);
    gpu::Status poly_add(const TensorU64& a, const TensorU64& b, TensorU64& c, uint64_t modulus);
    gpu::Status poly_eval(const TensorU64& poly, const TensorU64& points, TensorU64& results, uint64_t modulus);
    gpu::Status poly_interpolate(const TensorU64& points, const TensorU64& values, TensorU64& poly, uint64_t modulus);

    // Multi-scalar multiplication (MSM)
    gpu::Status msm(const TensorU64& scalars, const TensorU64& bases, TensorU64& result);
    gpu::Status msm_bn254(const TensorU64& scalars, const TensorU64& bases, TensorU64& result);
    gpu::Status msm_bls12_381(const TensorU64& scalars, const TensorU64& bases, TensorU64& result);

    // Commitment schemes
    gpu::Status pedersen_commit(const TensorU64& values, const TensorU64& blinding,
                                const TensorU64& generators, TensorU64& commitment);
    gpu::Status kate_commit(const TensorU64& poly, const TensorU64& srs, TensorU64& commitment);
    gpu::Status ipa_commit(const TensorU64& values, const TensorU64& generators, TensorU64& commitment);

    // Proof generation helpers
    gpu::Status compute_quotient(const TensorU64& poly, const TensorU64& divisor,
                                 TensorU64& quotient, uint64_t modulus);
    gpu::Status compute_lagrange_basis(size_t domain_size, TensorU64& basis, uint64_t modulus);
    gpu::Status batch_inverse(const TensorU64& input, TensorU64& output, uint64_t modulus);

    // Constraint satisfaction
    gpu::Status r1cs_check(const TensorU64& a, const TensorU64& b, const TensorU64& c,
                           const TensorU64& witness, bool& satisfied);
    gpu::Status plonk_gate_check(const TensorU64& witness, const TensorU64& selectors,
                                 const TensorU64& permutation, bool& satisfied, uint64_t modulus);

    // Fiat-Shamir heuristic
    gpu::Status transcript_squeeze(const TensorU8& transcript, TensorU64& challenge);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kinet::accel::zk
