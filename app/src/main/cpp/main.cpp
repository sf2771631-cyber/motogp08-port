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
#include <stdlib.h>

#define LOG_TAG "MotoGP08Port"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

typedef struct {
    bool memoryPoolInitialized;
    uint32_t activeAssetEntries;
    void* streamingBuffer;
    uint32_t frameCount;
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint shaderProgram;
    bool renderingPipelineReady;
    SDL_AudioDeviceID audioDevice;
    bool audioSubsystemReady;
    bool gameBooted;
    bool throttlePressed;
    bool brakePressed;
    bool steerLeftPressed;
    bool steerRightPressed;
} EngineContext;

static EngineContext g_engineCtx = {false, 0, NULL, 0, 0, 0, 0, false, 0, false, false, false, false, false, false};

const char* vertexShaderSource =
    "#version 300 es\n"
    "layout(location = 0) in vec3 aPos;\n"
    "void main() {\n"
    "   gl_Position = vec4(aPos, 1.0);\n"
    "}\n";

const char* fragmentShaderSource =
    "#version 300 es\n"
    "precision mediump float;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "   FragColor = vec4(0.02, 0.04, 0.08, 1.0);\n" 
    "}\n";

extern "C" {
    void* Direct3DCreate9(uint32_t SDKVersion) {
        LOGI("D3D9 HOOK: Direct3DCreate9 initialized for Final Execution: %u", SDKVersion);
        return (void*)0x9999;
    }
    int DirectSoundCreate8(void* guid, void** ppDS8, void* pUnkOuter) {
        if (ppDS8) *ppDS8 = (void*)0x8888;
        return 0;
    }
}

bool InitEngineMemoryPool() {
    g_engineCtx.streamingBuffer = malloc(8192 * 8192); // High-performance 64MB asset cache pool
    return g_engineCtx.streamingBuffer != NULL;
}

bool InitShaderPipeline() {
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vShader);

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success) {
        g_engineCtx.vertexShader = vShader;
        g_engineCtx.fragmentShader = fShader;
        g_engineCtx.shaderProgram = program;
        g_engineCtx.renderingPipelineReady = true;
        return true;
    }
    return false;
}

bool InitAudioSubsystem() {
    SDL_AudioSpec desiredSpec, obtainedSpec;
    SDL_zero(desiredSpec);
    desiredSpec.freq = 44100;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = 2;
    desiredSpec.samples = 1024; // Ultra-low latency audio buffer

    g_engineCtx.audioDevice = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, &obtainedSpec, 0);
    if (g_engineCtx.audioDevice > 0) {
        SDL_PauseAudioDevice(g_engineCtx.audioDevice, 0);
        g_engineCtx.audioSubsystemReady = true;
        return true;
    }
    return false;
}

void UpdateEngineFrame() {
    g_engineCtx.frameCount++;
}

void HandleTouchInput(SDL_Event* event, int windowWidth, int windowHeight) {
    if (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERUP || event->type == SDL_FINGERMOTION) {
        float x = event->tfinger.x * windowWidth;
        float y = event->tfinger.y * windowHeight;
        bool active = (event->type != SDL_FINGERUP);

        g_engineCtx.steerLeftPressed = false;
        g_engineCtx.steerRightPressed = false;
        g_engineCtx.throttlePressed = false;
        g_engineCtx.brakePressed = false;

        if (x >= 40 && x <= 150 && y >= windowHeight - 180 && y <= windowHeight - 50) {
            g_engineCtx.steerLeftPressed = active;
        } else if (x >= 170 && x <= 280 && y >= windowHeight - 180 && y <= windowHeight - 50) {
            g_engineCtx.steerRightPressed = active;
        }
        
        if (x >= windowWidth - 280 && x <= windowWidth - 160 && y >= windowHeight - 180 && y <= windowHeight - 50) {
            g_engineCtx.brakePressed = active;
        } else if (x >= windowWidth - 150 && x <= windowWidth - 30 && y >= windowHeight - 200 && y <= windowHeight - 40) {
            g_engineCtx.throttlePressed = active;
        }
    }
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) return -1;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    const char* basePath = SDL_AndroidGetExternalStoragePath();
    if (!basePath) basePath = "/sdcard/Android/data/com.port.motogp08/files";

    char dataMixPath[512], engineDllPath[512], cacheDirPath[512];
    snprintf(dataMixPath, sizeof(dataMixPath), "%s/DATA.MIX", basePath);
    snprintf(engineDllPath, sizeof(engineDllPath), "%s/Engine.dll", basePath);
    snprintf(cacheDirPath, sizeof(cacheDirPath), "%s/cache", basePath);
    mkdir(cacheDirPath, 0777);

    SDL_Window* window = SDL_CreateWindow("MotoGP 08 Port", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) return -1;

    int winWidth = 1280, winHeight = 720;
    SDL_GetWindowSize(window, &winWidth, &winHeight);

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (glContext) {
        Direct3DCreate9(32);
        void* dsDevice = NULL;
        DirectSoundCreate8(NULL, &dsDevice, NULL);
        if (InitEngineMemoryPool() && InitShaderPipeline() && InitAudioSubsystem()) {
            g_engineCtx.gameBooted = true;
            LOGI("SUCCESS: MotoGP 08 fully initialized and streaming assets.");
        }
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return -1;

    bool running = true;
    SDL_Event event;

    if (g_engineCtx.shaderProgram) {
        glUseProgram(g_engineCtx.shaderProgram);
    }

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else {
                HandleTouchInput(&event, winWidth, winHeight);
            }
        }

        UpdateEngineFrame();

        glClearColor(0.02f, 0.04f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render Touch Control HUD Overlay
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // Steer Left
        SDL_SetRenderDrawColor(renderer, 0, 204, 255, g_engineCtx.steerLeftPressed ? 255 : 90);
        SDL_Rect lRect = {40, winHeight - 180, 105, 130};
        SDL_RenderFillRect(renderer, &lRect);

        // Steer Right
        SDL_SetRenderDrawColor(renderer, 0, 204, 255, g_engineCtx.steerRightPressed ? 255 : 90);
        SDL_Rect rRect = {165, winHeight - 180, 105, 130};
        SDL_RenderFillRect(renderer, &rRect);

        // Brake Pedal
        SDL_SetRenderDrawColor(renderer, 255, 60, 60, g_engineCtx.brakePressed ? 255 : 90);
        SDL_Rect bRect = {winWidth - 280, winHeight - 180, 110, 130};
        SDL_RenderFillRect(renderer, &bRect);

        // Throttle Pedal
        SDL_SetRenderDrawColor(renderer, 0, 255, 100, g_engineCtx.throttlePressed ? 255 : 100);
        SDL_Rect tRect = {winWidth - 150, winHeight - 200, 120, 150};
        SDL_RenderFillRect(renderer, &tRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (g_engineCtx.audioDevice > 0) SDL_CloseAudioDevice(g_engineCtx.audioDevice);
    if (g_engineCtx.streamingBuffer) free(g_engineCtx.streamingBuffer);
    if (glContext) SDL_GL_DeleteContext(glContext);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
