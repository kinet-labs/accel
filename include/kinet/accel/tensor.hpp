// kinet/accel/tensor.hpp - GPU Tensor Type

#pragma once

#include <kinet/gpu/gpu.hpp>
#include <vector>
#include <span>
#include <numeric>

namespace kinet::accel {

/// N-dimensional tensor backed by GPU buffer
template<typename T>
class Tensor {
public:
    using value_type = T;

    Tensor() = default;
    Tensor(gpu::BufferPtr buffer, std::vector<size_t> shape)
        : buffer_(std::move(buffer)), shape_(std::move(shape)) {
        computeStrides();
    }

    /// Shape of tensor
    std::span<const size_t> shape() const { return shape_; }

    /// Strides for indexing
    std::span<const size_t> strides() const { return strides_; }

    /// Number of dimensions
    size_t ndim() const { return shape_.size(); }

    /// Total number of elements
    size_t numel() const {
        return std::accumulate(shape_.begin(), shape_.end(), size_t(1), std::multiplies<>{});
    }

    /// Size in bytes
    size_t bytes() const { return numel() * sizeof(T); }

    /// Underlying buffer
    gpu::Buffer& buffer() { return *buffer_; }
    const gpu::Buffer& buffer() const { return *buffer_; }
    gpu::BufferPtr bufferPtr() { return buffer_; }

    /// Map to host memory
    gpu::Result<T*> map() {
        auto result = buffer_->map();
        if (!result) return std::unexpected(result.error());
        return static_cast<T*>(*result);
    }

    void unmap() { buffer_->unmap(); }

    /// View with offset
    gpu::BufferView view(size_t offset = 0, size_t size = 0) const {
        return {buffer_, offset * sizeof(T), size ? size * sizeof(T) : bytes()};
    }

    /// Reshape (must have same total elements)
    gpu::Result<Tensor<T>> reshape(std::span<const size_t> newShape) const {
        size_t newNumel = std::accumulate(newShape.begin(), newShape.end(),
                                           size_t(1), std::multiplies<>{});
        if (newNumel != numel())
            return std::unexpected(gpu::Status::Error(gpu::StatusCode::InvalidArgument,
                "Reshape must preserve total elements"));
        return Tensor<T>(buffer_, std::vector<size_t>(newShape.begin(), newShape.end()));
    }

    /// Check if contiguous in memory
    bool isContiguous() const {
        size_t expected = 1;
        for (int i = static_cast<int>(shape_.size()) - 1; i >= 0; i--) {
            if (strides_[i] != expected) return false;
            expected *= shape_[i];
        }
        return true;
    }

private:
    void computeStrides() {
        strides_.resize(shape_.size());
        size_t stride = 1;
        for (int i = static_cast<int>(shape_.size()) - 1; i >= 0; i--) {
            strides_[i] = stride;
            stride *= shape_[i];
        }
    }

    gpu::BufferPtr buffer_;
    std::vector<size_t> shape_;
    std::vector<size_t> strides_;
};

// Common tensor type aliases
using TensorF64 = Tensor<double>;
using TensorF32 = Tensor<float>;
using TensorF16 = Tensor<uint16_t>;  // half
using TensorI32 = Tensor<int32_t>;
using TensorU32 = Tensor<uint32_t>;
using TensorU64 = Tensor<uint64_t>;
using TensorU8 = Tensor<uint8_t>;

} // namespace kinet::accel
