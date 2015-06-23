

/************************************************************
*                                                           *
*       TONEWHEEL ORGAN CLONE TEST v0.3_alpha               *
*       Author: Emant                                       *
*       Version : 0.3 alpha                                 *
*                                                           *
************************************************************/



#include <iostream>
#include <cstdlib>
#include <cmath>
#include <list>
#include <getopt.h>
#include <signal.h>
#include <cstring>
#include <stdint.h>

#include "adsr.h"
#include "RtAudio.h"
#include "RtMidi.h"
#include "sc_reverb.h"
#include "perc_envelope.h"


#if (defined (_WIN32) || defined (__WIN32__))

  #include <Windows.h>

  // Windows native mutexes
  #define MUTEX_T CRITICAL_SECTION
  #define MUTEX_INIT(x) InitializeCriticalSection(&x)
  #define MUTEX_DESTROY(x) DeleteCriticalSection(&x)
  #define MUTEX_LOCK(x) EnterCriticalSection(&x)
  #define MUTEX_UNLOCK(x) LeaveCriticalSection(&x)

#elif (defined (__linux)|| defined (__linux__))

	#include <pthread.h>

  #define MUTEX_T pthread_mutex_t
  #define MUTEX_INIT(x) pthread_mutex_init(&x, NULL)
  #define MUTEX_DESTROY(x) pthread_mutex_destroy(&x)
  #define MUTEX_LOCK(x) pthread_mutex_lock(&x)
  #define MUTEX_UNLOCK(x) pthread_mutex_unlock(&x)
  #ifndef _REENTRANT
  	#define _REENTRANT
  #endif
#endif

///GLOBALS

#define TABLE_SIZE (1024)
#define TREM_SIZE (48)

#ifndef M_PI
    #define M_PI (3.14159265)
#endif // M_PI

#define MAXUINT32 4294967296.0

using namespace emantFX;

///SETTINGS

unsigned int sample_rate = 44100;

unsigned int buf_size = 128;

double frq_const = MAXUINT32 / sample_rate;

double transient_ms = 8;

double volume = 0.8;

double xtalk = 0.05;

double chorus_level = 0.4;

double tremulant_level = 0.3;

double tremulant_freq = 3.0;

int perc_draw = 0;

int fast = 0;

double perc_lev = 1.5;

//                  S    S3    F    2   3     4    5    6    8
double drawbars[] = {0.8, 0.8 , 0.6, 0.4, 0.3, 0.2, 0.3, 0.5, 0.6};

/// --------------

const double INTERP_FACT = 1.0/0x400000;

double sine[TABLE_SIZE+1];
double gear_ratios[]={0.817307692, //C
                     0.865853659, //C#
                     0.917808219, //D
                     0.972222222, //D#
                     1.030000000, //E
                     1.090909091, //F
                     1.156250000, //F#
                     1.225000000, //G
                     1.297297297, //G#
                     1.375000000, //A
                     1.456521739, //A#
                     1.542857143}; //B

uint32_t wheel_phase[91];
double wheel_frequency[91];
double wheel_level[91];
double wheel_target[91];
double wheel_level_step[91];

double chorus_freq_low[36];
double chorus_freq_high[36];
uint32_t chorus_phase_low[36];
uint32_t chorus_phase_high[36];

double tremulant_wave[]=
{
    0.0,            0.0,            0.0,            0,0,
    0.0022222222,   0.0022222222,   0.0022222222,   0.0022222222,
    0.0088888889,   0.0088888889,   0.0088888889,   0.0088888889,
    0.0222222222,   0.0222222222,   0.0222222222,   0.0222222222,
    0.0472222222,   0.0472222222,   0.0472222222,   0.0472222222,
    0.1138888889,   0.1138888889,   0.1138888889,   0.1138888889,
    1.0,            1.0,            1.0,            1.0,
    0.1138888889,   0.1138888889,   0.1138888889,   0.1138888889,
    0.0472222222,   0.0472222222,   0.0472222222,   0.0472222222,
    0.0222222222,   0.0222222222,   0.0222222222,   0.0222222222,
    0.0088888889,   0.0088888889,   0.0088888889,   0.0088888889,
    0.0022222222,   0.0022222222,   0.0022222222,   0.0022222222,
    0.0
};



double tremulant_phase = 0.0;

double volume_coeff = 0.0;

// COMPRESSOR PARAMS
// Output level is estimated using the sum of all (target) tonewheels output multipliers

double comp_ratio = 2.5;
double comp_lim = -0.2 ;
double comp_thres = -10.0;
double comp_level = 1.0;
double comp_target = 1.0;
double comp_step = 0.0;
double comp_trans_ms = 60;
int comp_trans = 1;
int comp_count  = 0;


//                DRAWBARS  S   S3  F   2   3   4   5   6   8
short wirematrix[61][9]=   {1,  20, 13, 25, 32, 37, 41, 44, 49,     // C2
                            2,  21, 14, 26, 33, 38, 42, 45, 50,     // C#2
                            3,  22, 15, 27, 34, 39, 43, 46, 51,     // D2
                            4,  23, 16, 28, 35, 40, 44, 47, 52,     // D#2
                            5,  24, 17, 29, 36, 41, 45, 48, 53,     // E2
                            6,  25, 18, 30, 37, 42, 46, 49, 54,     // F2
                            7,  26,	19,	31,	38,	43,	47,	50,	55,     // F#2
                            8,  27,	20,	32,	39,	44,	48,	51,	56,     // G2
                            9,  28,	21,	33,	40,	45,	49,	52,	57,     // G#2
                            10, 29,	22,	34,	41,	46,	50,	53,	58,     // A2
                            11,	30,	23,	35,	42,	47,	51,	54,	59,     // A#2
                            12,	31,	24,	36,	43,	48,	52,	55,	60,     // B2
                            13,	32,	25,	37,	44,	49,	53,	56,	61,     // C3
                            14,	33,	26,	38,	45,	50,	54,	57,	62,
                            15,	34,	27,	39,	46,	51,	55,	58,	63,
                            16,	35,	28,	40,	47,	52,	56,	59,	64,
                            17,	36,	29,	41,	48,	53,	57,	60,	65,
                            18,	37,	30,	42,	49,	54,	58,	61,	66,
                            19,	38,	31,	43,	50,	55,	59,	62,	67,
                            20,	39,	32,	44,	51,	56,	60,	63,	68,
                            21,	40,	33,	45,	52,	57,	61,	64,	69,
                            22,	41,	34,	46,	53,	58,	62,	65,	70,
                            23,	42,	35,	47,	54,	59,	63,	66,	71,
                            24,	43,	36,	48,	55,	60,	64,	67,	72,
                            25,	44,	37,	49,	56,	61,	65,	68,	73,     //C4
                            26,	45,	38,	50,	57,	62,	66,	69,	74,
                            27,	46,	39,	51,	58,	63,	67,	70,	75,
                            28,	47,	40,	52,	59,	64,	68,	71,	76,
                            29,	48,	41,	53,	60,	65,	69,	72,	77,
                            30,	49,	42,	54,	61,	66,	70,	73,	78,
                            31,	50,	43,	55,	62,	67,	71,	74,	79,
                            32,	51,	44,	56,	63,	68,	72,	75,	80,
                            33,	52,	45,	57,	64,	69,	73,	76,	81,
                            34,	53,	46,	58,	65,	70,	74,	77,	82,
                            35,	54,	47,	59,	66,	71,	75,	78,	83,
                            36,	55,	48,	60,	67,	72,	76,	79,	84,
                            37,	56,	49,	61,	68,	73,	77,	80,	85,     // C5
                            38,	57,	50,	62,	69,	74,	78,	81,	86,
                            39,	58,	51,	63,	70,	75,	79,	82,	87,
                            40,	59,	52,	64,	71,	76,	80,	83,	88,
                            41,	60,	53,	65,	72,	77,	81,	84,	89,
                            42,	61,	54,	66,	73,	78,	82,	85,	90,
                            43,	62,	55,	67,	74,	79,	83,	86,	91,
                            44,	63,	56,	68,	75,	80,	84,	87,	68,
                            45,	64,	57,	69,	76,	81,	85,	88,	69,
                            46,	65,	58,	70,	77,	82,	86,	89,	70,
                            47,	66,	59,	71,	78,	83,	87,	90,	71,
                            48,	67,	60,	72,	79,	84,	88,	91,	72,
                            49,	68,	61,	73,	80,	85,	89,	80,	73,     // C6
                            50,	69,	62,	74,	81,	86,	90,	81,	74,
                            51,	70,	63,	75,	82,	87,	91,	82,	75,
                            52,	71,	64,	76,	83,	88,	80,	83,	76,
                            53,	72,	65,	77,	84,	89,	81,	84,	77,
                            54,	73,	66,	78,	85,	90,	82,	85,	78,
                            55,	74,	67,	79,	86,	91,	83,	86,	79,
                            56,	75,	68,	80,	87,	80,	84,	87,	80,
                            57,	76,	69,	81,	88,	81,	85,	88,	81,
                            58,	77,	70,	82,	89,	82,	86,	89,	82,
                            59,	78,	71,	83,	90,	83,	87,	90,	83,
                            60,	79,	72,	84,	91,	84,	88,	91,	84,
                            61,	80,	73,	85,	80,	85,	89,	80,	85};    // C7

MUTEX_T mutex;

RtAudio dac;

short trans = 0;
double incr = MAXUINT32/sample_rate;
double t_incr = ((double)TREM_SIZE)/sample_rate;

adsr *normal_envelope;
perc_envelope *perc;

/// Reverb
double wet_rev = 0.8, dry_rev = 0.1,rev_time = 1,rev_damping = 0.2;
unsigned int loopSpls[] = {1307,1637,1811,1931,223,199,73};
double gains[] = {1,1,1,1,0.09683,0.03292};
double dampings[] = {0.2,0.2,0.2,0.2};
filtersBank bank;


/// FUNCTIONS

int hammond_initialize()
{
    for( int i=0; i<TABLE_SIZE; i++)
    {
        sine[i] = sin( 2*M_PI/TABLE_SIZE*i);
        sine[i] += 0.048*sin( 4*M_PI/TABLE_SIZE*i);
    }
    sine[TABLE_SIZE] = sine [0];

    for( int i=0; i<91; i++)
    {
        wheel_phase[i] = 0.0;
        wheel_level[i] = 0.0;
        wheel_target[i] = 0.0;
        wheel_level_step[i] = 0.0;
    }

    int k = 0;
    int octave = 1;
    for (int i = 0 ; i < 7 ; i++)
    {
        octave = 2*octave;
        for(int j = 0; j < 12; j++)
        {
            wheel_frequency[k++] = 20*octave*gear_ratios[j];
        }

    }
    octave = 192;
    for (int i = 0 ; i < 7 ; i++)
	{
        wheel_frequency[k++] = 20*octave*gear_ratios[5+i];
	}
    trans = std::ceil(transient_ms*sample_rate/(buf_size*1000));

    double delta_freq = 0.8/100.0;
    for( int i=0; i<12; i++)
    {
        chorus_freq_low[i]  = wheel_frequency[55 +i]* ( 1 - delta_freq );
        chorus_freq_high[i] = wheel_frequency[55 +i]* ( 1 + delta_freq );
        chorus_phase_low[i] = 0.0;
        chorus_phase_high[i] = 0.0;
    }
    delta_freq = 0.4/100.0;
    for( int i=12; i<36; i++)
    {
        chorus_freq_low[i]  = wheel_frequency[55 +i]* ( 1 - delta_freq );
        chorus_freq_high[i] = wheel_frequency[55 +i]* ( 1 + delta_freq );
        chorus_phase_low[i] = 0.0;
        chorus_phase_high[i] = 0.0;
    }

    volume_coeff = (1 + chorus_level + tremulant_level+xtalk);
    volume = volume*1.0/(9.0*volume_coeff);
    comp_trans = std::ceil((comp_trans_ms*sample_rate)/(buf_size*1000.0));

	for(unsigned int i = 0; i < 4; i++ ){
		gains[i] = rev_time;
		dampings[i] = rev_damping;
	}

	for(unsigned int i = 0; i < 6; i++ )
        gains[i] = pow(0.001,loopSpls[i]/(sample_rate*gains[i]));
    if(initFilters(&bank,loopSpls,gains,dampings,4,2)){
    	std::cout << "\nError Allocating Memory !\n\n";
    	exit( 0 );
    }

    normal_envelope = new adsr(8.0,1.0,1.0,8.0,buf_size*1000.0/sample_rate,NO_PERCUSSION);
    if(fast == 1)
    	perc = new perc_envelope(8,120,0.26,120,8,buf_size*1000.0/sample_rate);
    else
    	perc = new perc_envelope(8,500,0.26,600,8,buf_size*1000.0/sample_rate);
    if(perc_draw > 0)
        drawbars[8] = 0;
    MUTEX_INIT(mutex);

    return 0;
}

inline double get_sine_value(uint32_t phase)
{
    uint32_t ind = phase >> 22;
    double res = (phase & 0x3FFFFF)*INTERP_FACT;
    return  sine[ind]*(1 - res) + sine[ind+1]*res;
}

inline float get_trem_value(double phase)
{
    int int_index = std::floor(phase);
    double residual = phase - int_index;
    float result = tremulant_wave[int_index] + residual*(tremulant_wave[int_index+1] - tremulant_wave[int_index]);
    return result;
}
inline int set_compression(double x)
{
    double db = 20*log10(x+1e-100);
    double trgt_db;
    if(db < comp_thres)
    {
        comp_step = (1.0-comp_level)/(comp_trans*buf_size);
        comp_count = comp_trans;
        return 0;
    }
    trgt_db = comp_thres + (db-comp_thres)/comp_ratio;

    if(trgt_db > comp_lim)
        trgt_db = comp_lim;
    comp_target = pow(10,trgt_db/20)/x;
    comp_step = (comp_target-comp_level)/(comp_trans*buf_size);
    comp_count = comp_trans;
    return 0;
}

int crosstalk(int wheel)
{
    if(wheel < 36)
        return wheel + 48;
    if(wheel < 41)
        return wheel;
    if (wheel < 48)
        return wheel +43;
    if (wheel < 84)
        return wheel -48;
    return wheel - 43;
}

inline void reset_targets()
{
	memset(wheel_target,0,91*sizeof(double));
}

inline int set_level_steps()
{

    reset_targets();
    MUTEX_LOCK(mutex);
    double prc = perc->get_level();
    for(int j = 0; j < 61; j++)
    {
        if(normal_envelope->get_status(j)!= IDLE){
            double lev = normal_envelope->get_level(j);


            for(int i = 0; i < 9 ; i++)
            {
                wheel_target[wirematrix[j][i]-1] +=  lev*drawbars[i];
            }
            if(perc_draw > 0)
                wheel_target[wirematrix[j][perc_draw]-1] += perc_lev*prc;
        }
    }
    normal_envelope->tick();
    perc->tick();
    MUTEX_UNLOCK(mutex);
    double estimated_rms = 0.0;
    for(short i = 0; i<91; i++)
    {
        estimated_rms+=wheel_target[i]*wheel_target[i];
        wheel_level_step[i] = (wheel_target[i]-wheel_level[i])/buf_size;
    }
    estimated_rms = sqrt(estimated_rms)*volume_coeff / 9.0;
    set_compression(estimated_rms);
    return 0;

}

inline double get_sample()
{
    double sample = 0.0;
    double trem;
    for(int wheel = 0; wheel < 91; wheel++)
    {
        wheel_level[wheel] += wheel_level_step[wheel];
        sample += wheel_level[wheel]*(get_sine_value(wheel_phase[wheel]));
        wheel_phase[wheel] += (uint32_t)(wheel_frequency[wheel]*incr);

    }
    for(int i = 0; i < 36; i++)
    {
        sample += wheel_level[i+55]*chorus_level*(get_sine_value(chorus_phase_low[i]) + get_sine_value(chorus_phase_high[i]) );
        chorus_phase_low[i] += (uint32_t)(chorus_freq_low[i] * incr) ;
        chorus_phase_high[i] += (uint32_t)(chorus_freq_high[i] * incr) ;
    }
    trem = tremulant_level*get_trem_value(tremulant_phase);
    tremulant_phase += tremulant_freq * t_incr;
    if(tremulant_phase >= TREM_SIZE) tremulant_phase -= TREM_SIZE;
    comp_level += comp_step;
    sample *= volume * comp_level *(1+trem);
    return sample;
}


// Two-channel wavetable signal generator.
inline int wave( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
double streamTime, RtAudioStreamStatus status, void *userData )
{
    std::vector<unsigned char> message;
    double *bufferL = (double *) outputBuffer;
    double *bufferR = bufferL + nBufferFrames;
    filtersBank *bnk = (filtersBank*) userData;
    if ( status )
    std::cout << "Stream underflow detected!" << std::endl;
    set_level_steps();
    for (unsigned int i=0; i<nBufferFrames; i++ )
        bufferL[i] = get_sample();
    // Reverb
    processParallelCombs(bnk,bufferL,bufferR,0,nBufferFrames);
    processSerieAllpass(bnk,bufferR,bufferR,bnk->nCombs,nBufferFrames);
    mixer(bufferL,bufferR,bufferL,bufferR,wet_rev,dry_rev,nBufferFrames);
    return 0;
}

inline void midicbk( double deltatime, std::vector< unsigned char > *message, void *userData )
{
    short key = message->at(1) - 36;
    if(key >=0 && key < 61)
    {
        MUTEX_LOCK(mutex);
        if (message->at(0) == 0x90 && message->at(2) > 0)
        {
            normal_envelope->key_pressed(key);
            perc->key_pressed(key);
        }

        else if((message->at(0) == 0x90 && message->at(2) == 0) || message->at(0) == 0x80)
        {
            normal_envelope->key_released(key);
            perc->key_released(key);
        }
        MUTEX_UNLOCK(mutex);
    }
}

static void kill_fn (int signo)
{
	std::cout<<"\nExit Requested...\nStopping Audio Stream...\n";
	try {
    // Stop the stream
    dac.stopStream();
    }
    catch (RtAudioError& e) {
    e.printMessage();
    }
    std::cout<<"Closing Audio Stream ...\n";
    if ( dac.isStreamOpen() ) dac.closeStream();
    std::cout<<"Cleaning MUTEX.\n\n";
    MUTEX_DESTROY(mutex);
    std::cout<<"Bye !!\n\n";
    exit(0);
}

int main(int argc, char **argv)
{
    int c;
    unsigned int midiport = 0,audiodev = 0;
    struct option opts[] =
    {{"sample_rate",1,NULL,'s'},
     {"buffer_size",1,NULL,'b'},
     {"drawbars",1,NULL,'d'},
     {"xtalk",1,NULL,'x'},
     {"chorus",1,NULL,'c'},
     {"volume",1,NULL,'V'},
     {"tremulant_level",1,NULL,'t'},
     {"tremulant_fre double dres = ((double)phase & 0x3FFFFF)/0x400000;quency",1,NULL,'f'},
     {"reverb_time",1,NULL,'T'},
     {"reverb_damping",1,NULL,'r'},
     {"reverb_dry",1,NULL,'D'},
     {"reverb_wet",1,NULL,'W'},
     {"help",0,NULL,'h'},
     {"midi_device",1,NULL,'M'},
     {"audio_device",1,NULL,'A'},
     {"percussion",1,NULL,'P'},
     {"perc_fast",0,NULL,'F'},
     {"perc_soft",0,NULL,'S'},
     {0,0,0,0}};

    while((c = getopt_long(argc,argv,"s:b:d:x:c:V:t:f:T:r:D:W:hM:A:P:FS",opts,NULL)) != -1)
    {
        switch (c)
        {
            case 's' :
                sample_rate=atoi(optarg);
                incr = MAXUINT32/sample_rate;
                t_incr = ((double)TREM_SIZE)/sample_rate;
            break;
            case 'b' :
                buf_size = atoi(optarg);
            break;
            case 'd' :
                for(int i = 0; i< 9; i++)
                {
                    short val = optarg[i] - 48;
                    if(val > 0)
                    {
                        if(val < 9)
                            drawbars[i] = val/10.0;
                        else
                            drawbars[i] = 0.8;
                    }
                    else
                        drawbars[i] = val/10.0;
                }

            break;
            case 'x' :
                xtalk = atoi(optarg)/100.0;
            break;
            case 'c' :
                chorus_level = atoi(optarg)/100.0;
            break;
            case 'V' :
                volume = atoi(optarg)/10.0;
            break;
            case 't' :
                tremulant_level=atoi(optarg)/100.0;
            break;
            case 'f' :
                tremulant_freq=atof(optarg);
            break;
            case 'T' :
            	rev_time = atof(optarg);
            break;
            case 'r' :
            	rev_damping = atof(optarg);
            break;
            case 'D' :
            	dry_rev = atof(optarg);
            break;
            case 'W' :
            	wet_rev = atof(optarg);
            break;
            case 'M' :
                midiport =(unsigned int)atoi(optarg);
            break;
            case 'A' :
                audiodev =(unsigned int)atoi(optarg);
            break;
            case 'P' :
                perc_draw = (int)atoi(optarg);
                perc_draw = (perc_draw == 2 || perc_draw == 3)? perc_draw+1 : 0;
            break;
            case 'F' :
            	fast = 1;
            break;
            case 'S' :
            	perc_lev = 1.0;
            break;
            case 'h' :
                std::cout<<"\nTONEWHEEL ORGAN CLONE TEST v0.1_alpha - Emant 2014.\nUsage: tonewheel [[Option][Value]]\n\nValid Options are:\n\
 -s Sample Rate\n -b Buffer Size\n -M MIDI Device\n -A AUDIO Device\n -V Volume - range = 0 : 10\n -d 9 Digits Drawbars Settings - range = 0 : 8 (eg. 886660143)\n\
 -x Crosstalk Level - range = 0 : 100\n -c Chorus Level - range = 0 : 100\n -t Tremulant Level - range = 0 : 100\n -f Tremulant Frequency - range 0.0 = 10.0\n\
 -T Reverb Time [s]\n -r Reverb Damping - range 0.0 : 1.0\n -D Dry Signal - range 0.0 : 1.0\n -W Reverb Wet Signal - range 0.0 : 1.0\n -P Percussion - 2 = 2nd Harm, 3 = 3rd Harm (Default 'OFF')\n\
 -F Percussion 'Fast' Decay (Default 'Slow')\n -S Percussion Volume 'Soft' (Default 'Normal')\n\n";
            return 0;
            break;
        }
    }
    signal(SIGINT, kill_fn);
    signal(SIGTERM, kill_fn);
    hammond_initialize();
    std::vector<unsigned char> message;
    RtMidiIn *midiin = 0;
    // RtMidiIn constructor
    try {
    midiin = new RtMidiIn();
    }
    catch ( RtMidiError &error ) {
    error.printMessage();
    exit( EXIT_FAILURE );
    }

    unsigned int nPorts = midiin->getPortCount();
    std::cout << "\nEmant 2014 - TONEWHEEL ORGAN CLONE TEST v0.1_alpha\nPowered by RtAudio - RtMidi.\n\n"<< nPorts <<" MIDI INPUT Device(s) Available. \n";
    if(nPorts == 0)
        {
            std::cout<<" No Midi IN! Quit ...\n";
            delete midiin;
            return 1;
        }
    std::string portName;
    for ( unsigned int i=0; i<nPorts; i++ ) {
    try {
    portName = midiin->getPortName(i);
    }
    catch ( RtMidiError &error ) {
    error.printMessage();
    }
    std::cout << " Input #" << i+1 << ": " << portName << '\n';
    }
    if((nPorts == 1 || midiport < 1) || midiport > nPorts)
    {
        std::cout<<"\nOpening Default MIDI Device ... \n\n";
        midiin->openPort(0);
    }
    else
    {
        std::cout<<"\nOpening MIDI Device #"<<midiport<<"... \n\n";
        midiin->openPort(midiport-1);
    }
    midiin->setCallback(&midicbk);
    unsigned int audio_count = dac.getDeviceCount();
    std::cout<< audio_count <<" AUDIO OUTPUT Device(s) Available. \n";
    if ( audio_count < 1 ) {
    std::cout << "\n No AUDIO Out! Quit ...\n";
    exit( 0 );
    }
    for(unsigned int i = 0; i< audio_count; i++)
    {
        RtAudio::DeviceInfo info = dac.getDeviceInfo(i);
        std::cout << " AUDIO Output #" << i+1 << ": " << info.name << '\n';
    }
    RtAudio::StreamParameters parameters;
    if((audio_count == 1 || audiodev < 1 ) || audiodev > audio_count)
    {
        parameters.deviceId = dac.getDefaultOutputDevice();
        std::cout<<"\nOpening Default AUDIO Device ... \n\n";
    }
    else
    {
        parameters.deviceId = audiodev-1;
        std::cout<<"\nOpening AUDIO Device #"<<audiodev<<" ... \n\n";
    }
    parameters.nChannels = 2;
    parameters.firstChannel = 0;

    RtAudio::StreamOptions Opt;
    Opt.flags = (RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_HOG_DEVICE | RTAUDIO_SCHEDULE_REALTIME | RTAUDIO_NONINTERLEAVED);

    try {
    dac.openStream( &parameters, NULL, RTAUDIO_FLOAT64,
    sample_rate, &buf_size, &wave, &bank, &Opt);
    dac.startStream();
    }
    catch ( RtAudioError& e ) {
    e.printMessage();
    exit( 0 );
    }

    char input;
    std::cout << "\nOK! You Can Play Now ...\nPress <Enter> to Quit ...\n";
    std::cin.get( input );
    try {
    // Stop the stream
    dac.stopStream();
    }
    catch (RtAudioError& e) {
    e.printMessage();
    }
    if ( dac.isStreamOpen() ) dac.closeStream();
    MUTEX_DESTROY(mutex);
    return 0;
}
