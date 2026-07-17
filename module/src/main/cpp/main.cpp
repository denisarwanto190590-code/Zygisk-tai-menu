#include <cstring>
#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <mutex>
#include "hook.h"
#include "zygisk.hpp"
#include "Includes/Dobby/dobby.h" // PERBAIKAN: Jalur library Dobby disesuaikan dengan folder Anda

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

// --- STRUKTUR DATA GAME ---
// PERBAIKAN: Dihapus/dikomentari karena sudah dideklarasikan di file Include/Vector3.h bawaan template Anda
/*
struct Vector3 {
    float x, y, z;
};
*/

// --- WADAH GLOBAL (Akan dibagikan ke menu ImGui nanti) ---
void* main_camera = nullptr;
std::vector<void*> active_players;
std::mutex player_mutex;

// PERBAIKAN: Variabel enable_hack dihapus dari sini karena tipenya bentrok (bool vs int) dengan extern yang ada di hook.h/hook.cpp

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
// Catatan: Fungsi hack_thread asli dipindahkan eksekusinya ke hook.cpp agar ImGui dan DobbyHook berjalan sinkron.

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
        // Menggunakan variabel global int enable_hack yang ditarik dari hook.h/hook.cpp
        extern int enable_hack;
        enable_hack = isGame(env_, args->app_data_dir); 
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        extern int enable_hack;
        extern void *hack_thread(void *arg); // Memanggil thread utama yang ada di hook.cpp
        
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
