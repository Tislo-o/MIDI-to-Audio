#include "melody.h"
#include <assert.h>

float pitch_to_freq(int h) {
    return ratio(h) * 440.f;
}

float ratio(int h) {
    float ratio = 4.f; //objectif: ratio = 2^(h/12) à la fin de la fonction
    int current = 24; //la hauteur correspondant au ratio
    while (current > h) { //on descend en octave jusqu'à être en dessus de h
        ratio /= 2.f;
        current -= 12;
    }
    while (current < h) { //on monte note par note on multipliant par la racine douzième de 2
        ratio *= _12thRootOf2;
        ++current;
    }
    return ratio;
}
