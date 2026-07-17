#ifndef ZYCHEATS_SGUYS_FUNCTIONS_H
#define ZYCHEATS_SGUYS_FUNCTIONS_H

// --- LIBRARY UNTUK ESP/IMGUI ---
#include <vector>
#include <mutex>
#include "hook.h" // Memanggil macro HOOK, PATCH_SWITCH, dan g_il2cppBaseMap agar dikenali oleh compiler

// --- STRUKTUR DATA ---
// PERBAIKAN: struct Vector3 lama dihapus karena sudah menggunakan Include/Vector3.h dari template utama Anda

// --- DECLARATION EKSTERNAL (Integrasi ke ImGui) ---
// Variable & hook pointer yang akan di-eksekusi di dalam file .cpp ImGui
extern void* main_camera;
extern std::vector<void*> active_players;
extern std::mutex player_mutex;

// Menggunakan tipe data Vector3 bawaan template utama Anda
extern Vector3 old_GetPosition(void *instance);
extern void *old_WorldToScreenPoint(void *camera, Vector3 position, int eye, Vector3 *output);

// --- VARIABLES UNTUK PATCHES ---
// Menggunakan inline agar variabel tidak memicu redefinition error saat dipanggil berkali-kali oleh menu.h dan hook.cpp
inline bool addCurrency = false;
inline bool freeItems = false;
inline bool everythingUnlocked = false;
inline bool showAllItems = false;
inline bool addSkins = false;

// --- UTILS ---
// Mengonversi string hexadesimal secara aman tanpa perlu fungsi pembantu eksternal yang hilang
inline uintptr_t parseHexOffset(const char* hexStr) {
    return std::strtoul(hexStr, nullptr, 16);
}

inline monoString *CreateIl2cppString(const char *str) {
    monoString *(*String_CreateString)(void *instance, const char *str) = (monoString*(*)(void*, const char*)) (g_il2cppBaseMap.startAddress + parseHexOffset(OBFUSCATE("0x2596B20")));
    return String_CreateString(NULL, str);
}

// --- POINTERS & HOOKS ---
inline void (*PurchaseRealMoney) (void* instance, monoString* itemId, monoString* receipt, void* callback);

inline void Pointers() {
    PurchaseRealMoney = (void(*)(void*, monoString*, monoString*, void*)) (g_il2cppBaseMap.startAddress + parseHexOffset(OBFUSCATE("0xE7AADC")));
}

inline void Patches() {
    PATCH_SWITCH("0x10A69A0", "200080D2C0035FD6", showAllItems);
    PATCH_SWITCH("0xF148A4", "E07C80D2C0035FD6", freeItems);
}

// --- FUNCTIONS HOOKING ---
inline void (*old_Backend)(void *instance);
inline void Backend(void *instance) {
    if (instance != NULL) {
        if (addCurrency) {
            LOGW("Calling Purchase");
            PurchaseRealMoney(instance, CreateIl2cppString("special_offer1"), CreateIl2cppString("dev"), NULL);
            addCurrency = false;
        }
    }
    return old_Backend(instance);
}

inline void* (*old_ProductDefinition)(void *instance, monoString* id, monoString* storeSpecificId, int type, bool enabled, void* payouts);
inline void* ProductDefinition(void *instance, monoString* id, monoString* storeSpecificId, int type, bool enabled, void* payouts) {
    if (instance != NULL) {
        LOGW("Product: %s | ID: %s", id->getChars(), storeSpecificId->getChars());
    }
    return old_ProductDefinition(instance, id, storeSpecificId, type, enabled, payouts);
}

inline void Hooks() {
    HOOK("0xE7BC74", Backend, old_Backend);
    HOOK("0x29DA08C", ProductDefinition, old_ProductDefinition);
}

#endif //ZYCHEATS_SGUYS_FUNCTIONS_H
