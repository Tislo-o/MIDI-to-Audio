#include "wav.h"
#include "midi.h"

void write_int(FILE* f, int a, int size) {
    for (int i = 0; i < size; ++i) {
        fprintf(f, "%c", (a >> (8 * i)) & 0xFF);
    }
}

void write_header(FILE* f, int n, int channels) {
    fprintf(f, "RIFF");
    write_int(f, 36 + 2*n*channels, 4);
    fprintf(f, "WAVEfmt ");
    write_int(f, 16, 4);
    write_int(f, 1, 2);
    write_int(f, channels, 2);
    write_int(f, 44100, 4);
    write_int(f, 88200*channels, 4);
    write_int(f, 2*channels, 2);
    write_int(f, 16, 2);
    fprintf(f, "data");
    write_int(f, 2*n*channels, 4);
}

void save_sound(char* filename, sound_t* s) {
    FILE* f = fopen(filename, "wb");//ouvrir en mode écriture binaire
    assert(f != NULL);
    write_header(f, s->n_samples, s->channels);
    for (int i = 0; i < s->n_samples * s->channels; ++i) {
        write_int(f, s->samples[i], 2);
    }
    fclose(f);
}   
unsigned int read_integer_le(FILE* f, int bytes) {
    unsigned char buffer[4];
    assert(fread(buffer,  1, bytes, f) == bytes);

    if (bytes == 2) return buffer[1] << 8 | buffer[0];
    if (bytes == 3) return buffer[2] << 16 | buffer[1] << 8 | buffer[0];


    return buffer[3] << 24 | buffer[2] << 16 | buffer[1] << 8 | buffer[0];
}


sound_t* read_wav(char* filename) {
    sound_t* r = malloc(sizeof(sound_t));

    FILE* f = fopen(filename, "rb");
    assert(f != NULL);

    unsigned char buffer[4];

    assert(fread(buffer,1,4,f)==4); // "RIFF"
    read_integer_le(f, 4);           // taille du fichier
    assert(fread(buffer,1,4,f)==4); // "WAVE"

    r->channels = 0;
    int block_align = 0;

    // parcourir les chunks pour avoir les infos de fmt et de data
    while(1) {
        assert(fread(buffer,1,4,f)==4);
        unsigned int size = read_integer_le(f,4);

        if(buffer[0]=='f' && buffer[1]=='m' && buffer[2]=='t' && buffer[3]==' ') {
            fseek(f, 2, SEEK_CUR); //  audioFormat
            r->channels = read_integer_le(f,2);
            fseek(f, 4, SEEK_CUR); //  sampleRate
            fseek(f, 4, SEEK_CUR); //  byteRate
            block_align = read_integer_le(f,2); 
            assert(read_integer_le(f,2) == 16); //  bitsPerSample
            fseek(f, size - 16, SEEK_CUR); // octets fmt restants
        }
        else if(buffer[0]=='d' && buffer[1]=='a' && buffer[2]=='t' && buffer[3]=='a') {

            assert(r->channels != 0 && block_align != 0); 
            r->n_samples = size / (block_align);
            break;
        }
        else {
            fseek(f, size + (size&1), SEEK_CUR); // passe les éventuels autre chunk en veillant à l'alignement(padding)
        }
    }
    r->samples = malloc(r->n_samples * r->channels * sizeof(int16_t));
    fread(r->samples, 2*r->channels, r->n_samples, f);  //récupérer les données

    return r;
}
