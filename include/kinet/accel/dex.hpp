// kinet/accel/dex.hpp - DEX/Trading Acceleration Operations

#pragma once

#include <kinet/accel/tensor.hpp>

namespace kinet::accel::dex {

/// DEX/Trading operations interface
class Ops {
public:
    explicit Ops(gpu::Device& device, gpu::Queue& queue);
    ~Ops();

    // Order matching
    gpu::Status match_orders(const TensorF32& bids, const TensorF32& asks,
                             TensorI32& matches, TensorF32& prices, TensorF32& amounts);
    gpu::Status sort_orderbook(TensorF32& prices, TensorF32& amounts, bool ascending);
    gpu::Status aggregate_levels(const TensorF32& prices, const TensorF32& amounts,
                                 TensorF32& agg_prices, TensorF32& agg_amounts, float tick_size);

    // AMM calculations
    gpu::Status constant_product_swap(const TensorF32& reserve_x, const TensorF32& reserve_y,
                                      const TensorF32& amount_in, bool x_to_y,
                                      TensorF32& amount_out, float fee = 0.003f);
    gpu::Status stable_swap(const TensorF32& reserves, const TensorF32& amount_in,
                            int token_in, int token_out, TensorF32& amount_out,
                            float amp = 100.0f, float fee = 0.0004f);
    gpu::Status concentrated_liquidity(const TensorF32& liquidity, const TensorF32& sqrt_price,
                                       const TensorF32& tick_lower, const TensorF32& tick_upper,
                                       const TensorF32& amount_in, bool x_to_y,
                                       TensorF32& amount_out);

    // Price calculations
    gpu::Status compute_twap(const TensorF32& prices, const TensorU64& timestamps,
                             uint64_t start_time, uint64_t end_time, TensorF32& twap);
    gpu::Status compute_vwap(const TensorF32& prices, const TensorF32& volumes,
                             TensorF32& vwap);
    gpu::Status compute_ema(const TensorF32& prices, float alpha, TensorF32& ema);
    gpu::Status compute_bollinger(const TensorF32& prices, int period, float num_std,
                                  TensorF32& upper, TensorF32& middle, TensorF32& lower);

    // Arbitrage detection
    gpu::Status detect_arbitrage(const TensorF32& exchange_matrix, TensorI32& path,
                                 TensorF32& profit);
    gpu::Status bellman_ford(const TensorF32& graph, int source, TensorF32& distances,
                             TensorI32& predecessors, bool& negative_cycle);

    // Liquidation calculations
    gpu::Status compute_health_factor(const TensorF32& collateral_values,
                                      const TensorF32& collateral_factors,
                                      const TensorF32& debt_values,
                                      TensorF32& health_factors);
    gpu::Status find_liquidatable(const TensorF32& health_factors, float threshold,
                                  TensorI32& indices, size_t& count);
    gpu::Status compute_liquidation_bonus(const TensorF32& debt_to_cover,
                                          const TensorF32& collateral_price,
                                          float bonus_rate, TensorF32& collateral_seized);

    // Risk calculations
    gpu::Status compute_var(const TensorF32& returns, float confidence,
                            TensorF32& value_at_risk);
    gpu::Status compute_cvar(const TensorF32& returns, float confidence,
                             TensorF32& conditional_var);
    gpu::Status compute_sharpe(const TensorF32& returns, float risk_free_rate,
                               TensorF32& sharpe_ratio);
    gpu::Status monte_carlo_paths(const TensorF32& initial_prices, float mu, float sigma,
                                  int num_steps, int num_paths, TensorF32& paths);

    // Portfolio optimization
    gpu::Status mean_variance_optimize(const TensorF32& returns, const TensorF32& cov_matrix,
                                       float target_return, TensorF32& weights);
    gpu::Status compute_efficient_frontier(const TensorF32& returns, const TensorF32& cov_matrix,
                                           int num_points, TensorF32& frontier_returns,
                                           TensorF32& frontier_volatility);

    // Batch transaction validation
    gpu::Status validate_balances(const TensorF32& balances, const TensorF32& amounts,
                                  TensorU8& valid);
    gpu::Status apply_transfers(TensorF32& balances, const TensorI32& from_indices,
                                const TensorI32& to_indices, const TensorF32& amounts);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kinet::accel::dex
