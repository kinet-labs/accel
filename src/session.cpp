#include <kinet/accel/accel.hpp>

namespace kinet::accel {

Session::Session(gpu::DevicePtr device, gpu::QueuePtr queue)
    : device_(std::move(device)), queue_(std::move(queue)) {
    ml_ = std::make_unique<ml::Ops>(*device_, *queue_);
    crypto_ = std::make_unique<crypto::Ops>(*device_, *queue_);
    zk_ = std::make_unique<zk::Ops>(*device_, *queue_);
    lattice_ = std::make_unique<lattice::Ops>(*device_, *queue_);
    fhe_ = std::make_unique<fhe::Ops>(*device_, *queue_);
    dex_ = std::make_unique<dex::Ops>(*device_, *queue_);
}

Session::~Session() = default;

gpu::Result<std::unique_ptr<Session>> Session::create() {
    auto& ctx = gpu::context();

    // Try backends in preference order
    for (auto type : {gpu::BackendType::Metal, gpu::BackendType::CUDA, gpu::BackendType::WebGPU}) {
        auto backend = ctx.getBackend(type);
        if (backend && backend->isAvailable()) {
            auto devices = backend->enumerateDevices();
            if (!devices.empty()) {
                auto device = backend->createDevice(0);
                if (device) {
                    auto queue = (*device)->createQueue();
                    if (queue) {
                        return std::unique_ptr<Session>(new Session(std::move(*device), std::move(*queue)));
                    }
                }
            }
        }
    }

    return std::unexpected(gpu::Status::Error(gpu::StatusCode::BackendNotAvailable,
        "No GPU backend available"));
}

gpu::Result<std::unique_ptr<Session>> Session::create(gpu::BackendType backendType) {
    auto& ctx = gpu::context();
    auto backend = ctx.getBackend(backendType);

    if (!backend || !backend->isAvailable())
        return std::unexpected(gpu::Status::Error(gpu::StatusCode::BackendNotAvailable));

    auto device = backend->createDevice(0);
    if (!device) return std::unexpected(device.error());

    auto queue = (*device)->createQueue();
    if (!queue) return std::unexpected(queue.error());

    return std::unique_ptr<Session>(new Session(std::move(*device), std::move(*queue)));
}

gpu::Result<std::unique_ptr<Session>> Session::create(gpu::DevicePtr device) {
    auto queue = device->createQueue();
    if (!queue) return std::unexpected(queue.error());

    return std::unique_ptr<Session>(new Session(std::move(device), std::move(*queue)));
}

template<typename T>
gpu::Result<Tensor<T>> Session::createTensor(std::initializer_list<size_t> shape) {
    std::vector<size_t> shapeVec(shape);
    return createTensor<T>(std::span<const size_t>(shapeVec));
}

template<typename T>
gpu::Result<Tensor<T>> Session::createTensor(std::span<const size_t> shape) {
    size_t numel = 1;
    for (auto dim : shape) numel *= dim;
    size_t bytes = numel * sizeof(T);

    auto buffer = device_->createBuffer(bytes, gpu::BufferUsage::Storage |
                                               gpu::BufferUsage::MapRead |
                                               gpu::BufferUsage::MapWrite);
    if (!buffer) return std::unexpected(buffer.error());

    return Tensor<T>(std::move(*buffer), std::vector<size_t>(shape.begin(), shape.end()));
}

template<typename T>
gpu::Result<Tensor<T>> Session::createTensor(std::span<const size_t> shape, std::span<const T> data) {
    size_t numel = 1;
    for (auto dim : shape) numel *= dim;

    if (data.size() != numel)
        return std::unexpected(gpu::Status::Error(gpu::StatusCode::InvalidArgument,
            "Data size does not match shape"));

    auto buffer = device_->createBuffer(
        std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), data.size() * sizeof(T)),
        gpu::BufferUsage::Storage | gpu::BufferUsage::MapRead | gpu::BufferUsage::MapWrite);
    if (!buffer) return std::unexpected(buffer.error());

    return Tensor<T>(std::move(*buffer), std::vector<size_t>(shape.begin(), shape.end()));
}

// Explicit instantiations
template gpu::Result<Tensor<float>> Session::createTensor(std::initializer_list<size_t>);
template gpu::Result<Tensor<float>> Session::createTensor(std::span<const size_t>);
template gpu::Result<Tensor<float>> Session::createTensor(std::span<const size_t>, std::span<const float>);

template gpu::Result<Tensor<uint16_t>> Session::createTensor(std::initializer_list<size_t>);
template gpu::Result<Tensor<uint16_t>> Session::createTensor(std::span<const size_t>);
template gpu::Result<Tensor<uint16_t>> Session::createTensor(std::span<const size_t>, std::span<const uint16_t>);

template gpu::Result<Tensor<int32_t>> Session::createTensor(std::initializer_list<size_t>);
template gpu::Result<Tensor<int32_t>> Session::createTensor(std::span<const size_t>);
template gpu::Result<Tensor<int32_t>> Session::createTensor(std::span<const size_t>, std::span<const int32_t>);

template gpu::Result<Tensor<uint32_t>> Session::createTensor(std::initializer_list<size_t>);
template gpu::Result<Tensor<uint32_t>> Session::createTensor(std::span<const size_t>);
template gpu::Result<Tensor<uint32_t>> Session::createTensor(std::span<const size_t>, std::span<const uint32_t>);

template gpu::Result<Tensor<uint64_t>> Session::createTensor(std::initializer_list<size_t>);
template gpu::Result<Tensor<uint64_t>> Session::createTensor(std::span<const size_t>);
template gpu::Result<Tensor<uint64_t>> Session::createTensor(std::span<const size_t>, std::span<const uint64_t>);

template gpu::Result<Tensor<uint8_t>> Session::createTensor(std::initializer_list<size_t>);
template gpu::Result<Tensor<uint8_t>> Session::createTensor(std::span<const size_t>);
template gpu::Result<Tensor<uint8_t>> Session::createTensor(std::span<const size_t>, std::span<const uint8_t>);

template gpu::Result<Tensor<double>> Session::createTensor(std::initializer_list<size_t>);
template gpu::Result<Tensor<double>> Session::createTensor(std::span<const size_t>);
template gpu::Result<Tensor<double>> Session::createTensor(std::span<const size_t>, std::span<const double>);

} // namespace kinet::accel
