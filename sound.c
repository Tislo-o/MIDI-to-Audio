#include "sound.h"
#include <stdio.h>
#include <assert.h>

void free_sound(sound_t* s) {
    free(s->samples);
    free(s);
}

void layerSound(sound_t* src, sound_t* dst, int offset) {
    assert(src->channels == dst->channels);
    assert(src->n_samples + offset <= dst->n_samples);

    for (int i = 0; i < src->n_samples * src->channels; ++i) {
        
        int sum = (int)dst->samples[i + offset*src->channels] + (int)src->samples[i];
        if (sum > 32767) sum = 32767;
        if (sum < -32767) sum = -32767;

        dst->samples[i + offset*src->channels] = sum;
    }
}
