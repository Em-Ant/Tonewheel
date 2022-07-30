
/***************************************************
		Linear Envelope Generator ADSR
----------------------------------------------------
		Author 	:	Emant
		Date	:	2014
		Version	:	1.0alpha
---------------------------------------------------
***************************************************/

#include "adsr.h"
#include <stdexcept>
#include <cmath>

namespace emantFX{

adsr::adsr(double attack_ms,
           double decay_ms,
           double sustain,
           double release_ms,
           double tick_ms,
           adsr_percussion_type_t perc)
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

    percussion = perc;

    tick_size_ms = tick_ms;
    sustain_level = sustain;

    double d = std::ceil(attack_ms/tick_ms);
    attack_slope = 1.0/d;

    d = std::ceil(decay_ms/tick_ms);
    decay_slope = (percussion == DECAY_ONLY) ? -1.0/d : (sustain - 1.0)/d;

    d = std::ceil(release_ms/tick_ms);
    release_slope = -1.0/d;

    keyArray = new keynote[128];
    for( short i = 0; i< 128; i++)
        keyArray[i].note_number = i;
    velocity_slope = 1.0/127;
}

adsr::~adsr()
{
    delete[] keyArray;
}

void adsr::key_pressed(short note, short velocity)
{
    keyArray[note].velocity = velocity;
    keyArray[note].status = ATTACK ;
}

void adsr::key_released(short note, short velocity)
{

    if(percussion == NO_PERCUSSION)
    {
        keyArray[note].velocity = velocity;
        keyArray[note].status = RELEASE ;
    }
}


void adsr::tick(void)
{
    double decay_thres = 0.0;
    adsr_status_t after_decay = IDLE;
    switch (percussion)
    {
        case NO_PERCUSSION:
            decay_thres = sustain_level;
            after_decay = SUSTAIN;
            break;
        case DECAY_ONLY:
            decay_thres = 0.0;
            after_decay = IDLE;
            break;
        case DECAY_RELEASE:
            decay_thres = sustain_level;
            after_decay = RELEASE;
            break;
    }
    for(short i = 0; i < 128; i++)
    {
        switch (keyArray[i].status)
        {
            case ATTACK :
                keyArray[i].current_level += attack_slope;
                if(keyArray[i].current_level >= 1.0){
                    keyArray[i].current_level = 1.0;
                    keyArray[i].status = DECAY;
                }
                break;
            case DECAY :
                if(decay_slope == 0.0){
                    keyArray[i].status = after_decay;
                    break;
                }else{
                    keyArray[i].current_level += decay_slope;
                    if(keyArray[i].current_level <= decay_thres){
                        keyArray[i].current_level = decay_thres;
                        keyArray[i].status = after_decay;
                    }
                }
                break;
            case RELEASE :
                keyArray[i].current_level += release_slope;
                if(keyArray[i].current_level <= 0.0){
                    keyArray[i].current_level = 0.0;
                    keyArray[i].status = IDLE;
                }
                break;
            default:
                break;
            }
    }
}





} //emantFX
