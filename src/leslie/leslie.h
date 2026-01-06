#ifndef LESLIE_H
#define LESLIE_H

#include <cmath>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Use the existing circular_buffer - declare it here
template <typename T>
class circular_buffer;

// Simple one-pole filter for crossover
class one_pole_filter
{
private:
    double a0, b1;
    double z1;

public:
    one_pole_filter() : a0(1.0), b1(0.0), z1(0.0) {}

    void set_lowpass(double freq, double sample_rate)
    {
        double omega = 2.0 * M_PI * freq / sample_rate;
        b1 = exp(-omega);
        a0 = 1.0 - b1;
        z1 = 0.0;
    }

    void set_highpass(double freq, double sample_rate)
    {
        double omega = 2.0 * M_PI * freq / sample_rate;
        b1 = exp(-omega);
        a0 = (1.0 + b1) / 2.0;
        z1 = 0.0;
    }

    double process(double input)
    {
        double output = a0 * input + b1 * z1;
        z1 = output;
        return output;
    }
};

class leslie_simulator
{
private:
    double sample_rate;

    // Physical parameters
    double horn_radius;
    double drum_radius;
    double mic_distance;
    double mic_angle;

    // Rotation state
    double current_horn_angle;
    double current_drum_angle;
    double current_horn_speed; // rad/s
    double current_drum_speed; // rad/s
    double target_horn_speed;  // rad/s
    double target_drum_speed;  // rad/s
    double accel_time;         // seconds
    double decel_time;         // seconds

    // Audio processing
    std::unique_ptr<circular_buffer<double>> horn_delay;
    std::unique_ptr<circular_buffer<double>> drum_delay;

    one_pole_filter horn_highpass;
    one_pole_filter drum_lowpass;

    double horn_crossover_freq;
    double drum_crossover_freq;

    double horn_tone_state = 0.0;

    // Tone filter parameters (you can make these configurable later)
    double min_horn_cutoff = 3000.0;  // Hz when horn faces cabinet
    double max_horn_cutoff = 12000.0; // Hz when horn faces mic

    // Physics update
    double calculate_delay_samples(
        double angle, double radius, double mic_dist, double mic_ang) const;
    double apply_tone_filter(double input, double angle);

public:
    leslie_simulator(double sample_rate);

    void set_leslie_122_preset();
    void set_physical_parameters(double horn_radius, double drum_radius,
                                 double mic_dist, double mic_angle);
    void set_fixed_speeds(double horn_rpm, double drum_rpm);
    void stop();

    void process(const double *input, double *output_left, double *output_right, size_t n_samples);
};

#endif // LESLIE_H
