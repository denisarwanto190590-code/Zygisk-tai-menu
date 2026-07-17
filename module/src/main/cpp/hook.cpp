#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/system_properties.h>
#include <dlfcn.h>
#include <cstdlib>
#include <cinttypes>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_android.h"
#include "KittyMemory/KittyMemory.h"
#include "KittyMemory/MemoryPatch.h"
#include "KittyMemory/KittyScanner.h"
#include "KittyMemory/KittyUtils.h"
#include "Includes/Dobby/dobby.h"
#include "Include/Unity.h"
#include "Misc.h"
#include "hook.h"
#include "Include/Roboto-Regular.h"
#include <iostream>
#include <chrono>
#include "Include/Quaternion.h"
#include "Rect.h"
#include <limits>

#define GamePackageName "com.dts.freefireth" 

int glHeight, glWidth;
char* game_data_dir = nullptr;
bool enable_hack = false;
ProcMap g_il2cppBaseMap;

int isGame(JNIEnv *env, jstring appDataDir)
{
    if (!appDataDir)
        return 0;
    const char *app_data_dir = env->GetStringUTFChars(appDataDir, nullptr);
    int user = 0;
    static char package_name[256];
    if (sscanf(app_data_dir, "/data/%*[^/]/%d/%s", &user, package_name) != 2) {
        if (sscanf(app_data_dir, "/data/%*[^/]/%s", package_name) != 1) {
            package_name[0] = '\0';
            LOGW("can't parse %s", app_data_dir);
            return 0;
        }
    }
    if (strcmp(package_name, GamePackageName) == 0) {
        LOGI("detect game: %s", package_name);
        game_data_dir = new char[strlen(app_data_dir) + 1];
        strcpy(game_data_dir, app_data_dir);
        env->ReleaseStringUTFChars(appDataDir, app_data_dir);
        return 1;
    } else {
        env->ReleaseStringUTFChars(appDataDir, app_data_dir);
        return 0;
    }
}

bool setupimg;

// --- PERBAIKAN: Deklarasi pointer langsung mengambil extern milik menu.h agar tidak duplikat ---
extern EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf);
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf); 

// --- HOOK INPUT SENTUH ---
void (*origInput)(void *thiz, void *ex_ab, void *ex_ac);
void myInput(void *thiz, void *ex_ab, void *ex_ac)
{
    origInput(thiz, ex_ab, ex_ac);
    ImGui_ImplAndroid_HandleInputEvent((AInputEvent *)thiz);
    return;
}

int32_t (*origConsume)(void *thiz, void *arg1, bool arg2, long arg3, uint32_t *arg4, AInputEvent **input_event);
int32_t myConsume(void *thiz, void *arg1, bool arg2, long arg3, uint32_t *arg4, AInputEvent **input_event)
{
    auto result = origConsume(thiz, arg1, arg2, arg3, arg4, input_event);
    if(result != 0 || *input_event == nullptr) return result;
    ImGui_ImplAndroid_HandleInputEvent(*input_event);
    return result;
}

#include "functions.h"
#include "menu.h"

// --- MEMANGGIL VARIABEL HOOK DARI MAIN.CPP ---
extern Vector3 hk_GetPosition(void *instance);
extern Vector3 (*old_GetPosition)(void *instance);
extern void *hk_WorldToScreenPoint(void *camera, Vector3 position, int eye, Vector3 *output);
extern void *(*old_WorldToScreenPoint)(void *camera, Vector3 position, int eye, Vector3 *output);
extern void hk_RegisterCustomPlayer(void *instance, void *player_instance);
extern void (*old_RegisterCustomPlayer)(void *instance, void *player_instance);

// --- FUNGSI UTAMA UNTUK MENGUNCI SEGALA OFFSET ---
void *hack_thread(void *arg) {
    do {
        sleep(1);
        g_il2cppBaseMap = KittyMemory::getLibraryBaseMap("libil2cpp.so");
    } while (!g_il2cppBaseMap.isValid());
    
    uintptr_t il2cpp_base = g_il2cppBaseMap.startAddress;
    KITTY_LOGI("il2cpp base: %p", (void*)il2cpp_base);
    
    // Inisialisasi Pointer & Patches Bawaan
    Pointers();
    Hooks();
    
    // [EKSEKUSI UTAMA] Pasang 3 DobbyHook ESP Anda di sini!
    DobbyHook((void *)(il2cpp_base + 0x80F07A0), (void *)hk_GetPosition, (void **)&old_GetPosition);
    DobbyHook((void *)(il2cpp_base + 0x80BFB2C), (void *)hk_WorldToScreenPoint, (void **)&old_WorldToScreenPoint);
    DobbyHook((void *)(il2cpp_base + 0x7FF17F0), (void *)hk_RegisterCustomPlayer, (void **)&old_RegisterCustomPlayer);

    // Kunci Driver Grafik Unity untuk ImGui Menu
    auto eglhandle = dlopen("libunity.so", RTLD_LAZY);
    auto eglSwapBuffers = dlsym(eglhandle, "eglSwapBuffers");
    DobbyHook((void*)eglSwapBuffers, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    
    // Kunci Sistem Input Sentuh Layar HP
    void *sym_input = DobbySymbolResolver(("/system/lib/libinput.so"), ("_ZN7android13InputConsumer21initializeMotionEventEPNS_11MotionEventEPKNS_12InputMessageE"));
    if (NULL != sym_input) {
        DobbyHook(sym_input, (void*)myInput, (void**)&origInput);
    } else {
        sym_input = DobbySymbolResolver(("/system/lib/libinput.so"), ("_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE"));
        if(NULL != sym_input) {
            DobbyHook(sym_input, (void *)myConsume, (void **)&origConsume);
        }
    }
    
    LOGI("Draw Done!");
    return nullptr;
}
