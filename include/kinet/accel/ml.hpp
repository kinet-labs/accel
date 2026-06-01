// kinet/accel/ml.hpp - ML Acceleration Operations

#pragma once

#include <kinet/accel/tensor.hpp>

namespace kinet::accel::ml {

/// ML operations interface
class Ops {
public:
    explicit Ops(gpu::Device& device, gpu::Queue& queue);
    ~Ops();

    // Matrix operations
    gpu::Status matmul(const TensorF32& a, const TensorF32& b, TensorF32& c);
    gpu::Status matmul_transposed(const TensorF32& a, const TensorF32& b, TensorF32& c, bool transA, bool transB);
    gpu::Status batch_matmul(const TensorF32& a, const TensorF32& b, TensorF32& c);

    // Activation functions
    gpu::Status relu(const TensorF32& input, TensorF32& output);
    gpu::Status gelu(const TensorF32& input, TensorF32& output);
    gpu::Status silu(const TensorF32& input, TensorF32& output);
    gpu::Status sigmoid(const TensorF32& input, TensorF32& output);
    gpu::Status tanh(const TensorF32& input, TensorF32& output);
    gpu::Status softmax(const TensorF32& input, TensorF32& output, int axis = -1);

    // Normalization
    gpu::Status layer_norm(const TensorF32& input, const TensorF32& gamma, const TensorF32& beta,
                           TensorF32& output, float eps = 1e-5f);
    gpu::Status rms_norm(const TensorF32& input, const TensorF32& gamma, TensorF32& output, float eps = 1e-5f);
    gpu::Status batch_norm(const TensorF32& input, const TensorF32& mean, const TensorF32& var,
                           const TensorF32& gamma, const TensorF32& beta,
                           TensorF32& output, float eps = 1e-5f);

    // Attention
    gpu::Status attention(const TensorF32& q, const TensorF32& k, const TensorF32& v,
                          TensorF32& output, float scale = 0.0f);
    gpu::Status flash_attention(const TensorF32& q, const TensorF32& k, const TensorF32& v,
                                TensorF32& output, float scale = 0.0f);
    gpu::Status multi_head_attention(const TensorF32& q, const TensorF32& k, const TensorF32& v,
                                     const TensorF32& wo, TensorF32& output, int num_heads);

    // Convolutions
    gpu::Status conv2d(const TensorF32& input, const TensorF32& weight, TensorF32& output,
                       int padding = 0, int stride = 1);
    gpu::Status depthwise_conv2d(const TensorF32& input, const TensorF32& weight, TensorF32& output,
                                 int padding = 0, int stride = 1);

    // Pooling
    gpu::Status max_pool2d(const TensorF32& input, TensorF32& output, int kernel_size, int stride = 0);
    gpu::Status avg_pool2d(const TensorF32& input, TensorF32& output, int kernel_size, int stride = 0);

    // Embedding
    gpu::Status embedding(const TensorI32& indices, const TensorF32& weight, TensorF32& output);
    gpu::Status rotary_embedding(TensorF32& q, TensorF32& k, const TensorF32& freqs, int offset = 0);

    // Reduction
    gpu::Status reduce_sum(const TensorF32& input, TensorF32& output, int axis = -1);
    gpu::Status reduce_mean(const TensorF32& input, TensorF32& output, int axis = -1);
    gpu::Status reduce_max(const TensorF32& input, TensorF32& output, int axis = -1);
    gpu::Status argmax(const TensorF32& input, TensorI32& output, int axis = -1);

    // Elementwise
    gpu::Status add(const TensorF32& a, const TensorF32& b, TensorF32& c);
    gpu::Status multiply(const TensorF32& a, const TensorF32& b, TensorF32& c);
    gpu::Status scale(const TensorF32& input, float alpha, TensorF32& output);

    // Quantization
    gpu::Status quantize_int8(const TensorF32& input, TensorU8& output, TensorF32& scale);
    gpu::Status dequantize_int8(const TensorU8& input, const TensorF32& scale, TensorF32& output);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kinet::accel::ml
