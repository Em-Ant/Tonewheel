
/***************************************************
		Linear Envelope Generator ADSR
----------------------------------------------------
		Author 	:	Emant
		Date	:	2014
		Version	:	1.0alpha
---------------------------------------------------
***************************************************/

#ifndef MIDI_KEYBOARD_H_INCLUDED
#define MIDI_KEYBOARD_H_INCLUDED


namespace emantFX {

enum adsr_status_t{ATTACK,DECAY,SUSTAIN,RELEASE,IDLE};

enum adsr_percussion_type_t{NO_PERCUSSION,DECAY_ONLY,DECAY_RELEASE};

class adsr;

class keynote
{
    friend class emantFX::adsr;
    keynote() : note_number(0), velocity(0),
                current_level(0.0), status(IDLE){}

    short note_number;
    short velocity;
    double current_level;
    adsr_status_t status;
};


class adsr
{
    public:
        adsr(double attack_ms,double decay_ms,double sustain,double release_ms, double tick_ms,adsr_percussion_type_t perc);
        virtual ~adsr();
        //inline void hold_pressed(void) {hold_pedal = true;}
        //inline void hold_released(void) {hold_pedal = false;}
        void key_pressed(short note, short velocity = 127);
        void key_released(short note, short velocity = 0);
        void tick(void);
        inline double get_level(short key){return (keyArray[key].velocity*velocity_slope)*keyArray[key].current_level;};
        inline adsr_status_t get_status(short key){return keyArray[key].status;};
    private:
        bool hold_pedal;
        keynote *keyArray;
        double tick_size_ms;
        double attack_slope;
        double decay_slope;
        double sustain_level;
        double release_slope;
        double velocity_slope;
        adsr_percussion_type_t percussion;
};

} // emantFX

#endif // KEYBOARD_H
