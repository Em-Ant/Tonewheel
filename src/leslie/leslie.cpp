#include "leslie.h"
#include "../utils/circular_buffer.h"
#include <algorithm>
#include <cmath>
#include <iostream>

constexpr double SPEED_OF_SOUND = 343.0;
constexpr size_t MAX_DELAY_SAMPLES = 500; // Maximum delay in samples

leslie_simulator::leslie_simulator(double sample_rate)
    : sample_rate(sample_rate),
      horn_radius(0.12),
      drum_radius(0.18),
      mic_distance(0.8),
      mic_angle(M_PI / 6),
      current_horn_angle(0.0),
      current_drum_angle(0.0),
      current_horn_speed(0.0),
      current_drum_speed(0.0),
      target_horn_speed(0.0),
      target_drum_speed(0.0),
      accel_time(1.8),
      decel_time(2.5),
      horn_delay(new circular_buffer<double>(static_cast<size_t>(500))),
      drum_delay(new circular_buffer<double>(static_cast<size_t>(500))),
      horn_crossover_freq(800.0),
      drum_crossover_freq(800.0)
{
    set_leslie_122_preset();

    // Initialize crossover filters
    horn_highpass.set_highpass(horn_crossover_freq, sample_rate);
    drum_lowpass.set_lowpass(drum_crossover_freq, sample_rate);
}

void leslie_simulator::set_leslie_122_preset()
{
    horn_radius = 0.12;
    drum_radius = 0.18;
    mic_distance = 0.8;
    mic_angle = M_PI / 6;
}

void leslie_simulator::set_physical_parameters(double horn_radius, double drum_radius,
                                               double mic_dist, double mic_angle)
{
    this->horn_radius = horn_radius;
    this->drum_radius = drum_radius;
    this->mic_distance = mic_dist;
    this->mic_angle = mic_angle;
}

void leslie_simulator::set_fixed_speeds(double horn_rpm, double drum_rpm)
{
    target_horn_speed = horn_rpm * 2.0 * M_PI / 60.0;
    target_drum_speed = drum_rpm * 2.0 * M_PI / 60.0;
}

void leslie_simulator::stop()
{
    target_horn_speed = 0.0;
    target_drum_speed = 0.0;
}

double leslie_simulator::calculate_delay_samples(
    double angle, double radius, double mic_dist, double mic_ang) const
{
    double dx = radius * std::cos(angle) - mic_dist * std::cos(mic_ang);
    double dy = radius * std::sin(angle) - mic_dist * std::sin(mic_ang);
    double distance = std::sqrt(dx * dx + dy * dy);

    return (distance / SPEED_OF_SOUND) * sample_rate;
}

double leslie_simulator::apply_tone_filter(double input, double angle)
{
    double direction = std::cos(angle - mic_angle);
    double brightness = 0.5 * (1.0 + direction);
    double cutoff_hz = min_horn_cutoff + brightness * (max_horn_cutoff - min_horn_cutoff);
    cutoff_hz = std::max(200.0, std::min(cutoff_hz, sample_rate * 0.45));

    double fc = cutoff_hz / sample_rate;
    double alpha = 1.0 / (1.0 + 1.0 / (2.0 * M_PI * fc));

    horn_tone_state = alpha * horn_tone_state + (1.0 - alpha) * input;
    return horn_tone_state;
}

void leslie_simulator::process(const double *input, double *output_left, double *output_right, size_t n_samples)
{
    double time_step = 1.0 / sample_rate;

    for (size_t i = 0; i < n_samples; i++)
    {
        // Update speeds towards targets (physics-based acceleration/deceleration)
        double delta_horn = target_horn_speed - current_horn_speed;
        if (delta_horn != 0.0)
        {
            double rate = (delta_horn > 0.0) ? (1.0 / accel_time) : (1.0 / decel_time);
            current_horn_speed += delta_horn * rate * time_step;
            // Clamp to prevent overshoot
            if ((delta_horn > 0.0 && current_horn_speed > target_horn_speed) ||
                (delta_horn < 0.0 && current_horn_speed < target_horn_speed))
            {
                current_horn_speed = target_horn_speed;
            }
        }

        double delta_drum = target_drum_speed - current_drum_speed;
        if (delta_drum != 0.0)
        {
            double rate = (delta_drum > 0.0) ? (1.0 / accel_time) : (1.0 / decel_time);
            current_drum_speed += delta_drum * rate * time_step;
            // Clamp to prevent overshoot
            if ((delta_drum > 0.0 && current_drum_speed > target_drum_speed) ||
                (delta_drum < 0.0 && current_drum_speed < target_drum_speed))
            {
                current_drum_speed = target_drum_speed;
            }
        }

        // Update rotation angles with current speeds
        current_horn_angle += current_horn_speed * time_step;
        current_drum_angle += current_drum_speed * time_step;

        // Wrap angles to [0, 2π)
        current_horn_angle = std::fmod(current_horn_angle, 2.0 * M_PI);
        if (current_horn_angle < 0)
            current_horn_angle += 2.0 * M_PI;

        current_drum_angle = std::fmod(current_drum_angle, 2.0 * M_PI);
        if (current_drum_angle < 0)
            current_drum_angle += 2.0 * M_PI;

        // Apply crossover filtering
        double horn_input = horn_highpass.process(input[i]);
        double drum_input = drum_lowpass.process(input[i]);

        // Calculate and smooth delay times
        double horn_delay_samples = calculate_delay_samples(current_horn_angle, horn_radius, mic_distance, mic_angle);
        double drum_delay_samples = calculate_delay_samples(current_drum_angle, drum_radius, mic_distance, mic_angle);

        // Write to delay lines
        horn_delay->write(horn_input);
        drum_delay->write(drum_input);

        // Read delayed signals with interpolation
        double horn_out = horn_delay->read_at(horn_delay_samples);
        double drum_out = drum_delay->read_at(drum_delay_samples);

        horn_out = apply_tone_filter(horn_out, current_horn_angle);

        double horn_amp = 0.7 + 0.3 * std::cos(current_horn_angle - mic_angle);
        double drum_amp = 0.9 + 0.1 * std::cos(current_drum_angle - mic_angle);

        horn_out *= horn_amp;
        drum_out *= drum_amp;

        // Mix outputs (attenuated to prevent clipping)
        output_left[i] = horn_out * 0.5 + drum_out * 0.4;
        output_right[i] = horn_out * 0.5 + drum_out * 0.4;
    }
}
