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
    // Touch Control States
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
    "   FragColor = vec4(0.05, 0.08, 0.12, 1.0);\n" 
    "}\n";

extern "C" {
    void* Direct3DCreate9(uint32_t SDKVersion) {
        LOGI("D3D9 HOOK: Direct3DCreate9 active: %u", SDKVersion);
        return (void*)0x9999;
    }
    
    int DirectSoundCreate8(void* guid, void** ppDS8, void* pUnkOuter) {
        LOGI("AUDIO HOOK: DirectSoundCreate8 active.");
        if (ppDS8) *ppDS8 = (void*)0x8888;
        return 0;
    }
}

bool InitEngineMemoryPool() {
    g_engineCtx.streamingBuffer = malloc(4096 * 4096);
    if (g_engineCtx.streamingBuffer) {
        g_engineCtx.memoryPoolInitialized = true;
        return true;
    }
    return false;
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
    desiredSpec.samples = 2048;

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
    float x = 0, y = 0;
    bool isDown = false;
    bool isUp = false;

    if (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERMOTION) {
        x = event->tfinger.x * windowWidth;
        y = event->tfinger.y * windowHeight;
        isDown = true;
    } else if (event->type == SDL_FINGERUP) {
        x = event->tfinger.x * windowWidth;
        y = event->tfinger.y * windowHeight;
        isUp = true;
    }

    if (isDown || isUp) {
        // Layout: 
        // Left side: Steering Left/Right (X < windowWidth * 0.4)
        // Right side Bottom: Throttle (X > windowWidth * 0.7, Y > windowHeight * 0.5)
        // Right side Top: Brake (X > windowWidth * 0.7, Y <= windowHeight * 0.5)

        bool activeState = isDown;

        if (x < windowWidth * 0.2f) {
            g_engineCtx.steerLeftPressed = activeState;
            if (activeState) LOGI("CONTROL: Steer Left Pressed");
        } else if (x >= windowWidth * 0.2f && x < windowWidth * 0.4f) {
            g_engineCtx.steerRightPressed = activeState;
            if (activeState) LOGI("CONTROL: Steer Right Pressed");
        } else if (x > windowWidth * 0.7f) {
            if (y > windowHeight * 0.5f) {
                g_engineCtx.throttlePressed = activeState;
                if (activeState) LOGI("CONTROL: THROTTLE (GAS) Pressed!");
            } else {
                g_engineCtx.brakePressed = activeState;
                if (activeState) LOGI("CONTROL: BRAKE Pressed!");
            }
        }
    }
}

int main(int argc, char *argv[]) {
    LOGI("Starting MotoGP 08 Phase 10: Touch Controls Overlay...");

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
    
    snprintf(dataMixPath, sizeof(dataMixPath), "%s/DATA.MIX", basePath);
    snprintf(engineDllPath, sizeof(engineDllPath), "%s/Engine.dll", basePath);
    snprintf(cacheDirPath, sizeof(cacheDirPath), "%s/cache", basePath);

    mkdir(cacheDirPath, 0777);

    bool fileExists = (access(dataMixPath, F_OK) == 0);
    bool engineFileExists = (access(engineDllPath, F_OK) == 0);
    bool tocParsed = false;
    bool sectionsParsed = false;
    bool glContextInitialized = false;
    bool d3d9HookReady = false;
    bool engineLoopRunning = false;
    bool inputPollingActive = false;
    bool shaderPipelineReady = false;
    bool audioReady = false;
    uint32_t fileCount = 0;

    if (fileExists) {
        FILE* file = fopen(dataMixPath, "rb");
        if (file) {
            char header[5] = {0};
            if (fread(header, 1, 4, file) == 4) {
                if (fread(&fileCount, sizeof(uint32_t), 1, file) == 1) {
                    tocParsed = true;
                    g_engineCtx.activeAssetEntries = fileCount;
                }
            }
            fclose(file);
        }
    }

    if (engineFileExists) {
        FILE* dllFile = fopen(engineDllPath, "rb");
        if (dllFile) {
            char mzHeader[2] = {0};
            if (fread(mzHeader, 1, 2, dllFile) == 2 && mzHeader[0] == 'M' && mzHeader[1] == 'Z') {
                fseek(dllFile, 0x3C, SEEK_SET);
                int32_t peOffset = 0;
                if (fread(&peOffset, sizeof(int32_t), 1, dllFile) == 1) {
                    fseek(dllFile, peOffset, SEEK_SET);
                    char peSig[4] = {0};
                    if (fread(peSig, 1, 4, dllFile) == 4 && peSig[0] == 'P' && peSig[1] == 'E') {
                        sectionsParsed = true;
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

    int winWidth = 1280, winHeight = 720;
    SDL_GetWindowSize(window, &winWidth, &winHeight);

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (glContext) {
        glContextInitialized = true;
        void* d3dDevice = Direct3DCreate9(32);
        void* dsDevice = NULL;
        DirectSoundCreate8(NULL, &dsDevice, NULL);
        
        if (d3dDevice && dsDevice) {
            d3d9HookReady = true;
            if (InitEngineMemoryPool()) {
                engineLoopRunning = true;
                inputPollingActive = true;
                if (InitShaderPipeline()) {
                    shaderPipelineReady = true;
                    if (InitAudioSubsystem()) {
                        audioReady = true;
                        g_engineCtx.gameBooted = true;
                    }
                }
            }
        }
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        LOGE("SDL_CreateRenderer Error: %s", SDL_GetError());
        return -1;
    }

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

        // Clear Viewport
        glClearColor(0.05f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render Touch Controls HUD Overlay using SDL Renderer
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // 1. Steer Left Button (Left translucent box)
        SDL_SetRenderDrawColor(renderer, 0, 204, 255, g_engineCtx.steerLeftPressed ? 180 : 70);
        SDL_Rect steerLeftRect = {20, winHeight - 160, 100, 120};
        SDL_RenderFillRect(renderer, &steerLeftRect);

        // 2. Steer Right Button
        SDL_SetRenderDrawColor(renderer, 0, 204, 255, g_engineCtx.steerRightPressed ? 180 : 70);
        SDL_Rect steerRightRect = {140, winHeight - 160, 100, 120};
        SDL_RenderFillRect(renderer, &steerRightRect);

        // 3. Brake Button (Top Right)
        SDL_SetRenderDrawColor(renderer, 255, 60, 60, g_engineCtx.brakePressed ? 180 : 70);
        SDL_Rect brakeRect = {winWidth - 140, winHeight - 280, 110, 100};
        SDL_RenderFillRect(renderer, &brakeRect);

        // 4. Throttle / Gas Button (Bottom Right - Large)
        SDL_SetRenderDrawColor(renderer, 0, 255, 100, g_engineCtx.throttlePressed ? 200 : 80);
        SDL_Rect throttleRect = {winWidth - 160, winHeight - 160, 130, 130};
        SDL_RenderFillRect(renderer, &throttleRect);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    if (g_engineCtx.audioDevice > 0) {
        SDL_CloseAudioDevice(g_engineCtx.audioDevice);
    }
    if (g_engineCtx.streamingBuffer) {
        free(g_engineCtx.streamingBuffer);
    }
    if (glContextInitialized) {
        SDL_GL_DeleteContext(glContext);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
