#pragma once
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include "sound.h"

extern const char* midi_event_Types[7]; //extern permet de déclarer sans initialiser

enum event_Types{
    MIDI,
    Meta
};

typedef struct event {
    long unsigned int tick_time; //instant de l'événement en ticks, depuis le début du morceau
    long unsigned int us_time;  //instant de l'événement en micro secondes, depuis le début du morceau
    enum event_Types type;

    unsigned char* data;
    int data_length; //taille des données en octets

    unsigned char meta_type; //type d'un événement Meta

    const char* midi_type; //type d'un événement MIDI (Note Off, Note On...)
    int channel; //canal d'un événement MIDI (0-16)
}event_t;

typedef struct m_track {
    int length; //longeur de la piste en octets;
    int n_events; //nombre d'événements
    event_t** events; //tableau de pointeurs vers les événements de la piste
} m_track_t;

typedef struct MIDI_file {
    int format; //0: 1 piste  1: plusieurs pistes jouant simultanément 2: plusieurs piste indépendantes
    int n_trks; //nombre de pistes
    int ticksPerQuarter; //nombre de tick tous les temps
    m_track_t** tracks; //tableau de pointeurs vers les pistes 
} midi_t;



//lit/renvoie l'entier positif tenant sur 'bytes' octets depuis f en gros boutisme
//'bytes' doit valoir 2, 3 ou 4 !
unsigned int read_integer_be(FILE* f, int bytes);

//Interprète les premiers octets pointés par f
//comme une valeur encodée en VLQ en avancant de le fichier f, et renvoie l'entier ainsi lu.
unsigned long vlq_decode(FILE *f);

//lit toutes les informations du fichier midi en construisant une structure midi_t et la renvoie
midi_t* read_midi(const char* filename);

//affiche les informations de la structure midi_t* m
void print_midi_info(midi_t* m);

//libère toute la mémoire allouée pour le midi_t* m
void free_midi(midi_t* m);


typedef struct sorted_events {
    event_t** events; 
    int n_events;
}sorted_events_t;

//extrait tous les événements du midi_t* m et les classe dans leur ordre d'apparition, 
//retournant ainsi un sorted_events_t*
//initialise aussi l'attribut us_time des événements à partir des événements de tempo
sorted_events_t* create_sorted_events(midi_t* m);

//affiche les informations de la structure storted_events_t* e
void print_sorted_event_infos(sorted_events_t* e);

//libère toute la mémoire allouée pour le sorted_events_t* e
//Doit être appelé après free_midi(m), m étant le midi_t* à l'origine de la création de e
void free_sorted_events(sorted_events_t* e);

//génère un sound_t* à partir d'événements classés dans l'ordre d'apparition
//selon 2 modes:
//0: synthétiseur, génère le son en fonction de synthType valant sawtooth, square, triangle ou sine
//1: sampler, génère le son en fonction du sound_t* s ayant pour numéro de note baseNote
sound_t* create_sound(sorted_events_t* s_ev, sound_t* s, int baseNote, const char* synthType, bool mode);
