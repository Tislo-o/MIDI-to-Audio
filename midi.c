#include "midi.h"
#include "waveform.h"
#include "melody.h"
#include <stdbool.h>
#include <math.h>

/*Nouvelles fonctions C utilisées:

int fseek(FILE  *stream, long offset, int whence); 
-> avance le pointeur 'stream' de 'offset' octets depuis 'whence'

size_t fread(void* ptr, size_t size, size_t n, FILE* stream);
-> stocke 'n' éléments de 'size' octets depuis 'stream' dans 'ptr'

long ftell(FILE *__stream)
-> Donne la position du pointeur __stream dans le fichier
*/

const char* midi_event_Types[7] = {
    "Note_Off",
    "Note_On",
    "Polyphonic_Key_Pressure",
    "Control_Change",
    "Program_Change",
    "Channel_Pressure",
    "Pitch_Wheel_Change",
};

unsigned int read_integer_be(FILE* f, int bytes) {
    unsigned char buffer[4];
    assert(fread(buffer,  1, bytes, f) == bytes);

    if (bytes == 2) return buffer[0] << 8 | buffer[1];
    if (bytes == 3) return buffer[0] << 16 | buffer[1] << 8 | buffer[2];


    return buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
}

unsigned long vlq_decode(FILE *f) {
    unsigned long value = 0;
    unsigned char byte;

    do {
        fread(&byte, 1, 1, f);
        value = (value << 7) | (byte & 0x7F); //union de la valeur précédente décalée de 7 bits
                                              //avec les 7 premiers bits de la nouvelle valeur
    } while (byte & 0x80);

    return value;
}

midi_t* read_midi(const char* filename) {
    midi_t* r = malloc(sizeof(midi_t));

    FILE* f = fopen(filename, "rb"); //ouvrir en binaire pour avoir accès aux octets bruts
    assert(f != NULL);

    //------------1 Lecture du header-------------//

    //S'assure qu'on est bien au début d'un fichier midi: "Mthd" encodé au début 
    //puis 6 (taille de header) sur 4 octets;
    unsigned char chunkType[4];
    fread(chunkType, 1, 4, f);
    assert(chunkType[0] == 'M' && chunkType[1] == 'T' && chunkType[2] == 'h' && chunkType[3] == 'd');
    assert(read_integer_be(f, 4) == 6);

    r->format = read_integer_be(f, 2);
    r->n_trks = read_integer_be(f, 2);
    r->ticksPerQuarter = read_integer_be(f, 2);
    assert((r->ticksPerQuarter & (1 << 15)) == 0); //dans le cas contraire le format SMPTE est utilisé(non pris en charge ici) 
    
    //------------2 Lecture de chaque piste--------//

    r->tracks = malloc(r->n_trks * sizeof(m_track_t*));
    for (int i = 0; i < r->n_trks; ++i) {
        r->tracks[i] = malloc(sizeof(m_track_t));
        m_track_t* t = r->tracks[i];

        //S'assure qu'on est bien au début d'une piste : "Mtrk" encodé au début 
        fread(chunkType, 1, 4, f);
        assert(chunkType[0] == 'M' && chunkType[1] == 'T' && chunkType[2] == 'r' && chunkType[3] == 'k');

        t->length = read_integer_be(f, 4);
        t->events = NULL;
        t->n_events = 0;
        long track_end = ftell(f) + t->length;
        unsigned char running_status;
        long unsigned int time = 0;

        //----------Lecture de chaque événement--------//

        while (ftell(f) < track_end) { //tant qu'on a pas parcouru toute la piste
            event_t* e = malloc(sizeof(event_t));

            time += vlq_decode(f); //la première chose encodée est le delta-time en format VLQ
            e->tick_time = time;

            unsigned char b; 
            fread(&b, 1, 1, f);  //lire le premier octet

            if (b == 0xF0 || b == 0xF7) { //type SysEx, on le saute
                free(e);
                e = NULL;

                int length = vlq_decode(f);
                fseek(f, length, SEEK_CUR);

            } else if (b == 0xFF) { //type Meta
                e->type = Meta;
                t->n_events++;

                fread(&e->meta_type, 1, 1, f); //lit le type

                e->data_length = vlq_decode(f); //lit la taille des données
                e->data = malloc(e->data_length); //alloue la mémoire pour les données
                fread(e->data, 1, e->data_length, f); //lit les données

            } else { //type MIDI
                e->type = MIDI;
                t->n_events++;

                if (b >= 0x80) {  //Si le bit 1000 0000 est à 1, alors b est un octet de statut
                    running_status = b;
                } else {                    //Sinon b était en fait un octet de données donc on revient en arrière 
                    fseek(f, -1, SEEK_CUR); //d'1 octet et on utilise le statut en cours
                }
                int midi_type_index = (running_status & 0x70) >> 4; 
                assert(midi_type_index >= 0 && midi_type_index <= 6); 

                e->midi_type = midi_event_Types[midi_type_index];
                if (midi_type_index == 4 || midi_type_index == 5) { //la taille des données
                    e->data_length = 1;                             //varie en fonction du
                } else {                                            //type d'événement MIDI
                    e->data_length = 2;
                }

                e->channel = running_status & 0x0F; //récupère ces bits: 0000 1111, qui constituent le canal

                e->data = malloc(e->data_length);
                fread(e->data, 1, e->data_length, f);
            }    
            if (e != NULL) {
                
                t->events = realloc(t->events, t->n_events * sizeof(event_t*)); //augmente de 1 la taille du tableau de pointeurs
                t->events[t->n_events-1] = e;  //place l'événement créé à la fin
            }
        }
    }

    fclose(f);
    return r;
}

sorted_events_t* create_sorted_events(midi_t* m) {
    sorted_events_t* r = malloc(sizeof(sorted_events_t));

    unsigned long int usPerQuarter = 1000000; //correspond à un tempo de 60
    unsigned long int current_us = 0; //temps atctuel en micro secondes
    unsigned long int last_tick = 0; //tick du dernier événement 

    int* idx = malloc(m->n_trks * sizeof(int)); //tableau contenant l'indice courant pour chaque piste
    int eventCount = 0;
    //Initialise les indices à 0 et compte le nombre total d'événements
    for (int i = 0; i < m->n_trks; ++i) {
        idx[i] = 0;
        eventCount += m->tracks[i]->n_events;
    }
    r->events = malloc(sizeof(event_t*) * eventCount);
    r->n_events = eventCount;

    //Construit la liste des événements triés en prenant celui 
    //qui arrive le plus tôt à chaque fois parmi toutes les pistes
    for (int eventIdx = 0; eventIdx < eventCount; ++eventIdx) {
        unsigned long int lowest_time = __UINT32_MAX__;
        int lowest_track = 0;
        
        for (int i = 0; i < m->n_trks; ++i) {
            if (idx[i] >= m->tracks[i]->n_events) continue; //Si jamais on a déjà épuisé tous les événements de la piste, 
                                                            //on passe à la suivante
            unsigned long int time = m->tracks[i]->events[idx[i]]->tick_time;
            if (time < lowest_time) {
                lowest_time = time;
                lowest_track = i;
            }
        }
        r->events[eventIdx] = m->tracks[lowest_track]->events[idx[lowest_track]];
        ++idx[lowest_track];
        
        event_t* e = r->events[eventIdx];

        //on avance dans le temps par rapport l'événement précédent
        current_us += (e->tick_time - last_tick) * usPerQuarter / m->ticksPerQuarter;
        e->us_time = current_us;
        last_tick = e->tick_time;

        //Si l'événement ajouté est un événement de tempo
        if (e->type == Meta && e->meta_type == 0x51) {
            usPerQuarter = e->data[0] << 16 | e->data[1] << 8 | e->data[2]; 
        }   
    }

    free(idx);
    return r;
}

sound_t* create_sound(sorted_events_t* s_ev, sound_t* s, int baseNote, const char* synthType, bool mode) {
    sound_t* r = malloc(sizeof(sound_t));

    if (mode) {                     //si mode sampler
        r->channels = s->channels;  //on prend le nombre de canaux du sample
    } else {                        //sinon
        r->channels = 1;            //on crée un son mono
    }

    unsigned long lastEventTime = s_ev->events[s_ev->n_events-1]->us_time;

    r->n_samples = lastEventTime * 44100ULL / 1000000ULL; //On sait qu'au maximum le son s'arrête au dernier événement
    r->samples = malloc(r->n_samples * r->channels * sizeof(int16_t)); 
    
    //Initialiser tout les échantillon à zéro 
    for (int i = 0; i < r->n_samples * r->channels; ++i) {
        r->samples[i] = 0;
    }

    int sampleProgression = 0; //sera le nombre exact d'échantillons nécessaires pour encoder toutes les notes

    int allocated_pending = 1; //nombre d'event_t alloués, pointés par pending_events
    int currently_pending = 0; //nombre d'event_t en attente
    event_t** pending_events = malloc(sizeof(event_t*)); //événements en attente d'un autre pour former une note

    for (int i = 0; i < s_ev->n_events; ++i) {
        event_t* e = s_ev->events[i];

        if (e->type == MIDI) {
            bool Note_On = (strcmp(e->midi_type, "Note_On") == 0);
            bool Note_Off = (strcmp(e->midi_type, "Note_Off") == 0);

            if (Note_On || Note_Off) {
                unsigned char noteNumber = e->data[0] & 0x7F;
                unsigned char velocity = e->data[1] & 0x7F;
                unsigned char channel = e->channel;

                if (velocity == 0 || Note_Off) {    //Si l'événement marque la fin d'une note,
                    assert(currently_pending > 0);  //on parcourt tous les événements en attente

                    for (int j = currently_pending-1; j >= 0; --j) {
                        event_t* e2 = pending_events[j];

                        if ((e2->data[0] & 0x7F) == noteNumber && e2->channel == channel) { //Si les 2 événements ont les
                                                                                            //mêmes notes et canaux, on forme une note
                            float freq = pitch_to_freq(noteNumber - 69);                //noteNumber = 69 correspond au LA 440 Hz
                            float amp = (e2->data[1] & 0x7F) / (float)127;              //amplitude normalisée, entre 0 et 1
                            float duree = (float)(e->us_time - e2->us_time) / 1000000; //durée de la note en seconde
                            int offset = e2->us_time  * 44100ULL / 1000000ULL;         //nombre d'échantillons avant le début de la note

                            int newProgression = offset + (e->us_time - e2->us_time) * 44100ULL / 1000000ULL;  //nombre d'échantillons avant la fin de la note
                            if (newProgression > sampleProgression) sampleProgression = newProgression;

                            sound_t* note = NULL;
                            if (mode) { //si mode sampler
                                note = pitch_sample(amp, duree, s, baseNote, noteNumber);
                            } else {
                                if (strcmp(synthType,"sawtooth")==0) note = sawtooth(freq, amp, duree, 44100);
                                else if (strcmp(synthType,"square")==0) note = square(freq, amp, duree, 44100);
                                else if (strcmp(synthType,"triangle")==0) note = triangle(freq, amp, duree, 44100);
                                else if (strcmp(synthType, "sine")==0) note = sine(freq, amp, duree, 44100);
                            }
                            assert(note != NULL);
                            
                            layerSound(note, r, offset);

                            free_sound(note);

                            //Place le dernier événement de la file d'attente au niveau de celui qu'on veut supprimer
                            pending_events[j] = pending_events[currently_pending-1];
                            pending_events[currently_pending-1] = NULL;

                            --currently_pending;
                            break;
                        }
                    }
                } else {    //L'événement marque le début d'une note, on l'ajoute à ceux en attente
                    if (currently_pending == allocated_pending) {
                        allocated_pending *= 2;
                        pending_events = realloc(pending_events, allocated_pending * sizeof(event_t*));
                        assert(pending_events != NULL);
                    }
                    pending_events[currently_pending] = e;
                    ++currently_pending;
                }
            }
        } 
    }
    assert(currently_pending == 0);
    if (pending_events != NULL) free(pending_events);
    assert(sampleProgression <= r->n_samples);
    r->n_samples = sampleProgression;

    return r;
}

//-----------Fonctions pour libérer la mémoire-------------//

void free_midi(midi_t* m) {
    for (int i = 0; i < m->n_trks; ++i) {
        m_track_t* t = m->tracks[i];
        for (int j = 0; j < t->n_events; ++j) {
            free(t->events[j]->data);
            free(t->events[j]);
        }

        free(t->events);
        free(t);
    }
    free(m->tracks);
    free(m);
}

void free_sorted_events(sorted_events_t* e) {
    //pas besoin de free les événements eux-même puisqu'ils ont déjà été free par free_midi()
    free(e->events);
    free(e);
}

//-----------Fonctions d'affichage---------------//

void print_midi_info(midi_t* m) {

    printf("format: %d   n_trks: %d   ticksPerQuarter: %d\n", m->format, m->n_trks, m->ticksPerQuarter);
    for (int i = 0; i < m->n_trks; ++i) {
        m_track_t* t = m->tracks[i];
        printf("\nPiste #%d   taille: %d octets  événements: %d\n\n", i, t->length, t->n_events);

        for (int j = 0; j < t->n_events && j < 100; ++j) {
            event_t* e = t->events[j];

            printf("Evénement #%d  time: %u  type: ", j, e->tick_time);
            if (e->type == Meta) {
                printf("Meta  meta_type: 0x%02X\n", e->meta_type);
            } else {
                printf("Midi  midi_type: %s  canal: %d\n", e->midi_type, e->channel);
            }
        }
    }
}

void print_sorted_event_infos(sorted_events_t* s) {

    for (int j = 0; j < s->n_events && j < 5000; ++j) {
        event_t* e = s->events[j];

        printf("Evénement #%d  time: %u  type: ", j, e->tick_time);
        if (e->type == Meta) {
            printf("Meta  meta_type: 0x%02X\n", e->meta_type);
        } else {
            printf("Midi  midi_type: %s  canal: %d", e->midi_type, e->channel);
            if (strcmp(e->midi_type, "Note_On") == 0 || strcmp(e->midi_type, "Note_Off") == 0) {
                printf("  note number: %d    velocity: %d", e->data[0] & 0x7F, e->data[1] & 0x7F);
            }
            printf("\n");
        }
    }
}
