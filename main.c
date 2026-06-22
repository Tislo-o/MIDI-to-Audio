#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "midi.h"
#include "wav.h"
#include "waveform.h"
#include "sound.h"

int main(int argc, char** argv) {
    assert(argc == 4 || argc == 5);

    clock_t start = clock();


    midi_t* m = read_midi(argv[1]);
    //print_midi_info(m);
    sorted_events_t* e = create_sorted_events(m);
    //print_sorted_event_infos(e);
    sound_t* result;
    sound_t* sample = NULL;

    if (argc == 4) { //mode synthétiseur
        result = create_sound(e, NULL, 0, argv[3], 0);
    } else {        //mode sampleur
        sample = read_wav(argv[3]);
        result = create_sound(e, sample, strtol(argv[4], NULL, 10), "", 1);
    }
    save_sound(argv[2], result);
    

    clock_t end = clock();
    double elapsed = (double)(end-start) / CLOCKS_PER_SEC;

    printf("Fichier");
    if (argc == 4) {
        printf(" mono ");
    } else {
        assert(sample != NULL);
        if (sample->channels == 1) printf(" mono ");
        else printf(" stereo ");
    }
    printf("%s généré (temps écoulé: %fs)\n", argv[2], elapsed);
    int duree = result->n_samples / 44100; //en secondes
    float taille = (result->n_samples * 2 + 44) / 1048576.f; //taille en Mo(1048576 octets = 1Mo)
    printf("Durée du fichier: %dm%ds (taille %fMo)\n", duree / 60, duree % 60, taille);

    if (sample != NULL) free_sound(sample);
    free_sound(result);
    free_sorted_events(e);
    free_midi(m);

    return  0;
}