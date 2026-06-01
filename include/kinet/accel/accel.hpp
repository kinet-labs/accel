// kinet/accel/accel.hpp - GPU Acceleration SDK

#pragma once

#include <kinet/gpu/gpu.hpp>
#include <kinet/accel/tensor.hpp>
#include <kinet/accel/ml.hpp>
#include <kinet/accel/crypto.hpp>
#include <kinet/accel/zk.hpp>
#include <kinet/accel/lattice.hpp>
#include <kinet/accel/fhe.hpp>
#include <kinet/accel/dex.hpp>

namespace kinet::accel {

/// Session manages a GPU context for acceleration operations
class Session {
public:
    /// Create session with auto-detected best backend
    static gpu::Result<std::unique_ptr<Session>> create();

    /// Create session with specific backend preference
    static gpu::Result<std::unique_ptr<Session>> create(gpu::BackendType backend);

    /// Create session with specific device
    static gpu::Result<std::unique_ptr<Session>> create(gpu::DevicePtr device);

    ~Session();

    /// Get underlying device
    gpu::Device& device() { return *device_; }
    const gpu::Device& device() const { return *device_; }

    /// Get command queue
    gpu::Queue& queue() { return *queue_; }

    /// Create tensor in this session
    template<typename T>
    gpu::Result<Tensor<T>> createTensor(std::initializer_list<size_t> shape);

    template<typename T>
    gpu::Result<Tensor<T>> createTensor(std::span<const size_t> shape);

    template<typename T>
    gpu::Result<Tensor<T>> createTensor(std::span<const size_t> shape, std::span<const T> data);

    /// Synchronize all operations
    gpu::Status sync() { return queue_->waitIdle(); }

    /// ML operations accessor
    ml::Ops& ml() { return *ml_; }

    /// Crypto operations accessor
    crypto::Ops& crypto() { return *crypto_; }

    /// ZK operations accessor
    zk::Ops& zk() { return *zk_; }

    /// Lattice operations accessor
    lattice::Ops& lattice() { return *lattice_; }

    /// FHE operations accessor
    fhe::Ops& fhe() { return *fhe_; }

    /// DEX operations accessor
    dex::Ops& dex() { return *dex_; }

private:
    Session(gpu::DevicePtr device, gpu::QueuePtr queue);

    gpu::DevicePtr device_;
    gpu::QueuePtr queue_;
    std::unique_ptr<ml::Ops> ml_;
    std::unique_ptr<crypto::Ops> crypto_;
    std::unique_ptr<zk::Ops> zk_;
    std::unique_ptr<lattice::Ops> lattice_;
    std::unique_ptr<fhe::Ops> fhe_;
    std::unique_ptr<dex::Ops> dex_;
};

} // namespace kinet::accel
