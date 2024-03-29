# Tonewheel

### A Tonewheel Organ Emulation

It's an almost accurate emulation of a Hammond Tonewheel organ, featuring 96+36 free-running wave-table sine oscillator, with original non equal-tempered frequencies. 
It'also has a tremolo unit, a tonewheel chorus, the classic percussion envelope, and an algorithmic [Schroeder Reverb](https://ccrma.stanford.edu/~jos/pasp/). I plan to add a saturation/distortion, and maybe a Leslie emulator. 
For now it's a standalone console project, without realtime parameters change during play.  
I've tested it under Win7 and Ubuntu 14.04/16.04. It's a little CPU hungry, I'plan to substitute the wheel chorus (emulating the Hammond BC or D Models) with a variable delay Tremolo, like the B3.

It's based on [RtAudio](http://www.music.mcgill.ca/~gary/rtaudio/) v4.1.1 & [RtMidi](http://music.mcgill.ca/~gary/rtmidi/) v2.1.0. For the CMake file to work as is You need to download them, compile these packages as static libraries for your platform, and place them in the /lib folder together with the headers which are included in this project. 

You'll need an external MIDI keyboard or some controller emulator to run it. Under Windows I strongly recommend to compile RtAudio with **ASIO** support ( you will need the ASIO SDK from Steinberg, it's free but cannot be redistributed. To obtain it you need to [register as a developer](http://www.steinberg.net/en/company/developers.html) at Steinberg) . Then you will need an *ASIO Driver* : [ASIO4ALL](http://www.asio4all.com/) is the standard for non-pro audio cards. 


