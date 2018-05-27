

/************************************************************
*                                                           *
*       TONEWHEEL ORGAN CLONE TEST v0.1_alpha               *
*       Author: Emant    -    Date: July 2014               *
*                                                           *
************************************************************/


#include "RtAudio.h"
#include "RtMidi.h"
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <list>
#include <getopt.h>
#include <signal.h>


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

///SETTINGS

unsigned int sample_rate = 44100;

unsigned int buf_size = 128;

float transient_ms = 8;

float volume = 0.8;

float xtalk = 0.05;

float chorus_level = 0.4;

float tremulant_level = 0.3;

float tremulant_freq = 3.0;

//                  S    S3    F    2   3     4    5    6    8
float drawbars[] = {0.8, 0.8 , 0.6, 0.4, 0.3, 0.2, 0.3, 0.5, 0.6};

/// --------------

float sine[TABLE_SIZE+1];
float gear_ratios[]={0.817307692, //C
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

double wheel_phase[91];
float wheel_frequency[91];
float wheel_level[91];
float wheel_target_level[91];
short wheel_transient_counter[91];
double wheel_level_step[91];

float chorus_freq_low[36];
float chorus_freq_high[36];
double chorus_phase_low[36];
double chorus_phase_high[36];

float tremulant_wave[]=
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

float volume_coeff = 0.0;

// COMPRESSOR PARAMS
// Output level is estimated using the sum of all (target) tonewheels output multipliers

float comp_ratio = 2.5;
float comp_lim = -0.2 ;
float comp_thres = -10.0;
float comp_level = 1.0;
float comp_target = 1.0;
float comp_step = 0.0;
float comp_trans_ms = 60;
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
double incr = ((double)TABLE_SIZE)/(sample_rate);
double t_incr = ((double)TREM_SIZE)/(sample_rate);

std::list<short> keyboard;

// First Order RC LowPass Filter - Direct Form II

double 	filter_R = 1.0,		    // Variable R
		filter_C = .60e-5;		// Fixed C

double filter_coefs[] = {	0.0,	// delay
							0.0,	// a[1]
							0.0,	// b[0]
							0.0};	// b[1]



/// FUNCTIONS

int hammond_initialize()
{
    for( int i=0; i<TABLE_SIZE; i++)
        sine[i] = sin( 2*M_PI/TABLE_SIZE*i);
    sine[TABLE_SIZE] = 0.0f;

    for( int i=0; i<91; i++)
    {
        wheel_phase[i] = 0.0;
        wheel_level[i] = 0.0;
        wheel_target_level[i] = 0.0;
        wheel_transient_counter[i] = 0;
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


    // Tone LowPass Filter Init
    double tau = filter_R*filter_C;
    filter_coefs[1] = (1 - 2*tau*sample_rate)/(1 + 2*tau*sample_rate);
    filter_coefs[2] = 1.0 / (1 + 2*tau*sample_rate);
    filter_coefs[3] = filter_coefs[2];

    MUTEX_INIT(mutex);

    return 0;
}

inline float get_sine_value(double phase)
{
    int int_index = std::floor(phase);
    double residual = phase - int_index;
    float result = sine[int_index] + residual*(sine[int_index+1] - sine[int_index]);
    return result;
}

inline float get_trem_value(double phase)
{
    int int_index = std::floor(phase);
    double residual = phase - int_index;
    float result = tremulant_wave[int_index] + residual*(tremulant_wave[int_index+1] - tremulant_wave[int_index]);
    return result;
}

inline int set_compression(float x)
{
    float db = 20*log10(x+1e-100);
    float trgt_db;
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

inline int set_target_levels()
{
    for (int i = 0 ; i < 91 ; i++)
    wheel_target_level[i] = 0.0;

    for(auto it = keyboard.begin(); it != keyboard.end(); it++)
    {
        for(int bar = 0; bar <9; bar ++)
        {
            wheel_target_level[wirematrix[*it][bar] -1] += drawbars[bar];
            wheel_target_level[crosstalk(wirematrix[*it][bar] -1)] += xtalk*drawbars[bar];
        }

    }
    // Estimate RMS from Tonewheel Target Levels and set Target Gain Coefficient
    float estimated_rms = 0.0;
    for (int i = 0 ; i < 91 ; i++)
         estimated_rms+=wheel_target_level[i]*wheel_target_level[i];
    estimated_rms = sqrt(estimated_rms)*volume_coeff / 9.0;
    set_compression(estimated_rms);
    return 0;
}

inline int set_level_steps()
{
    MUTEX_LOCK(mutex);
    for (int i = 0 ; i < 91 ; i++)
    {
        if(wheel_transient_counter[i] == trans)
        {
            wheel_level_step[i] = (wheel_target_level[i] - wheel_level[i])/(buf_size*trans);
            wheel_transient_counter[i]-- ;
        }
        else if (wheel_transient_counter[i] > 0)
        {
            wheel_transient_counter[i]-- ;
        }
        else
            wheel_level_step[i] = 0.0;

    }
    if(comp_count == 0)
        comp_step = 0.0;
    else
        comp_count --;

    MUTEX_UNLOCK(mutex);
    return 0;

}

inline double get_sample()
{
    double sample = 0.0;
    float trem;
    for(int wheel = 0; wheel < 91; wheel++)
    {
        wheel_level[wheel] += wheel_level_step[wheel];
        sample += wheel_level[wheel]*(get_sine_value(wheel_phase[wheel]));
        wheel_phase[wheel] += wheel_frequency[wheel]*incr;
        if(wheel_phase[wheel] >= TABLE_SIZE) wheel_phase[wheel] -= TABLE_SIZE;

    }
    for(int i = 0; i < 36; i++)
    {
        sample += wheel_level[i+55]*chorus_level*(get_sine_value(chorus_phase_low[i]) + get_sine_value(chorus_phase_high[i]) );
        chorus_phase_low[i] += chorus_freq_low[i] * incr ;
        chorus_phase_high[i] += chorus_freq_high[i] * incr ;
        if(chorus_phase_low[i] >= TABLE_SIZE) chorus_phase_low[i] -= TABLE_SIZE;
        if(chorus_phase_high[i] >= TABLE_SIZE) chorus_phase_high[i] -= TABLE_SIZE;
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
    double *buffer = (double *) outputBuffer;
    if ( status )
    std::cout << "Stream underflow detected!" << std::endl;
    set_level_steps();
    for (unsigned int i=0; i<nBufferFrames; i++ )
    {
        double s = get_sample();
        double k1,out_s;
        k1 = s - filter_coefs[1]*filter_coefs[0];
        out_s = k1 * filter_coefs[2] + filter_coefs[0]*filter_coefs[3];
        filter_coefs[0] = k1;
        for ( int j=0; j<2; j++ )
            *buffer++ = out_s;
    }
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
            for(int bar = 0; bar <9; bar ++)
            {
                wheel_transient_counter[wirematrix[key][bar]-1] = trans;
                wheel_transient_counter[crosstalk(wirematrix[key][bar]-1)] = trans;
            }
            keyboard.push_back(key);
        }

        else if((message->at(0) == 0x90 && message->at(2) == 0) || message->at(0) == 0x80)
        {
            for(int bar = 0; bar <9; bar ++)
            {
                wheel_transient_counter[wirematrix[key][bar]-1] = trans;
                wheel_transient_counter[crosstalk(wirematrix[key][bar]-1)] = trans;
            }
            keyboard.remove(key);
        }
        MUTEX_UNLOCK(mutex);
        set_target_levels();
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
     {"tone",1,NULL,'T'},
     {"tremulant_level",1,NULL,'t'},
     {"tremulant_frequency",1,NULL,'f'},
     {"help",0,NULL,'h'},
     {"midi_device",1,NULL,'M'},
     {"audio_device",1,NULL,'A'},
     {0,0,0,0}};

    while((c = getopt_long(argc,argv,"s:b:d:x:c:V:T:t:f:hM:A:",opts,NULL)) != -1)
    {
        switch (c)
        {
            case 's' :
                sample_rate=atoi(optarg);
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
             case 'T' :
                filter_R= 1.0 + 499.0*(1.0 - atof(optarg));
            break;
            case 't' :
                tremulant_level=atoi(optarg)/100.0;
            break;
            case 'f' :
                tremulant_freq=atof(optarg);
            break;
            case 'M' :
                midiport =(unsigned int)atoi(optarg);
            break;
            case 'A' :
                audiodev =(unsigned int)atoi(optarg);
            break;
            case 'h' :
                std::cout<<"\nTONEWHEEL ORGAN CLONE TEST v0.1_alpha - Emant 2014.\nUsage: tonewheel_test [[Option][Value]]\n\nValid Options are:\n -s Sample Rate\n -b Buffer Size\n -M MIDI Device\n -A ASIO Device\n -V Volume - range = 0 : 10\n -T Tone - range 0.0 : 1.0\n -d 9 Digits Drawbars Settings - range = 0 : 8 (eg. 886660143)\n\
 -x Crosstalk Level - range = 0 : 100\n -c Chorus Level - range = 0 : 100\n -t Tremulant Level - range = 0 : 100\n -f Tremulant Frequency - range 0.0 = 10.0\n\n";
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
    try {
    dac.openStream( &parameters, NULL, RTAUDIO_FLOAT64,
    sample_rate, &buf_size, &wave);
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
