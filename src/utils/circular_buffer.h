#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <cmath>

template <typename T>
class circular_buffer
{
private:
  std::unique_ptr<T[]> buffer;
  size_t buffer_size;
  size_t write_index;

public:
  circular_buffer(size_t buffer_size);
  T write(const T &input) noexcept;
  inline T read_at_N() const noexcept;
  T read_at(size_t delay) const;                        // integer delay
  T read_at(double delay_samples) const;                // fractional delay (NEW)
  T read_ms(double delay_ms, double sample_rate) const; // convenience wrapper
};

template <typename T>
circular_buffer<T>::circular_buffer(size_t buffer_size)
    : buffer(new T[buffer_size]{0}),
      buffer_size(buffer_size),
      write_index(0)
{
  if (buffer_size == 0)
    throw std::invalid_argument("Buffer size must be > 0");
}

template <typename T>
T circular_buffer<T>::write(const T &input) noexcept
{
  T old_sample = buffer[write_index];
  buffer[write_index] = input;
  write_index = (write_index + 1) % buffer_size;
  return old_sample;
}

template <typename T>
inline T circular_buffer<T>::read_at_N() const noexcept
{
  return buffer[write_index];
}

// Integer delay read (existing)
template <typename T>
T circular_buffer<T>::read_at(size_t delay) const
{
  if (delay >= buffer_size)
  {
    throw std::runtime_error("circular buffer delay index too large");
  }
  size_t idx = (write_index - 1 - delay + buffer_size) % buffer_size;
  return buffer[idx];
}

// NEW: Fractional delay read with linear interpolation
template <typename T>
T circular_buffer<T>::read_at(double delay_samples) const
{
  if (delay_samples < 0.0)
    delay_samples = 0.0;

  // Clamp to valid range [0, buffer_size - 1]
  double max_delay = static_cast<double>(buffer_size - 1);
  if (delay_samples > max_delay)
    delay_samples = max_delay;

  size_t delay_int = static_cast<size_t>(delay_samples);
  double frac = delay_samples - static_cast<double>(delay_int);

  // Helper to safely read at integer delay (handles wrap-around)
  auto read_sample = [this](size_t d) -> T
  {
    if (d >= buffer_size)
      d = buffer_size - 1;
    size_t idx = (write_index - 1 - d + buffer_size) % buffer_size;
    return buffer[idx];
  };

  T s1 = read_sample(delay_int);
  T s2 = read_sample(delay_int + 1);

  return s1 + static_cast<T>(frac) * (s2 - s1);
}

// Convenience method (uses new read_at)
template <typename T>
T circular_buffer<T>::read_ms(double delay_ms, double sample_rate) const
{
  double delay_samples = (delay_ms / 1000.0) * sample_rate;
  return read_at(delay_samples);
}

#endif // CIRCULAR_BUFFER_H