#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "sound.h"

//écrit l'entier a sur size octets dans le fichier f en little-endian
void write_int(FILE* f, int a, int size);

//écrit l'en-tête d'un fichier WAV à n échantillons, stereo ou non
void write_header(FILE* f, int n, int channels);

//sauvegarde le son s dans le fichier au nom filename
void save_sound(char* filename, sound_t* s);


//lit/renvoie l'entier positif tenant sur 'bytes' octets depuis f en petit boutisme
//'bytes' doit valoir 2, 3 ou 4 !
unsigned int read_integer_le(FILE* f, int bytes);

//transforme un fichier wav en un sound_t*
//ne tolère que les fichiers 16 bits !
sound_t* read_wav(char* filename);