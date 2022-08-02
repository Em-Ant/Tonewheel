#include <cstddef>
#include <vector>

#include "circular_buffer.h"

struct filter
{
  inline virtual void process(double const *input,
                              double *output,
                              size_t n_samples) = 0;
};

class comb_filter final : public filter
{
  double gain;
  double damping;
  size_t buffer_size;
  circular_buffer<double> y_buffer;
  circular_buffer<double> x_buffer;
  double lp_filter_memo = 0;

public:
  comb_filter(double gain, double damping, size_t buffer_size);
  virtual void process(double const *input,
                       double *output,
                       size_t n_samples) override;
};

class all_pass_filter final : public filter
{
  double gain;
  size_t buffer_size;
  circular_buffer<double> y_buffer;
  circular_buffer<double> x_buffer;

public:
  all_pass_filter(double gain, size_t buffer_size);
  virtual void process(double const *input,
                       double *output,
                       size_t n_samples) override;
};

class reverb
{
  std::vector<comb_filter> comb_filters;
  std::vector<all_pass_filter> all_pass_filters;
  size_t buffer_size;
  double dry;
  double wet;
  double room;
  double damping;

public:
  reverb(double room, double damping, double dry, double wet, size_t buffer_size);
  void process(double const *input_mono, double *output_left, double *output_right);
};