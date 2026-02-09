#pragma once

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
  T read_at(size_t delay) const;                      // integer delay
  T read_at(double delay_samples) const;              // fractional delay (NEW)
  T read_s(double delay_s, double sample_rate) const; // convenience wrapper
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

template <typename T>
T circular_buffer<T>::read_at(size_t delay) const
{
  if (delay < 0 || delay >= buffer_size)
  {
    throw std::runtime_error("circular buffer delay index out of range");
  }
  size_t idx = (write_index - 1 - delay + buffer_size) % buffer_size;
  return buffer[idx];
}

template <typename T>
T circular_buffer<T>::read_at(double delay) const
{
  size_t delay_int = static_cast<size_t>(delay);
  double frac = delay - static_cast<double>(delay_int);

  T l_val = read_at(delay_int);
  T h_val = read_at(delay_int + 1);

  return l_val + static_cast<T>(frac) * (h_val - l_val);
}

template <typename T>
T circular_buffer<T>::read_s(double delay_s, double sample_rate) const
{
  double delay_samples = delay_s * sample_rate;
  return read_at(delay_samples);
}
