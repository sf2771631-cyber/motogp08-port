#include <SDL.h>
#include <GLES3/gl3.h>
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

// Phase 4: D3D9 Compatibility Stub Hook
extern "C" {
    void* Direct3DCreate9(uint32_t SDKVersion) {
        LOGI("D3D9 HOOK: Direct3DCreate9 called with SDK Version: %u", SDKVersion);
        return (void*)0x9999; // Mock D3D9 device interface handle
    }
}

int main(int argc, char *argv[]) {
    LOGI("Starting MotoGP 08 Phase 4: D3D9 Translation Bridge Initialization...");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        LOGE("SDL_Init Error: %s", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

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
    bool glContextInitialized = false;
    bool d3d9HookReady = false;
    uint32_t fileCount = 0;

    // 1. MIX Pipeline
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

    // 2. Engine.dll PE Section Verification
    if (access(engineDllPath, F_OK) == 0) {
        FILE* dllFile = fopen(engineDllPath, "rb");
        if (dllFile) {
            char mzHeader[2] = {0};
            if (fread(mzHeader, 1, 2, dllFile) == 2 && mzHeader[0] == 'M' && mzHeader[1] == 'Z') {
                fseek(dllFile, 0x3C, SEEK_SET);
                int32_t peOffset = 0;
                if (fread(&peOffset, sizeof(int32_t), 1, dllFile) == 1) {
                    fseek(dllFile, peOffset, SEEK_SET);
                    char peSig[4] = {0};
                    if (fread(peSig, 1, 4, dllFile) == 4 && peSig[0] == 'P' && peSig[1] == 'E' && peSig[2] == 0 && peSig[3] == 0) {
                        engineParsed = true;
                        uint16_t machine = 0;
                        uint16_t numOfSections = 0;
                        fread(&machine, sizeof(uint16_t), 1, dllFile);
                        fread(&numOfSections, sizeof(uint16_t), 1, dllFile);
                        if (numOfSections > 0 && numOfSections < 96) {
                            sectionsParsed = true;
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
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );

    if (!window) {
        LOGE("SDL_CreateWindow Error: %s", SDL_GetError());
        return -1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (glContext) {
        glContextInitialized = true;
        // Test our D3D9 hook stub
        void* d3dDevice = Direct3DCreate9(32);
        if (d3dDevice) {
            d3d9HookReady = true;
            LOGI("SUCCESS: D3D9 Translation Bridge Stub Initialized!");
        }
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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

        // Phase 4 Status Colors: Racing Flame Amber / Neon Orange when D3D9 bridge is active
        if (d3d9HookReady) {
            SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255);    // Stage 10: Flame Amber (D3D9 Translation Bridge Ready!)
        } else if (glContextInitialized) {
            SDL_SetRenderDrawColor(renderer, 138, 43, 226, 255);    // Stage 9: Electric Violet
        } else if (sectionsParsed) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 204, 255);    // Stage 8: Teal
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

    if (glContextInitialized) {
        SDL_GL_DeleteContext(glContext);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
