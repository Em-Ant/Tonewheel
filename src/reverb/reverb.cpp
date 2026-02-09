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

  // Comb filter delay times - offset by small primes for stereo width
  // Left channel (original times)
  comb_filters_left.push_back(comb_filter(room, damping, 1557));
  comb_filters_left.push_back(comb_filter(room, damping, 1617));
  comb_filters_left.push_back(comb_filter(room, damping, 1491));
  comb_filters_left.push_back(comb_filter(room, damping, 1422));
  comb_filters_left.push_back(comb_filter(room, damping, 1277));
  comb_filters_left.push_back(comb_filter(room, damping, 1356));
  comb_filters_left.push_back(comb_filter(room, damping, 1188));
  comb_filters_left.push_back(comb_filter(room, damping, 1116));

  // Right channel (offset by 1-3 samples)
  comb_filters_right.push_back(comb_filter(room, damping, 1559));
  comb_filters_right.push_back(comb_filter(room, damping, 1619));
  comb_filters_right.push_back(comb_filter(room, damping, 1493));
  comb_filters_right.push_back(comb_filter(room, damping, 1423));
  comb_filters_right.push_back(comb_filter(room, damping, 1279));
  comb_filters_right.push_back(comb_filter(room, damping, 1357));
  comb_filters_right.push_back(comb_filter(room, damping, 1189));
  comb_filters_right.push_back(comb_filter(room, damping, 1117));

  // All-pass filters - left channel
  all_pass_filters_left.push_back(all_pass_filter(all_pass_gain, 225));
  all_pass_filters_left.push_back(all_pass_filter(all_pass_gain, 556));
  all_pass_filters_left.push_back(all_pass_filter(all_pass_gain, 441));
  all_pass_filters_left.push_back(all_pass_filter(all_pass_gain, 341));

  // All-pass filters - right channel (offset for width)
  all_pass_filters_right.push_back(all_pass_filter(all_pass_gain, 227));
  all_pass_filters_right.push_back(all_pass_filter(all_pass_gain, 559));
  all_pass_filters_right.push_back(all_pass_filter(all_pass_gain, 443));
  all_pass_filters_right.push_back(all_pass_filter(all_pass_gain, 343));
};

void reverb::process(const double *input, double *output)
{
  // Planar format: input[0..buffer_size-1] = left, input[buffer_size..2*buffer_size-1] = right
  const double *input_left = input;
  const double *input_right = input + buffer_size;
  double *output_left = output;
  double *output_right = output + buffer_size;

  // Temporary buffers for reverb processing
  std::vector<double> temp_left(buffer_size, 0.0);
  std::vector<double> temp_right(buffer_size, 0.0);
  std::vector<double> cross_left(buffer_size);
  std::vector<double> cross_right(buffer_size);

  // Apply cross-mix for stereo width
  // L_mix = L_in + cross_mix * R_in
  // R_mix = R_in + cross_mix * L_in
  for (size_t i = 0; i < buffer_size; i++)
  {
    cross_left[i] = input_left[i] + cross_mix * input_right[i];
    cross_right[i] = input_right[i] + cross_mix * input_left[i];
  }

  // Process parallel comb filters for left channel
  for (comb_filter &comb : comb_filters_left)
  {
    comb.process(cross_left.data(), temp_left.data(), buffer_size);
  }

  // Process parallel comb filters for right channel
  for (comb_filter &comb : comb_filters_right)
  {
    comb.process(cross_right.data(), temp_right.data(), buffer_size);
  }

  // Process series all-pass filters for left channel
  for (all_pass_filter &all_pass : all_pass_filters_left)
  {
    all_pass.process(temp_left.data(), temp_left.data(), buffer_size);
  }

  // Process series all-pass filters for right channel
  for (all_pass_filter &all_pass : all_pass_filters_right)
  {
    all_pass.process(temp_right.data(), temp_right.data(), buffer_size);
  }

  // Normalize and mix
  double scale = 1.0 / comb_filters_left.size();

  for (size_t i = 0; i < buffer_size; i++)
  {
    double wet_left = temp_left[i] * scale * wet;
    double wet_right = temp_right[i] * scale * wet;
    double dry_left = input_left[i] * dry;
    double dry_right = input_right[i] * dry;

    output_left[i] = dry_left + wet_left;
    output_right[i] = dry_right + wet_right;
  }
}
