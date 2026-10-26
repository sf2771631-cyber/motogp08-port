#include <SDL.h>
#include <android/log.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define LOG_TAG "MotoGP08Port"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

int main(int argc, char *argv[]) {
    LOGI("Starting MotoGP 08 3-Stage Diagnostic Parser...");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        LOGE("SDL_Init Error: %s", SDL_GetError());
        return -1;
    }

    const char* basePath = SDL_AndroidGetExternalStoragePath();
    if (!basePath) {
        basePath = "/sdcard/Android/data/com.port.motogp08/files";
    }

    char dataMixPath[512];
    snprintf(dataMixPath, sizeof(dataMixPath), "%s/DATA.MIX", basePath);

    LOGI("Target Path for fopen: %s", dataMixPath);

    bool fileExists = (access(dataMixPath, F_OK) == 0);
    bool headerParsed = false;

    if (fileExists) {
        LOGI("SUCCESS: DATA.MIX file detected on storage!");
        FILE* file = fopen(dataMixPath, "rb");
        if (file) {
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            rewind(file);

            char header[5] = {0};
            fread(header, 1, 4, file);
            fclose(file);

            LOGI("SUCCESS: Opened DATA.MIX! File Size: %ld bytes", fileSize);
            LOGI("MIX Header Magic Bytes: %s", header);
            headerParsed = true;
        } else {
            LOGE("ERROR: File exists but fopen failed! Errno: %d (%s)", errno, strerror(errno));
        }
    } else {
        LOGE("WAITING: DATA.MIX not found at target path.");
    }

    SDL_Window* window = SDL_CreateWindow(
        "MotoGP 08 Port",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        1280, 720,
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL
    );

    if (!window) {
        LOGE("SDL_CreateWindow Error: %s", SDL_GetError());
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        LOGE("SDL_CreateRenderer Error: %s", SDL_GetError());
        return -1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // 3-Stage Visual Status Colors:
        if (headerParsed) {
            SDL_SetRenderDrawColor(renderer, 220, 120, 20, 255); // Stage 3: Racing Gold/Orange (Parsed Successfully!)
        } else if (fileExists) {
            SDL_SetRenderDrawColor(renderer, 20, 100, 50, 255);   // Stage 2: Racing Green (File Found on Storage)
        } else {
            SDL_SetRenderDrawColor(renderer, 15, 25, 45, 255);    // Stage 1: Racing Blue (File Missing / Not Found)
        }

        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
