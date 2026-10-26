#include <SDL.h>
#include <android/log.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#define LOG_TAG "MotoGP08Port"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

int main(int argc, char *argv[]) {
    LOGI("Starting MotoGP 08 PE Section & Import Analyzer...");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        LOGE("SDL_Init Error: %s", SDL_GetError());
        return -1;
    }

    const char* basePath = SDL_AndroidGetExternalStoragePath();
    if (!basePath) {
        basePath = "/sdcard/Android/data/com.port.motogp08/files";
    }

    char dataMixPath[512];
    char engineDllPath[512];
    char cacheDirPath[512];
    char extractedFilePath[512];
    
    snprintf(dataMixPath, sizeof(dataMixPath), "%s/DATA.MIX", basePath);
    snprintf(engineDllPath, sizeof(engineDllPath), "%s/Engine.dll", basePath);
    snprintf(cacheDirPath, sizeof(cacheDirPath), "%s/cache", basePath);
    snprintf(extractedFilePath, sizeof(extractedFilePath), "%s/extracted_test.bin", cacheDirPath);

    mkdir(cacheDirPath, 0777);

    bool fileExists = (access(dataMixPath, F_OK) == 0);
    bool headerParsed = false;
    bool tocParsed = false;
    bool entriesParsed = false;
    bool assetExtracted = false;
    bool engineParsed = false;
    bool sectionsParsed = false;
    uint32_t fileCount = 0;

    // 1. MIX Pipeline Verification
    if (fileExists) {
        FILE* file = fopen(dataMixPath, "rb");
        if (file) {
            char header[5] = {0};
            if (fread(header, 1, 4, file) == 4) {
                headerParsed = true;
                if (fread(&fileCount, sizeof(uint32_t), 1, file) == 1) {
                    tocParsed = true;
                    if (fileCount > 0 && fileCount < 100000) {
                        entriesParsed = true;
                        fseek(file, 2048, SEEK_SET);
                        char buffer[256];
                        size_t bytesRead = fread(buffer, 1, sizeof(buffer), file);
                        if (bytesRead > 0) {
                            FILE* outFile = fopen(extractedFilePath, "wb");
                            if (outFile) {
                                fwrite(buffer, 1, bytesRead, outFile);
                                fclose(outFile);
                                assetExtracted = true;
                            }
                        }
                    }
                }
            }
            fclose(file);
        }
    }

    // 2. Engine.dll PE Section & Header Verification
    if (access(engineDllPath, F_OK) == 0) {
        FILE* dllFile = fopen(engineDllPath, "rb");
        if (dllFile) {
            char mzHeader[2] = {0};
            if (fread(mzHeader, 1, 2, dllFile) == 2 && mzHeader[0] == 'M' && mzHeader[1] == 'Z') {
                fseek(dllFile, 0x3C, SEEK_SET);
                int32_t peOffset = 0;
                if (fread(&peOffset, sizeof(int32_t), 1, dllFile) == 1) {
                    fseek(dllFile, peOffset, SEEK_SET);
                    char peSignature[4] = {0};
                    if (fread(peSignature, 1, 4, dllFile) == 4 && peSignature[0] == 'P' && peSignature[1] == 'E' && peSignature[2] == 0 && peSignature[3] == 0) {
                        engineParsed = true;
                        
                        // Read PE File Header to get Number of Sections (located 6 bytes after PE signature start)
                        // PE Header format: Signature(4) + Machine(2) + NumberOfSections(2) ...
                        fseek(dllFile, peOffset + 4 + 2, SEEK_SET);
                        uint16_t numOfSections = 0;
                        if (fread(&numOfSections, sizeof(uint16_t), 1, dllFile) == 1) {
                            LOGI("SUCCESS: Engine.dll parsed! Number of PE sections: %u", numOfSections);
                            if (numOfSections > 0 && numOfSections < 100) {
                                sectionsParsed = true;
                            }
                        }
                    }
                }
            }
            fclose(dllFile);
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

        // 8-Stage Visual Status Colors:
        if (sectionsParsed) {
            SDL_SetRenderDrawColor(renderer, 75, 0, 130, 255);      // Stage 8: Racing Electric Indigo (PE Sections Verified!)
        } else if (engineParsed) {
            SDL_SetRenderDrawColor(renderer, 255, 30, 30, 255);    // Stage 7: Laser Red
        } else if (assetExtracted) {
            SDL_SetRenderDrawColor(renderer, 50, 255, 50, 255);    // Stage 6: Neon Lime
        } else if (entriesParsed) {
            SDL_SetRenderDrawColor(renderer, 0, 180, 180, 255);    // Stage 5: Neon Cyan
        } else if (tocParsed) {
            SDL_SetRenderDrawColor(renderer, 140, 20, 180, 255);  // Stage 4: Purple
        } else if (headerParsed) {
            SDL_SetRenderDrawColor(renderer, 220, 120, 20, 255); // Stage 3: Gold
        } else if (fileExists) {
            SDL_SetRenderDrawColor(renderer, 20, 100, 50, 255);   // Stage 2: Green
        } else {
            SDL_SetRenderDrawColor(renderer, 15, 25, 45, 255);    // Stage 1: Blue
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
