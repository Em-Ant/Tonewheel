#include <cstddef>
#include <memory>

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
  T read_at(size_t pos) const;
};

template <typename T>
circular_buffer<T>::circular_buffer(size_t buffer_size)
    : buffer(new T[buffer_size]{0}),
      buffer_size(buffer_size),
      write_index(0) {}

template <typename T>
T circular_buffer<T>::write(const T &input) noexcept
{
  T old_sample = buffer[write_index];
  buffer[write_index] = input;

  if (++write_index == buffer_size)
  {
    write_index = 0;
  }

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
  if (delay >= buffer_size)
  {
    throw std::runtime_error("circular buffer delay index too large");
  }
  size_t i = (delay > write_index - 1)
                 ? write_index - 1 + buffer_size - delay
                 : write_index - 1 - delay;

  return buffer[i];
}
