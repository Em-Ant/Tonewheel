
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

/* Filter Bank = nCombs in Parallel + nAllPass in Serie*/
/* Order of Processing can be selected */
typedef struct _fltrs{
    unsigned int nCombs;
    unsigned int nAllPass;
    double **buffers;
    double *gains;
    double *combDampings;
    double *combDly;
    unsigned int *indexes;
    unsigned int *maxIndexes;
}filtersBank;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* Initialize filters bank - Allocate memory fo buffers. MUST be called*/
int initFilters(	filtersBank* bank,
					unsigned int* loopSamples,          /* N of delay samples for each filter   */
					double *gains,                      /* Gain coefficients g for each filter  */
					double *combDampings,               /* Combs inner LowPass filter coefficients      */
					unsigned int nCombsParallel,        /* Number of Comb Filters in Parallel   */
					unsigned int nAllpassSerie);        /* Number of AllPass Filters in Serie   */

/* Procesc Functions. Should be called in Audio Callback Routine */
int processParallelCombs(	filtersBank* bank,
							double* inbuf,              /* Input Buffer     */
							double* outbuf,             /* Output Buffer    */
							unsigned int startPoint,    /* Index of the first Comb Filter in 'bank'.
                                                           Used to select order of processing   */
							unsigned int nSamples);     /* N samples in Audio Callback Buffer   */

int processSerieAllpass(filtersBank* bank,
                        double* inbuf,                  /* Exactly as before ... */
                        double* outbuf,
                        unsigned int startPoint,
                        unsigned int nSamples);


/* Warning: This function needs NON INTERLEAVED L-R Buffers*/
int mixer (	double *sigbuf,             /* Input Signal Buffer          */
			double *revbuf,             /* Mono Reverb Buffer coming from process functions */
			double *outL,               /* Left Channel Out Buffer      */
			double *outR,               /* Rigth Channel Out Buffer     */
			double dry,                 /* Dry signal in [0.0,1.0]      */
			double wet,                 /* Wet Signal in [0.0,1.0]      */
			unsigned int nSamples);     /* N samples in Audio Callback Buffer   */

#ifdef __cplusplus
}
#endif // __cplusplus
