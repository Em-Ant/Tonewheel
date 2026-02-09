#include <cstddef>
#include <vector>

#include "../utils/circular_buffer.h"

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
  circular_buffer<double> buffer;
  double lp_filter_1 = 0;

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
  circular_buffer<double> buffer;

public:
  all_pass_filter(double gain, size_t buffer_size);
  virtual void process(double const *input,
                       double *output,
                       size_t n_samples) override;
};

class reverb
{
  // Separate filter banks for left and right channels
  std::vector<comb_filter> comb_filters_left;
  std::vector<comb_filter> comb_filters_right;
  std::vector<all_pass_filter> all_pass_filters_left;
  std::vector<all_pass_filter> all_pass_filters_right;

  size_t buffer_size;
  double dry;
  double wet;
  double room;
  double damping;

  // Cross-mix coefficients for stereo width (hardcoded at 15%)
  static constexpr double cross_mix = 0.15;

public:
  reverb(double room, double damping, double dry, double wet, size_t buffer_size);

  // Planar format: input[0..buffer_size-1] = left, input[buffer_size..2*buffer_size-1] = right
  // Same for output
  void process(const double *input, double *output);
};