#include <SDL.h>
#include <android/log.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define LOG_TAG "MotoGP08Port"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

int main(int argc, char *argv[]) {
    LOGI("Starting MotoGP 08 Storage & File Initializer...");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        LOGE("SDL_Init Error: %s", SDL_GetError());
        return -1;
    }

    // Get Android Scoped Storage Path
    const char* basePath = SDL_AndroidGetExternalStoragePath();
    if (!basePath) {
        LOGE("Failed to get SDL External Storage Path!");
        basePath = "/sdcard/Android/data/com.port.motogp08/files";
    }

    // Construct paths
    char dirPath[512];
    char dataMixPath[512];
    snprintf(dirPath, sizeof(dirPath), "%s/motogp08", basePath);
    snprintf(dataMixPath, sizeof(dataMixPath), "%s/DATA.MIX", dirPath);

    LOGI("Target Directory Path: %s", dirPath);

    // Automatically create the folder if it doesn't exist
    struct st;
    if (access(dirPath, F_OK) != 0) {
        int result = mkdir(dirPath, 0777);
        if (result == 0) {
            LOGI("SUCCESS: Created directory %s", dirPath);
        } else {
            LOGE("ERROR: Failed to create directory %s", dirPath);
        }
    } else {
        LOGI("Directory already exists: %s", dirPath);
    }

    // Check if DATA.MIX is present
    bool fileExists = (access(dataMixPath, F_OK) == 0);
    if (fileExists) {
        LOGI("SUCCESS: DATA.MIX detected in folder!");
    } else {
        LOGE("WAITING: DATA.MIX not found at %s", dataMixPath);
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

        // Green if DATA.MIX is found, Blue if waiting for files
        if (fileExists) {
            SDL_SetRenderDrawColor(renderer, 20, 100, 50, 255); // Racing Green
        } else {
            SDL_SetRenderDrawColor(renderer, 15, 25, 45, 255);   // Racing Blue
        }

        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
