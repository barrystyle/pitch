// Copyright (c) 2026 barrystyle

#include <mnemonic.h>

#define BITLEN 192
#define SAMPLE_RATE 44100
#define WINDOW_SIZE 2048
#define HOP_SIZE 512
#define MIN_FREQ 60.0
#define MAX_FREQ 1500.0
#define SILENCE_RMS 0.01
#define CLARITY_THRESHOLD 0.05

void safe_to_return(int& errorLevel)
{
    errorLevel = 0;
    std::vector<unsigned char> buffer(16);
    std::ifstream urandom("/dev/urandom", std::ios::in | std::ios::binary);
    if (!urandom) {
        errorLevel = 1;
        return;
    }
    while (true) {
        urandom.read(reinterpret_cast<char*>(buffer.data()), 16);
        if (!urandom) {
            errorLevel = 1;
            return;
        }
        if (*(uint32_t*)&buffer[0] < 0x1000) {
            return;
        }
    }
}

void print_entropy(std::vector<uint8_t>& entropy, int offset, int bitlen = -1)
{
    if (bitlen >= 0) {
        for (int i = 0; i < bitlen / 8; i++) {
            printf("%02hhx", entropy[i]);
        }
        printf("\n");
        return;
    }
    if (offset < 0) {
        for (int i = 0; i < 32 * (offset == -1 ? 32 : 1); i++) {
            printf("%02hhx", entropy[i]);
        }
        printf("\n\n");
        return;
    }
    for (int i = offset * 32; i < (offset * 32) + 32; i++) {
        printf("%02hhx", entropy[i]);
    }
    printf("\n");
}

static void observe_silence(const float* buf, int n, int sample_rate, double* clarity_out)
{
    if (clarity_out)
        *clarity_out = 0.0;

    double sumsq = 0.0;
    for (int i = 0; i < n; i++)
        sumsq += (double)buf[i] * buf[i];

    double rms = sqrt(sumsq / n);
    if (rms < SILENCE_RMS || sumsq <= 0.0)
        return;

    int min_lag = (int)(sample_rate / MAX_FREQ);
    int max_lag = (int)(sample_rate / MIN_FREQ);
    if (max_lag >= n)
        max_lag = n - 1;
    if (min_lag < 1)
        min_lag = 1;
    if (min_lag > max_lag)
        return; /* no valid lag range */

    double best_r = -1.0;
    for (int lag = min_lag; lag <= max_lag; lag++) {
        double r = 0.0;
        int m = n - lag;
        for (int i = 0; i < m; i++)
            r += (double)buf[i] * buf[i + lag];
        r /= sumsq;
        if (r > best_r)
            best_r = r;
    }

    if (clarity_out && best_r > 0.0)
        *clarity_out = best_r;
}

int main(int argc, char** argv)
{

    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_F32SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = NULL;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 1, &want, &have, 0);
    if (dev == 0) {
        fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_PauseAudioDevice(dev, 0);

    float* window = (float*)calloc(WINDOW_SIZE, sizeof(float));
    float* hopbuf = (float*)calloc(HOP_SIZE, sizeof(float));
    if (!window || !hopbuf) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    const uint32_t hop_bytes = HOP_SIZE * sizeof(float);

    printf("gathering entropy from audio (%dbits)..\n\n", BITLEN);

    std::vector<uint8_t> entropy;
    entropy.resize(WINDOW_SIZE);
    int let_buf = 0, used_buf = 0, skip_err = 0;

    while (true) {

        while (SDL_GetQueuedAudioSize(dev) < hop_bytes) {
            SDL_Delay(2);
        }

        // sample audio from input
        uint32_t got = SDL_DequeueAudio(dev, hopbuf, hop_bytes);
        if (got < hop_bytes) {
            memset(((uint8_t*)hopbuf) + got, 0, hop_bytes - got);
        }

        // shift buffer per pass
        memmove(window, window + HOP_SIZE, (WINDOW_SIZE - HOP_SIZE) * sizeof(float));
        memcpy(window + (WINDOW_SIZE - HOP_SIZE), hopbuf, HOP_SIZE * sizeof(float));

        // observe buffer and determine if it is silent
        double clarity = 0.0;
        observe_silence(window, WINDOW_SIZE, have.freq, &clarity);
        bool is_silent = !(clarity > CLARITY_THRESHOLD);

        // update entropy if buffer is full (not a partial)
        if (!is_silent)
        {
            // create a entirely random delay (pow-style)
            safe_to_return(skip_err);
            if (skip_err > 0) {
                printf("fatal error in random\n");
                return 0;
            }
            // ensure we only process full frames
            if (let_buf > 0) {
                let_buf--;
            } else {
                // sha256 buf into entropy[usedbuf*32]
                SHA256((const unsigned char*)window, WINDOW_SIZE, entropy.data() + (used_buf * 32));
                print_entropy(entropy, used_buf);
                used_buf++;
                if (used_buf >= 32) {
                    // sha256 compounded entropy back to entropy[0]
                    print_entropy(entropy, -1);
                    SHA256(entropy.data(), 32 * 32, entropy.data());
                    // print_entropy(entropy, -2);
                    print_entropy(entropy, 0, BITLEN);
                    // and process the result..
                    std::string mnemonic;
                    std::vector<uint8_t> seed;
                    entropy_to_mnemonic((char*)entropy.data(), BITLEN, mnemonic);
                    printf("%s\n", mnemonic.c_str());
                    mnemonic_to_seed(mnemonic, std::string(""), seed);
                    calculate_from_seed(seed);
                    break;
                }
            }
        } else {
            let_buf++;
            if (let_buf > WINDOW_SIZE / HOP_SIZE) {
                let_buf = WINDOW_SIZE / HOP_SIZE;
            }
        }

        fflush(stdout);
    }

    printf("\nStopping, closing audio device...\n");
    free(window);
    free(hopbuf);
    SDL_CloseAudioDevice(dev);
    SDL_Quit();
    return 0;
}
