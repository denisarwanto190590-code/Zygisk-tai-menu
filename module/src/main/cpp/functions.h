#ifndef ZYCHEATS_SGUYS_FUNCTIONS_H
#define ZYCHEATS_SGUYS_FUNCTIONS_H

// --- LIBRARY UNTUK ESP/IMGUI ---
#include <vector>
#include <mutex>

// --- STRUKTUR DATA (Wajib untuk koordinat) ---
struct Vector3 {
    float x, y, z;
};

// --- DECLARATION EKSTERNAL (Integrasi ke ImGui) ---
// Variable & hook pointer yang akan di-eksekusi di dalam file .cpp ImGui
extern void* main_camera;
extern std::vector<void*> active_players;
extern std::mutex player_mutex;
extern Vector3 (*old_GetPosition)(void *instance);
extern void *(*old_WorldToScreenPoint)(void *camera, Vector3 position, int eye, Vector3 *output);

// --- VARIABLES UNTUK PATCHES ---
bool addCurrency = false, freeItems = false, everythingUnlocked = false, showAllItems = false, addSkins = false;

// --- UTILS ---
monoString *CreateIl2cppString(const char *str) {
    monoString *(*String_CreateString)(void *instance, const char *str) = (monoString*(*)(void*, const char*)) (g_il2cppBaseMap.startAddress + string2Offset(OBFUSCATE("0x2596B20")));
    return String_CreateString(NULL, str);
}

// --- POINTERS & HOOKS ---
void (*PurchaseRealMoney) (void* instance, monoString* itemId, monoString* receipt, void* callback);

void Pointers() {
    PurchaseRealMoney = (void(*)(void*, monoString*, monoString*, void*)) (g_il2cppBaseMap.startAddress + string2Offset(OBFUSCATE("0xE7AADC")));
}

void Patches() {
    PATCH_SWITCH("0x10A69A0", "200080D2C0035FD6", showAllItems);
    PATCH_SWITCH("0xF148A4", "E07C80D2C0035FD6", freeItems);
}

// --- FUNCTIONS HOOKING ---
void (*old_Backend)(void *instance);
void Backend(void *instance) {
    if (instance != NULL) {
        if (addCurrency) {
            LOGW("Calling Purchase");
            PurchaseRealMoney(instance, CreateIl2cppString("special_offer1"), CreateIl2cppString("dev"), NULL);
            addCurrency = false;
        }
    }
    return old_Backend(instance);
}

void* (*old_ProductDefinition)(void *instance, monoString* id, monoString* storeSpecificId, int type, bool enabled, void* payouts);
void* ProductDefinition(void *instance, monoString* id, monoString* storeSpecificId, int type, bool enabled, void* payouts) {
    if (instance != NULL) {
        LOGW("Product: %s | ID: %s", id->getChars(), storeSpecificId->getChars());
    }
    return old_ProductDefinition(instance, id, storeSpecificId, type, enabled, payouts);
}

void Hooks() {
    HOOK("0xE7BC74", Backend, old_Backend);
    HOOK("0x29DA08C", ProductDefinition, old_ProductDefinition);
}

#endif //ZYCHEATS_SGUYS_FUNCTIONS_H
