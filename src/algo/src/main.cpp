// SYCL smoke-test: parallel Monte Carlo estimation of π.
//
// Each work-item independently samples a point in the unit square and checks
// whether it falls inside the unit circle. The partial hit-counts are reduced
// on the device via an atomic; the final count is copied back to the host to
// compute the π estimate. The same calculation is then repeated on the host
// so the two results can be compared side-by-side.
//
// Uses the SYCL 2020 USM model (`sycl::malloc_device` + in-order queue) rather
// than the buffer/accessor model to avoid the implicit synchronization overhead
// that the latter introduces.

#include <argus/utils/macro.hpp>

#include <sycl/sycl.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <numbers>
#include <print>
#include <span>
#include <string_view>

namespace {

constexpr uint32_t kSamples = ARGUS_BIT(24U);  // ~16 M samples
constexpr uint32_t kSeed = 0xDEAD'BEEFU;

// ---------------------------------------------------------------------------
// Minimal xoshiro32** — a fast, small-state PRNG suitable for GPU kernels.
// Each work-item receives a unique seed derived from its global id so the
// sequences are independent.
// ---------------------------------------------------------------------------

/// @brief Rotate-left helper used by the xoshiro scrambler.
/// @param value The value to rotate
/// @param shift The number of bits to rotate by
/// @return The rotated value
[[nodiscard]] constexpr uint32_t Rotl(uint32_t value, int shift) noexcept {
  return (value << shift) | (value >> (32 - shift));
}

/// @brief Single step of xoshiro128** returning the next pseudo-random value
/// and updating the four-word state in-place.
/// @param state Four-word state array; modified by each call
/// @return Next pseudo-random 32-bit value
[[nodiscard]] uint32_t XoshiroNext(std::span<uint32_t, 4> state) noexcept {
  const uint32_t result = Rotl(state[1] * 5U, 7U) * 9U;
  const uint32_t mixed = state[1] << 9U;

  state[2] ^= state[0];
  state[3] ^= state[1];
  state[1] ^= state[2];
  state[0] ^= state[3];
  state[2] ^= mixed;
  state[3] = Rotl(state[3], 11U);

  return result;
}

/// @brief Seed the xoshiro128** state from a single 32-bit value using
/// SplitMix32 so that nearby seeds produce uncorrelated sequences.
/// @param seed Scalar seed (typically the global work-item id + base seed)
/// @param out Four-word output state
void SeedXoshiro(uint32_t seed, std::span<uint32_t, 4> out) noexcept {
  // SplitMix32 — fills the state with high-quality bits.
  auto splitmix = [&]() noexcept -> uint32_t {
    seed += 0x9e3779b9U;
    uint32_t word = seed;
    word = (word ^ (word >> 16U)) * 0x85ebca6bU;
    word = (word ^ (word >> 13U)) * 0xc2b2ae35U;
    return word ^ (word >> 16U);
  };

  std::ranges::generate(out, splitmix);
}

/// @brief RAII wrapper that frees a USM device pointer on destruction.
/// @details Keeps the allocation and its queue together so the free always
/// targets the correct context without passing the queue separately.
class DeviceAlloc {
public:
  DeviceAlloc(sycl::queue& queue, size_t bytes)
      : queue_{queue}, ptr_{sycl::malloc_device(bytes, queue)} {}

  DeviceAlloc(DeviceAlloc&&) = default;
  DeviceAlloc(const DeviceAlloc&) = delete;
  ~DeviceAlloc() { sycl::free(ptr_, queue_); }

  DeviceAlloc& operator=(const DeviceAlloc&) = delete;
  DeviceAlloc& operator=(DeviceAlloc&&) = delete;

  [[nodiscard]] void* Ptr() noexcept { return ptr_; }

private:
  sycl::queue& queue_;
  void* ptr_ = nullptr;
};

/// @brief Count how many of `n_samples` random points in [0,1)² lie inside
/// the unit circle, using a USM device allocation and an atomic
/// reduction across all work-items.
/// @details Uses `sycl::malloc_device` so the counter lives on the GPU for the
/// entire kernel lifetime. A single `sycl::memcpy` transfers the scalar result
/// to the host after the kernel finishes. The queue is created in in-order
/// mode (the default) so no explicit event dependencies are needed between the
/// memset, kernel, and memcpy.
/// @param queue In-order SYCL queue targeting the chosen device
/// @param n_samples Total number of (x,y) pairs to test
/// @return Number of points that satisfied x²+y² < 1
[[nodiscard]] uint64_t CountHitsOnDevice(sycl::queue& queue,
                                         uint32_t n_samples) {
  DeviceAlloc alloc{queue, sizeof(uint64_t)};
  auto* device_hits = static_cast<uint64_t*>(alloc.Ptr());

  // Zero the counter on the device before launching the kernel.
  queue.memset(device_hits, 0, sizeof(uint64_t));

  queue.parallel_for(sycl::range<1>{n_samples}, [=](sycl::id<1> work_id) {
    // Give each work-item an independent PRNG state.
    std::array<uint32_t, 4> prng_state = {};
    SeedXoshiro(static_cast<uint32_t>(work_id.get(0)) + kSeed, prng_state);

    // Map two 32-bit integers to floats in [0, 1).
    constexpr float kInv32 = 1.0F / static_cast<float>(0xFFFF'FFFFU);
    const float coord_x = static_cast<float>(XoshiroNext(prng_state)) * kInv32;
    const float coord_y = static_cast<float>(XoshiroNext(prng_state)) * kInv32;

    if (coord_x * coord_x + coord_y * coord_y < 1.0F) {
      sycl::atomic_ref<uint64_t, sycl::memory_order::relaxed,
                       sycl::memory_scope::device,
                       sycl::access::address_space::global_space>
          atomic_hits{*device_hits};
      ++atomic_hits;
    }
  });

  // Copy the scalar result back to the host; wait() ensures the kernel and
  // memcpy have both completed before we return.
  uint64_t hit_count = 0;
  queue.memcpy(&hit_count, device_hits, sizeof(uint64_t)).wait();

  return hit_count;
}

/// @brief Same Monte Carlo estimate executed serially on the host.
/// @details Used to cross-check the device result and provide a timing
/// reference. The PRNG seeds are identical to the device kernel so the two
/// estimates converge to the same value.
/// @param n_samples Total number of (x,y) pairs to test
/// @return Number of points that satisfied x²+y² < 1
[[nodiscard]] uint64_t CountHitsOnHost(uint32_t n_samples) {
  uint64_t hits = 0;

  for (uint32_t sample_idx = 0; sample_idx < n_samples; ++sample_idx) {
    std::array<uint32_t, 4> prng_state = {};
    SeedXoshiro(sample_idx + kSeed, prng_state);

    constexpr float kInv32 = 1.0F / static_cast<float>(0xFFFF'FFFFU);
    const float coord_x = static_cast<float>(XoshiroNext(prng_state)) * kInv32;
    const float coord_y = static_cast<float>(XoshiroNext(prng_state)) * kInv32;

    if ((coord_x * coord_x) + (coord_y * coord_y) < 1.0F) {
      ++hits;
    }
  }

  return hits;
}

/// @brief Format a duration in milliseconds to a human-readable string.
/// @param ms Duration in milliseconds
/// @return Formatted string
[[nodiscard]] std::string FormatMs(double ms) {
  return std::format("{:.2f} ms", ms);
}

/// @brief Compute π estimate from a hit count and total sample count.
/// @param hits Number of hits
/// @param total Total number of samples
/// @return π estimate
[[nodiscard]] constexpr double EstimatePi(uint64_t hits,
                                          uint32_t total) noexcept {
  return 4.0 * static_cast<double>(hits) / static_cast<double>(total);
}

/// @brief Print a labelled result row to stdout.
/// @param label Result label
/// @param pi_estimate π estimate
/// @param elapsed_ms Elapsed time in milliseconds
/// @param hits Number of hits
void PrintResult(std::string_view label, double pi_estimate, double elapsed_ms,
                 uint64_t hits) {
  const double error_ppm =
      std::abs(pi_estimate - std::numbers::pi) / std::numbers::pi * 1'000'000.0;

  std::println(
      "  {:<10}  π ≈ {:.8f}  |  error {:7.2f} ppm  |  hits {:>10}  |  {}",
      label, pi_estimate, error_ppm, hits, FormatMs(elapsed_ms));
}

}  // namespace

int main() {
  std::println("╔══════════════════════════════════════════════════╗");
  std::println("║  argus / SYCL smoke-test: Monte Carlo π          ║");
  std::println("╚══════════════════════════════════════════════════╝\n");

  // ── Device selection ────────────────────────────────────────────────────
  sycl::queue queue{sycl::default_selector_v};
  const auto device = queue.get_device();

  std::println("  Device  : {}", device.get_info<sycl::info::device::name>());
  std::println("  Vendor  : {}", device.get_info<sycl::info::device::vendor>());
  std::println("  Version : {}",
               device.get_info<sycl::info::device::version>());
  std::println("  Samples : {}\n", kSamples);

  // ── Warm-up: trigger JIT for the real kernel shape before timing ─────────
  // Submit a minimal parallel_for with the same code path so AdaptiveCpp
  // compiles and caches the kernel binary before the timed run starts.
  {
    uint64_t dummy_hits = 0;
    DeviceAlloc warmup_alloc{queue, sizeof(uint64_t)};
    auto* warmup_ptr = static_cast<uint64_t*>(warmup_alloc.Ptr());
    queue.memset(warmup_ptr, 0, sizeof(uint64_t));
    queue.parallel_for(sycl::range<1>{1}, [=](sycl::id<1> work_id) {
      std::array<uint32_t, 4> prng_state = {};
      SeedXoshiro(static_cast<uint32_t>(work_id.get(0)) + kSeed, prng_state);
      constexpr float kInv32 = 1.0F / static_cast<float>(0xFFFF'FFFFU);
      const float coord_x =
          static_cast<float>(XoshiroNext(prng_state)) * kInv32;
      const float coord_y =
          static_cast<float>(XoshiroNext(prng_state)) * kInv32;
      if ((coord_x * coord_x) + (coord_y * coord_y) < 1.0F) {
        sycl::atomic_ref<uint64_t, sycl::memory_order::relaxed,
                         sycl::memory_scope::device,
                         sycl::access::address_space::global_space>
            atomic_hits{*warmup_ptr};
        ++atomic_hits;
      }
    });
    queue.memcpy(&dummy_hits, warmup_ptr, sizeof(uint64_t)).wait();
  }

  std::println("  Running kernels…\n");

  // ── Device run ──────────────────────────────────────────────────────────
  const auto device_start = std::chrono::steady_clock::now();
  const uint64_t device_hits = CountHitsOnDevice(queue, kSamples);
  const auto device_end = std::chrono::steady_clock::now();
  const double device_ms =
      std::chrono::duration<double, std::milli>(device_end - device_start)
          .count();

  // ── Host run ────────────────────────────────────────────────────────────
  const auto host_start = std::chrono::steady_clock::now();
  const uint64_t host_hits = CountHitsOnHost(kSamples);
  const auto host_end = std::chrono::steady_clock::now();
  const double host_ms =
      std::chrono::duration<double, std::milli>(host_end - host_start).count();

  // ── Results ─────────────────────────────────────────────────────────────
  std::println("  Results");
  std::println("  ───────────────────────────────────────────────────────");
  PrintResult("device", EstimatePi(device_hits, kSamples), device_ms,
              device_hits);
  PrintResult("host", EstimatePi(host_hits, kSamples), host_ms, host_hits);
  std::println("  ───────────────────────────────────────────────────────\n");

  std::println("  Speedup : {:.1f}×  (host / device wall-clock)",
               host_ms / device_ms);

  // ── Sanity check ────────────────────────────────────────────────────────
  //
  // The device and host sequences use identical seeds, so they sample the
  // exact same points and must agree on the hit count.
  if (device_hits != host_hits) {
    std::println(stderr, "\n  [FAIL] Hit counts differ — device: {}  host: {}",
                 device_hits, host_hits);
    return 1;
  }

  std::println("\n  [PASS] Device and host hit counts agree.");
  return 0;
}
