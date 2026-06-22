#pragma once

#define _12thRootOf2  1.05946309436 //2^(1/12)
#include "sound.h"
#include "waveform.h"
#include <stdio.h>
#include <string.h>

//retourne la fréquence associée à la hauteur de note h
float pitch_to_freq(int h);

//retourne 2^(h/12)
float ratio(int h);

