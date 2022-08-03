#include "reverb.h"

comb_filter::comb_filter(double gain, double damping, size_t buffer_size)
    : gain(gain),
      damping(damping),
      buffer_size(buffer_size),
      buffer(circular_buffer<double>(buffer_size)),
      lp_filter_1(0){};

/**
 * @brief
 * H_filtered_comb(z) = z^-N/(1-g*[H_lp(z)]*z^-N),
 * H_lp(z) = (1-d)/(1-d*z^-1) <- First Order LowPass
 *
 * y[n] = g * yf[n] + x[n-N]
 * yf[n] = yf[n-1]*d +(1-d)*xf[n] -> xf[n] = y[n-N]
 *
 * (implemented in direct form II)
 *
 * see https://ccrma.stanford.edu/~jos/pasp/
 *
 * @param input
 * @param output
 * @param n_samples
 */
void comb_filter::process(double const *input,
                          double *output,
                          size_t n_samples)
{
  for (size_t i = 0; i < n_samples; i++)
  {
    double t_N = buffer.read_at_N();
    lp_filter_1 = damping * lp_filter_1 + (1 - damping) * t_N;
    double t = lp_filter_1 * gain + input[i];
    output[i] += t_N + t;
    buffer.write(t);
  }
};

all_pass_filter::all_pass_filter(double gain, size_t buffer_size)
    : gain(gain),
      buffer_size(buffer_size),
      buffer(circular_buffer<double>(buffer_size)){};

/**
 * @brief
 * H_AP(z)   = (-g + z^-N)/(1-g*z^-N)
 * y[n] = g * y[n-N] + x[n-N] - g*x[n]
 *
 * (implemented in direct form II)
 *
 * see https://ccrma.stanford.edu/~jos/pasp/
 *
 * @param input
 * @param output
 * @param n_samples
 */
void all_pass_filter::process(double const *input,
                              double *output,
                              size_t n_samples)
{
  for (size_t i = 0; i < n_samples; i++)
  {
    double t_N = buffer.read_at_N();
    double t = t_N * gain + input[i];
    output[i] = gain * (t - t_N);
    buffer.write(t);
  }
};

reverb::reverb(double room, double damping,
               double dry, double wet, size_t buffer_size)
    : buffer_size(buffer_size), dry(dry), wet(wet),
      room(room), damping(damping)
{
  constexpr double all_pass_gain = 0.5;
  comb_filters.push_back(comb_filter(room, damping, 1557));
  comb_filters.push_back(comb_filter(room, damping, 1617));
  comb_filters.push_back(comb_filter(room, damping, 1491));
  comb_filters.push_back(comb_filter(room, damping, 1422));
  comb_filters.push_back(comb_filter(room, damping, 1277));
  comb_filters.push_back(comb_filter(room, damping, 1356));
  comb_filters.push_back(comb_filter(room, damping, 1188));
  comb_filters.push_back(comb_filter(room, damping, 1116));

  all_pass_filters.push_back(all_pass_filter(all_pass_gain, 225));
  all_pass_filters.push_back(all_pass_filter(all_pass_gain, 556));
  all_pass_filters.push_back(all_pass_filter(all_pass_gain, 441));
  all_pass_filters.push_back(all_pass_filter(all_pass_gain, 341));
};

void reverb::process(const double *input_mono, double *out_left, double *out_right)
{
  // parallel comb filters
  // out_left is used as temp buffer to sum all comb responses
  for (comb_filter &comb : comb_filters)
  {
    comb.process(input_mono, out_right, buffer_size);
  }
  // series all pass filters
  for (all_pass_filter &all_pass : all_pass_filters)
  {
    all_pass.process(out_right, out_right, buffer_size);
  }

  // normalize
  double scale = 1.0 / comb_filters.size();

  // mixer
  for (size_t i = 0; i < buffer_size; i++)
  {
    double _dry = input_mono[i] * dry;
    double _wet = out_right[i] * scale * wet;

    out_left[i] = _dry + _wet;
    out_right[i] = _dry - _wet;
  }
}
