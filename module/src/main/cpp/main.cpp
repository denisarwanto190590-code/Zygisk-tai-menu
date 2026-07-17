#include <cstring>
#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <mutex>
#include "hook.h"
#include "zygisk.hpp"
#include "dobby.h" // Pastikan library dobby terhubung

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

// --- STRUKTUR DATA GAME ---
struct Vector3 {
    float x, y, z;
};

// --- WADAH GLOBAL (Akan dibagikan ke menu ImGui nanti) ---
void* main_camera = nullptr;
std::vector<void*> active_players;
std::mutex player_mutex;
bool enable_hack = false;

// --- 1. HOOK POSISI (get_Position) ---
Vector3 (*old_GetPosition)(void *instance);
Vector3 hk_GetPosition(void *instance) {
    return old_GetPosition(instance);
}

// --- 2. HOOK KAMERA (WorldToScreenPoint) ---
void *(*old_WorldToScreenPoint)(void *camera, Vector3 position, int eye, Vector3 *output);
void *hk_WorldToScreenPoint(void *camera, Vector3 position, int eye, Vector3 *output) {
    if (camera != nullptr) {
        main_camera = camera;
    }
    return old_WorldToScreenPoint(camera, position, eye, output);
}

// --- 3. HOOK PENANGKAP PEMAIN (RegisterCustomPlayer) ---
void (*old_RegisterCustomPlayer)(void *instance, void *player_instance);
void hk_RegisterCustomPlayer(void *instance, void *player_instance) {
    if (player_instance != nullptr) {
        std::lock_guard<std::mutex> lock(player_mutex);
        
        bool already_exists = false;
        for (void* p : active_players) {
            if (p == player_instance) {
                already_exists = true;
                break;
            }
        }
        if (!already_exists) {
            active_players.push_back(player_instance);
        }
    }
    old_RegisterCustomPlayer(instance, player_instance);
}

// --- MAIN HACK THREAD (Eksekusi Penguncian Offset) ---
void *hack_thread(void *) {
    uintptr_t il2cpp_base = 0;
    
    // Tunggu sampai library game termuat penuh
    while (!il2cpp_base) {
        il2cpp_base = get_module_base("libil2cpp.so"); // Ambil base address
        sleep(1);
    }

    // PASANG OFFSET NYATA ANDA DI SINI BOS!
    DobbyHook((void *)(il2cpp_base + 0x80F07A0), (void *)hk_GetPosition, (void **)&old_GetPosition);
    DobbyHook((void *)(il2cpp_base + 0x80BFB2C), (void *)hk_WorldToScreenPoint, (void **)&old_WorldToScreenPoint);
    DobbyHook((void *)(il2cpp_base + 0x7FF17F0), (void *)hk_RegisterCustomPlayer, (void **)&old_RegisterCustomPlayer);

    return nullptr;
}

// --- STRUKTUR MODUL UTAMA ZYGISK ANDA ---
class MyModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        env_ = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        if (!args || !args->nice_name) {
            return;
        }
        enable_hack = isGame(env_, args->app_data_dir); 
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (enable_hack) {
            int ret;
            pthread_t ntid;
            if ((ret = pthread_create(&ntid, nullptr, hack_thread, nullptr))) {
                // Gagal membuat thread
            }
        }
    }

private:
    JNIEnv *env_{};
};

REGISTER_ZYGISK_MODULE(MyModule)
