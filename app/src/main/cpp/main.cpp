#include <jni.h>
#include <android/log.h>
#include <SDL.h>

#define LOG_TAG "MotoGP08Port"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {
    JNIEXPORT jint JNICALL
    Java_com_port_motogp08_MainActivity_nativeInit(JNIEnv *env, jobject thiz) {
        LOGI("Initializing MotoGP 08 Native Window...");

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
            LOGE("SDL_Init Error: %s", SDL_GetError());
            return -1;
        }

        // Create a 1080p Fullscreen Window
        SDL_Window* window = SDL_CreateWindow(
            "MotoGP 08 Port",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            1920, 1080,
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

        LOGI("SDL Native Window & Renderer Created Successfully!");

        // Active Game/Rendering Loop
        bool running = true;
        SDL_Event event;

        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
            }

            // Clear screen to Dark Racing Blue
            SDL_SetRenderDrawColor(renderer, 15, 25, 45, 255);
            SDL_RenderClear(renderer);

            // Present the frame
            SDL_RenderPresent(renderer);

            SDL_Delay(16); // Target ~60 FPS
        }

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();

        return 0;
    }
}
