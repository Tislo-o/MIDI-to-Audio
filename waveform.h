#pragma once
#include "sound.h"

#define _2PI  6.283185307

//crée un son aléatoire, un bruit blanc
sound_t* white(float duree, int f_ech);

//sons périodiques
sound_t* sine(float freq, float amplitude, float duree, int f_ech);
sound_t* square(float freq, float amplitude, float duree, int f_ech);
sound_t* triangle(float freq, float amplitude, float duree, int f_ech);
sound_t* sawtooth(float freq, float amplitude, float duree, int f_ech);

//transpose le sound_t* src de la note src_noteNumber vers la note dst_noteNumber
//le son généré a pour amplitude amp et a une durée maximum de duree
sound_t* pitch_sample(float amp, float duree, sound_t* src, int src_noteNumber, int dst_noteNumber);
