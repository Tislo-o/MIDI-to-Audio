#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "waveform.h"
#include "melody.h"

sound_t* white(float duree, int f_ech) {
    int n = duree * f_ech + 1;

    sound_t* s = malloc(sizeof(sound_t));
    s->samples = malloc(n * sizeof(int16_t));
    s->channels = 1;

    s->n_samples = n;
    for (int i = 0; i < n; ++i) {
        s->samples[i] = (rand() % 65535) - 32768; //[-32768, 32767];
    }
    return s;
}

sound_t* sine(float freq, float amplitude, float duree, int f_ech) {
    int n = duree * f_ech + 1;

    sound_t* s = malloc(sizeof(sound_t));
    s->samples = malloc(n * sizeof(int16_t));
    s->channels = 1;

    s->n_samples = n;
    float env = 1.f;
    int fade_len = 3000;
    for (int i = 0; i < n; ++i) {
        if (i > s->n_samples - fade_len) {              //permet d'éviter les clics en 
            env = (float)(s->n_samples - i) / fade_len; //baissant le volume à la fin des notes
        }

        float normalized = env * amplitude * sin((_2PI * freq * i) / (float)f_ech);
        if (normalized > 1.f) normalized = 1.f;
        if (normalized < -1.f) normalized = -1.f;

        s->samples[i] = normalized * 32767;
    }
    return s;
}
sound_t* square(float freq, float amplitude, float duree, int f_ech) {
    int n = duree * f_ech + 1;

    sound_t* s = malloc(sizeof(sound_t));
    s->samples = malloc(n * sizeof(int16_t));
    s->channels = 1;

    s->n_samples = n;
    float progress = 0; //va de 0 à 1 à chaque période
    
    for (int i = 0; i < n; ++i) {
        progress += (freq/f_ech);
        if (progress >= 1.f) progress -= 1.f;

        float normalized = amplitude * (progress > 0.5f ? -1:1);
        if (normalized > 1.f) normalized = 1.f;
        if (normalized < -1.f) normalized = -1.f;

        s->samples[i] = normalized * 32767;
    }
    return s;
}
sound_t* triangle(float freq, float amplitude, float duree, int f_ech) {
    int n = duree * f_ech + 1;

    sound_t* s = malloc(sizeof(sound_t));
    s->samples = malloc(n * sizeof(int16_t));
    s->channels = 1;

    s->n_samples = n;
    float progress = 0; //va de 0 à 1 à chaque période
    float env = 1.f;
    int fade_len = 3000; //durée du fade out

    
    for (int i = 0; i < n; ++i) {
        progress += (freq/f_ech);
        if (progress >= 1.f) progress -= 1.f;

        if (i > s->n_samples - fade_len) {              //permet d'éviter les clics en 
            env = (float)(s->n_samples - i) / fade_len; //baissant le volume à la fin des notes
        }

        float normalized = env * amplitude * (2.f * fabsf(2.f * progress - 1.f) - 1.f);   
        if (normalized > 1.f) normalized = 1.f;     
        if (normalized < -1.f) normalized = -1.f;

        s->samples[i] = normalized * 32767;
    }
    return s;
}
sound_t* sawtooth(float freq, float amplitude, float duree, int f_ech) {
    int n = duree * f_ech + 1;

    sound_t* s = malloc(sizeof(sound_t));
    s->samples = malloc(n * sizeof(int16_t));
    s->channels = 1;

    s->n_samples = n;
    float progress = 0; //va de 0 à 1 à chaque période
    
    for (int i = 0; i < n; ++i) {
        progress += (freq/f_ech);
        if (progress >= 1.f) progress -= 1.f;

        float normalized = amplitude * (progress * 2.f - 1.f); //monte -1 -> 1
        if (normalized > 1.f) normalized = 1.f;
        if (normalized < -1.f) normalized = -1.f;

        s->samples[i] = normalized * 32767;
    }
    return s;
}

sound_t* pitch_sample(float amp, float duree, sound_t* src, int src_noteNumber, int dst_noteNumber) {
    sound_t* r = malloc(sizeof(sound_t));
    r->channels = src->channels;

    int maxSamples = duree * 44100;

    float multiplier = ratio(dst_noteNumber - src_noteNumber);

    r->n_samples = (int)(src->n_samples / multiplier);  //le nombre d'échantillon est le minimum entre celui de son
    if (maxSamples < r->n_samples) r->n_samples = maxSamples; //source et la celui imposé par la durée de la note

    float gain = amp / sqrtf(multiplier);
    float env = 1.f;
    int fade_len = 3000; //durée du fade out

    r->samples = malloc(r->n_samples * r->channels * sizeof(int16_t));

     for (int i = 0; i < r->n_samples; ++i) {
        float idx_f = i * multiplier;
        int idx0 = (int)idx_f;
        int idx1 = idx0 + 1 < src->n_samples ? idx0 + 1 : idx0;
        float frac = idx_f - idx0;

        if (i > r->n_samples - fade_len) {              //permet d'éviter les clics en 
            env = (float)(r->n_samples - i) / fade_len; //baissant le volume à la fin des notes
        }

        for (int c = 0; c < r->channels; ++c) {
            int16_t s0 = src->samples[idx0*src->channels + c];  //interpolation 
            int16_t s1 = src->samples[idx1*src->channels + c];  //entre les 2 échantillons
                                                                //les plus proches
            int32_t tmp = (int32_t)(((1-frac)*s0 + frac*s1) * gain * env);
            if (tmp > 32767) tmp = 32767;
            if (tmp < -32768) tmp = -32768;
            r->samples[i*r->channels + c] = (int16_t)tmp;
        }
        
    }

    return r;
}