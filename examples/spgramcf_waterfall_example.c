// waterfall example

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "liquid.h"

#define OUTPUT_FILENAME "spgramcf_waterfall_example.gnu"

int main()
{
    // msource parameters
    int          ms          = LIQUID_MODEM_QPSK;
    unsigned int k           =     4;
    unsigned int m           =    12;
    float        beta        = 0.30f;

    // spectral periodogram options
    unsigned int nfft        =   2400;  // spectral periodogram FFT size
    unsigned int num_samples = 250000;  // number of samples

    unsigned int i;

    // create time-varying multi-path channel object
    unsigned int c_len = 17;
    float        std   = 0.1f;
    float        tau   = 1e-4f;
    tvmpch_cccf channel = tvmpch_cccf_create(c_len, std, tau);
    tvmpch_cccf_print(channel);

    unsigned int buf_len = 64;
    float complex buf[buf_len];

    // create spectral periodogram
    spgramcf periodogram = spgramcf_create(nfft, LIQUID_WINDOW_KAISER, nfft/2, buf_len);
    spgramcf_set_alpha(periodogram, 0.5f);

    // create stream generator
    msourcecf gen = msourcecf_create();
    
    // add noise source (narrow-band)
    int id_noise = msourcecf_add_noise(gen, 0.10f);
    msourcecf_set_frequency(gen, id_noise, 0.4*2*M_PI);
    msourcecf_set_gain     (gen, id_noise, -20.0f);

    // add tone
    int id_tone = msourcecf_add_tone(gen);
    msourcecf_set_frequency(gen, id_tone, -0.4*2*M_PI);
    msourcecf_set_gain     (gen, id_tone, -10.0f);

    // add modulated data
    int id_modem = msourcecf_add_modem(gen,ms,k,m,beta);
    msourcecf_set_frequency(gen, id_modem, -0.1*2*M_PI);
    msourcecf_set_gain     (gen, id_modem, 0.0f);

    // print source generator object
    msourcecf_print(gen);

    unsigned int num_transforms = 0;

    FILE * fid = fopen("waterfall.bin","wb");
    // write header
    float nfftf = (float)nfft;
    fwrite(&nfftf, sizeof(float), 1, fid);
    for (i=0; i<nfft; i++) {
        float f = (float)i/nfftf - 0.5f;
        fwrite(&f, sizeof(float), 1, fid);
    }

    float psd[nfft];
    unsigned int total_samples = 0;
    while (total_samples < num_samples) {
        // write samples to buffer
        msourcecf_write_samples(gen, buf, buf_len);

        // run through channel
        tvmpch_cccf_execute_block(channel, buf, buf_len, buf);

        // push resulting sample through periodogram
        spgramcf_write(periodogram, buf, buf_len);

        // compute power spectral density output
        spgramcf_get_psd(periodogram, psd);

        // write output
        float n = (float)total_samples;
        fwrite(&n, sizeof(float), 1, fid);
        fwrite(psd, sizeof(float), nfft, fid);
        num_transforms++;

        // accumulated samples
        total_samples += buf_len;

    }
    tvmpch_cccf_print(channel);
    printf("total samples:    %u\n", total_samples);
    printf("total transforms: %u\n", num_transforms);
    fclose(fid);

    // destroy objects
    msourcecf_destroy(gen);
    spgramcf_destroy(periodogram);
    tvmpch_cccf_destroy(channel);

    // export output file
    fid = fopen(OUTPUT_FILENAME,"w");
    fprintf(fid,"#!/usr/bin/gnuplot\n");
    fprintf(fid,"reset\n");
    fprintf(fid,"set terminal png size 800,800 enhanced font 'Verdana,10'\n");
    fprintf(fid,"set output '%s.png'\n", OUTPUT_FILENAME);
    fprintf(fid,"unset key\n");
    fprintf(fid,"set style line 11 lc rgb '#808080' lt 1\n");
    fprintf(fid,"set border 3 front ls 11\n");
    fprintf(fid,"set style line 12 lc rgb '#888888' lt 0 lw 1\n");
    fprintf(fid,"set grid front ls 12\n");
    fprintf(fid,"set tics nomirror out scale 0.75\n");
    fprintf(fid,"set xrange [-0.5:0.5]\n");
    fprintf(fid,"set yrange [0:%u]\n", num_samples-1);
    fprintf(fid,"set xlabel 'Normalized Frequency [f/F_s]'\n");
    fprintf(fid,"set ylabel 'Sample Index'\n");
    fprintf(fid,"# disable colorbar tics\n");
    fprintf(fid,"set cbtics scale 0\n");
    fprintf(fid,"set palette negative defined ( \\\n");
    fprintf(fid,"    0 '#D53E4F',\\\n");
    fprintf(fid,"    1 '#F46D43',\\\n");
    fprintf(fid,"    2 '#FDAE61',\\\n");
    fprintf(fid,"    3 '#FEE08B',\\\n");
    fprintf(fid,"    4 '#E6F598',\\\n");
    fprintf(fid,"    5 '#ABDDA4',\\\n");
    fprintf(fid,"    6 '#66C2A5',\\\n");
    fprintf(fid,"    7 '#3288BD' )\n");
    fprintf(fid,"\n");
    fprintf(fid,"plot 'waterfall.bin' u 1:2:3 binary matrix with image\n");
    fclose(fid);
    printf("results written to %s.\n", "waterfall.bin");
    printf("results written to %s.\n", OUTPUT_FILENAME);

    printf("done.\n");
    return 0;
}

