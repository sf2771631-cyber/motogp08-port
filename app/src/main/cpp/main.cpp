#include <jni.h>
#include <android/log.h>
#include <SDL.h>

#define LOG_TAG "MotoGP08Port"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {
    JNIEXPORT jint JNICALL
    Java_com_port_motogp08_MainActivity_nativeInit(JNIEnv *env, jobject thiz) {
        LOGI("Initializing MotoGP 08 Native Environment...");
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) != 0) {
            LOGE("SDL_Init Error: %s", SDL_GetError());
            return -1;
        }
        LOGI("SDL initialized successfully!");
        return 0;
    }
}
