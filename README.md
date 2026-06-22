# MIDI-to-Audio
Transforms a MIDI file into an audio(wav) file with a sound specified by the user. It is either a waveform(sawtooth/square/triangle/sine), either a transposed sample provided by the user. In that case, one has to add as addionnal argument the note number (69 for A4).

Examples:   
Compile: gcc sound.c melody.c waveform.c wav.c midi.c main.c -o app -lm   
Execute: ./app torrent.mid output.wav triangle   
          ./app fantaisie.mid output.wav sawtooth   
          ./app ballade.mid output.wav piano_a4.wav 69   
          ./app nocturne.mid output.wav guitar_c4.wav 60   
