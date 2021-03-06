#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>

#include "common/wave.h"
#include "ft8/pack.h"
#include "ft8/encode.h"
#include "ft8/pack_v2.h"
#include "ft8/encode_v2.h"


// Convert a sequence of symbols (tones) into a sinewave of continuous phase (FSK).
// Symbol 0 gets encoded as a sine of frequency f0, the others are spaced in increasing
// fashion.
void synth_fsk(const uint8_t *symbols, int num_symbols, float f0, float spacing, 
                float symbol_rate, float signal_rate, float *signal) {
    float phase = 0;
    float dt = 1/signal_rate;
    float dt_sym = 1/symbol_rate;
    float t = 0;
    int j = 0;
    int i = 0;
    while (j < num_symbols) {
        float f = f0 + symbols[j] * spacing;
        phase += 2 * M_PI * f / signal_rate;
        signal[i] = sin(phase);
        t += dt;
        if (t >= dt_sym) {
            // Move to the next symbol
            t -= dt_sym;
            ++j;
        }
        ++i;
    }
}


void usage() {
    printf("Generate a 15-second WAV file encoding a given message.\n");
    printf("Usage:\n");
    printf("\n");
    printf("gen_ft8 MESSAGE WAV_FILE\n");
    printf("\n");
    printf("(Note that you might have to enclose your message in quote marks if it contains spaces)\n");
}


int main(int argc, char **argv) {
    // Expect two command-line arguments
    if (argc < 3) {
        usage();
        return -1;
    }

    const char *message = argv[1];
    const char *wav_path = argv[2];

    // First, pack the text data into 72-bit binary message
    uint8_t packed[10];
    //int rc = packmsg(message, packed);
    int rc = ft8_v2::pack77(message, packed);
    if (rc < 0) {
        printf("Cannot parse message!\n");
        printf("RC = %d\n", rc);
        return -2;
    }

    printf("Packed data: ");
    for (int j = 0; j < 10; ++j) {
        printf("%02x ", packed[j]);
    }
    printf("\n");

    // Second, encode the binary message as a sequence of FSK tones
    uint8_t tones[NN];          // NN = 79, lack of better name at the moment
    //genft8(packed, 0, tones);
    ft8_v2::genft8(packed, tones);

    printf("FSK tones: ");
    for (int j = 0; j < NN; ++j) {
        printf("%d", tones[j]);
    }
    printf("\n");

    // Third, convert the FSK tones into an audio signal
    const int num_samples = (int)(0.5 + NN / 6.25 * 12000);
    const int num_silence = (15 * 12000 - num_samples) / 2;
    float signal[num_silence + num_samples + num_silence];
    for (int i = 0; i < num_silence + num_samples + num_silence; i++) {
        signal[i] = 0;
    }

    synth_fsk(tones, NN, 1000, 6.25, 6.25, 12000, signal + num_silence);
    save_wav(signal, num_silence + num_samples + num_silence, 12000, wav_path);

    return 0;
}
