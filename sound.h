#pragma once

#include <stdlib.h>
#include <stdint.h>

typedef struct sound{
    int n_samples; //nombre d'échantillons, indépendent du nombre de canaux  
    int channels; //1: mono   2:stereo
    int16_t* samples; //échantillons, alternant canaux gauche et droite si stereo
} sound_t;

//libère la mémoire allouée pour s de type sound_t
void free_sound(sound_t* s);

//superpose le sound_t* src sur le sound_t* dst à partir du offset-ième échantillon
void layerSound(sound_t* src, sound_t* dst, int offset);



