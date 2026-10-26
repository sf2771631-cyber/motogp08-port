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
    LOGI("Starting MotoGP 08 .MIX Table of Contents (TOC) Parser...");

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

    bool fileExists = (access(dataMixPath, F_OK) == 0);
    bool headerParsed = false;
    bool tocParsed = false;
    uint32_t fileCount = 0;

    if (fileExists) {
        FILE* file = fopen(dataMixPath, "rb");
        if (file) {
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            rewind(file);

            char header[5] = {0};
            size_t bytesRead = fread(header, 1, 4, file);
            if (bytesRead == 4) {
                headerParsed = true;
                LOGI("SUCCESS: DATA.MIX Header parsed! Size: %ld bytes", fileSize);

                // Read file count index (typically stored right after header in Milestone MIX structures)
                if (fread(&fileCount, sizeof(uint32_t), 1, file) == 1) {
                    LOGI("SUCCESS: Milestone .MIX TOC Traversed! Internal File Count: %u", fileCount);
                    tocParsed = true;
                } else {
                    LOGE("WARNING: Header read OK, but failed to read internal file count TOC.");
                }
            }
            fclose(file);
        } else {
            LOGE("ERROR: fopen failed! Errno: %d (%s)", errno, strerror(errno));
        }
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

        // 4-Stage Visual Status Colors:
        if (tocParsed) {
            SDL_SetRenderDrawColor(renderer, 140, 20, 180, 255);  // Stage 4: Racing Purple/Magenta (TOC Parsed!)
        } else if (headerParsed) {
            SDL_SetRenderDrawColor(renderer, 220, 120, 20, 255); // Stage 3: Racing Gold/Orange (Header Read)
        } else if (fileExists) {
            SDL_SetRenderDrawColor(renderer, 20, 100, 50, 255);   // Stage 2: Racing Green (File Found)
        } else {
            SDL_SetRenderDrawColor(renderer, 15, 25, 45, 255);    // Stage 1: Racing Blue (Missing)
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
