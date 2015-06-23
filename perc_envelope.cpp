#include "perc_envelope.h"
#include <cmath>
#include <stdexcept>

#include <iostream>

namespace emantFX{

perc_envelope::perc_envelope(double attack_ms,
                             double decay_ms,
                             double sustain,
                             double sustain_decay_ms,
                             double release_ms,
                             double tick_ms)
{
    if(tick_ms <= 0.0)
    {
        throw(std::invalid_argument("Negative or Zero Tick Size."));
        return;
    }
    attack_ms = (attack_ms > 0.0) ? attack_ms : tick_ms;
    decay_ms = (decay_ms > 0.0) ?  decay_ms : tick_ms;
    release_ms = (release_ms > 0.0) ? release_ms :  tick_ms;
    sustain = (sustain <= 1.0 && sustain >= 0.0) ? sustain : ((sustain > 1.0) ? 1.0 : 0.0);

    tick_size_ms = tick_ms;
    sustain_level = sustain;

    double d = std::ceil(attack_ms/tick_ms);
    attack_slope = 1.0/d;

    d = std::ceil(decay_ms/tick_ms);
    decay_slope = (sustain - 1.0)/d;

    d = std::ceil(sustain_decay_ms/tick_ms);
    sustain_slope = -sustain/d;

    d = std::ceil(release_ms/tick_ms);
    release_slope = -1.0/d;

    status = IDLE;
    ready = true;
    level = 0.0;


}

perc_envelope::~perc_envelope()
{
    pushed.clear();
}

void perc_envelope::tick(void)
{
    if(ready && !pushed.empty())
    {
        status = ATTACK;
        ready = false;
        level += attack_slope;
        if(level >= 1.0){
            level = 1.0;
            status = DECAY;
        }
    }
    else if (!ready && pushed.empty())
    {
        status = IDLE;
        level = 0.0;
        ready = true;
    }
    else
    {
        switch(status)
        {
            case ATTACK:
                level += attack_slope;
                if(level >= 1.0){
                    level = 1.0;
                    status = DECAY;
                }
                break;
            case DECAY :
                level += decay_slope;
                if(level <= sustain_level){
                    level = sustain_level;
                    status = SUSTAIN;
                }
                break;
            case SUSTAIN :
                level += sustain_slope;
                if(level <= 0.0){
                    level = 0.0;
                    status = IDLE;
                }
                break;
            case RELEASE:
                level += release_slope;
                if(level <= 0.0){
                    level = 0.0;
                    status = IDLE;
                }
                break;
            default :
                break;
        }
    }

}

} //namespace emantFX
