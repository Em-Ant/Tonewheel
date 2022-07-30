
/*****************************************************
        Mono In - Stereo Out Schroeder Reverb
------------------------------------------------------
        Author:  Emant
        Version: 0.1alpha
        Date:    2014
------------------------------------------------------
 Artificial Algorithmic (i.e. No Convolution) Reverb
 using Filtered Feedback Combs and All Pass Filters.

   H_filtered_comb(z) = z^-N/(1-g*[H_lp(z)]*z^-N),
   H_lp(z) = (1-d)/(1-d*z^-1) <- First Order LowPass

   H_AP(z)   = (-g + z^-N)/(1-g*z^-N)

------------------------------------------------------
 For An Introductive Reading see :
    https://ccrma.stanford.edu/~jos/pasp/
*****************************************************/

#include "sc_reverb.h"
#include <stdlib.h>

/* Allocating Memory & Setting Values */
int initFilters  (filtersBank* bank,
                  unsigned int* loopSamples,
                  double *gains,
                  double *combDampings,
                  unsigned int nCombsParallel,
                  unsigned int nAllpassSerie)
{
    unsigned int i,n,totalBufferLength = 0;
    bank->nCombs = nCombsParallel;
    bank->nAllPass = nAllpassSerie;
    n = nCombsParallel + nAllpassSerie ;
    bank->maxIndexes = (unsigned int*)calloc(2*n,sizeof(unsigned int));
	if(bank->maxIndexes == 0)
		return 1;
    bank->buffers = (double**)calloc(n,sizeof(double*));
    bank->indexes = bank->maxIndexes + n;
    for(i=0; i < n; i++){
        bank->maxIndexes[i] = loopSamples[i];
        totalBufferLength += loopSamples[i];
    }
    bank->buffers[0] = (double*)calloc(totalBufferLength+2*(n+nCombsParallel),sizeof(double));
    if(bank->buffers == 0)
		return 1;
    for(i=1; i < n; i++)
        bank->buffers[i] = bank->buffers[i-1] + bank->maxIndexes[i-1];
    bank->gains = bank->buffers[n-1] + bank->maxIndexes[n-1];
    bank->combDampings = bank->gains + n;
    bank->combDly = bank->combDampings + nCombsParallel;
    for(i=0; i < n; i++)
        bank->gains[i] = gains[i];
    for(i=0; i < nCombsParallel; i++)
        bank->combDampings[i] = combDampings[i];
    return 0;
}

int processParallelCombs(filtersBank* bank,
                         double* inbuf,
                         double* outbuf,
                         unsigned int startPoint,
                         unsigned int nSamples)
{
    unsigned int i,j;
    double s, out;
    for(i = 0; i < nSamples; i++ ){
        out = 0;
        for(j = startPoint; j < startPoint + bank->nCombs; j++ ){						/* 'startPoint' is used to select processing order 	*/					
            s = bank->buffers[j][bank->indexes[j]];
            s = s*(1-bank->combDampings[j]) + bank->combDampings[j]*bank->combDly[j];	/* Inner First Order Lowpass Filter					*/
            bank->combDly[j]=s;
            bank->buffers[j][bank->indexes[j]++] = s*bank->gains[j] + inbuf[i];
            out += s;
            if(bank->indexes[j] >= bank->maxIndexes[j])
                bank->indexes[j] = 0;
        }
        outbuf[i] = out;
    }
    return 0;
}

int processSerieAllpass (filtersBank* bank,
                         double* inbuf,
                         double* outbuf,
                         unsigned int startPoint,
                         unsigned int nSamples)
{
    unsigned int i,j;
    double s, in;
    for(i = 0; i < nSamples; i++ ){
        in = inbuf[i];
        for(j = startPoint; j < startPoint + bank->nAllPass; j++ ){
            s = bank->buffers[j][bank->indexes[j]];
            bank->buffers[j][bank->indexes[j]++] = s*bank->gains[j] + in;
            in = (-bank->gains[j])*in + s;
            if(bank->indexes[j] >= bank->maxIndexes[j])
                bank->indexes[j] = 0;
        }
        outbuf[i] = in;
    }
    return 0;
}


int mixer (double *sigbuf,
           double *revbuf,
           double *outL,
           double *outR,
           double dry,
           double wet,
           unsigned int nSamples)
{
    unsigned int i;
    double L,R;
    for(i = 0; i < nSamples; i++){
        L = dry*sigbuf[i] + wet*revbuf[i];
        R = dry*sigbuf[i] + wet*-revbuf[i];
        outL[i] = L;
        outR[i] = R;
    }
    return 0;
}
