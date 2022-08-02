#include <cstddef>
#include <memory>

template <typename T>
class circular_buffer final
{
private:
  std::unique_ptr<T[]> buffer;
  size_t buffer_size;
  size_t write_index;

public:
  circular_buffer(size_t buffer_size);
  T write(const T &input);
  T read_at(size_t pos) const;
};

template <typename T>
circular_buffer<T>::circular_buffer(size_t buffer_size)
    : buffer(new T[buffer_size]),
      buffer_size(buffer_size),
      write_index(0) {}

template <typename T>
T circular_buffer<T>::write(const T &input)
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
T circular_buffer<T>::read_at(size_t delay) const
{
  size_t i = (delay > write_index)
                 ? write_index + buffer_size - delay
                 : write_index - delay;

  return buffer[i];
}
