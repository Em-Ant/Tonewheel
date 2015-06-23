#ifndef PERC_ENVELOPE_H
#define PERC_ENVELOPE_H

#include <list>
#include "adsr.h"

namespace emantFX{
class perc_envelope
{
    public:
        perc_envelope(double attack_ms,double decay_ms,double sustain,double sustain_decay_ms,double release_ms, double tick_ms);
        virtual ~perc_envelope();
        void tick(void);
        inline void key_pressed(short key) {pushed.push_back(key);}
        inline void key_released(short key) {pushed.remove(key);}
        inline double get_level(void){return level;}
    private:
        double level;
        double tick_size_ms;
        double attack_slope;
        double decay_slope;
        double sustain_level;
        double sustain_slope;
        double release_slope;
        std::list<short> pushed;
        adsr_status_t status;
        bool ready;

};
}
#endif // PERC_ENVELOPE_H
