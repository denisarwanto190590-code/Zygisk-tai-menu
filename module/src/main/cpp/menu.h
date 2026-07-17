//
// Created by lbert on 4/15/2023.
//

#ifndef ZYGISK_MENU_TEMPLATE_MENU_H
#define ZYGISK_MENU_TEMPLATE_MENU_H

using namespace ImGui;

// --- CHECKBOX UNTUK TOMBOL ON/OFF ESP ---
bool enable_esp_line = false;

// --- LOGIKA UTAMA GAMBAR ESP LINE ---
void DrawESP()
{
    // Jika menu ESP dimatikan atau data kamera belum tertangkap, lewati fungsi ini
    if (!enable_esp_line || main_camera == nullptr) return;

    // Ambil ukuran resolusi layar HP secara dinamis
    float screen_width = GetIO().DisplaySize.x;
    float screen_height = GetIO().DisplaySize.y;

    // Kunci wadah pemain agar aman dari tabrakan data (mencegah game crash)
    std::lock_guard<std::mutex> lock(player_mutex);

    for (void* player : active_players) {
        if (player == nullptr) continue;

        // [FILTER 1 - SS PERTAMA ANDA] Cek apakah ini diri kita sendiri (Local Player)
        // Kita intip memori pada alamat instance pemain + offset field 0xF8
        bool is_local = *(bool*)((uintptr_t)player + 0xF8);

        // Jika bernilai true (1), berarti ini karakter Anda sendiri. Lompati/Jangan digambar!
        if (is_local) continue;

        // Ambil koordinat 3D musuh menggunakan fungsi asli get_Position yang sudah kita ikat
        Vector3 enemyWorldPos = old_GetPosition(player);

        // Wadah kosong untuk menampung hasil konversi koordinat ke layar 2D HP
        Vector3 enemyScreenPos = {0, 0, 0};

        // Lempar koordinat 3D tadi ke fungsi kamera game (UGCAPIWorldToScreenPoint)
        old_WorldToScreenPoint(main_camera, enemyWorldPos, 2, &enemyScreenPos);

        // Cek jika musuh berada di depan sudut pandang kamera (Z > 0)
        if (enemyScreenPos.z > 0.0f) {
            
            // Tentukan titik awal garis: Tepat di tengah bawah layar HP Anda
            ImVec2 line_start = ImVec2(screen_width / 2.0f, screen_height);

            // Tentukan titik akhir garis: Tepat di titik koordinat hasil konversi kamera tadi
            ImVec2 line_end = ImVec2(enemyScreenPos.x, enemyScreenPos.y);

            // EKSEKUSI GAMBAR GARIS ESP KE LAYAR HP!
            GetForegroundDrawList()->AddLine(
                line_start, 
                line_end, 
                IM_COL32(255, 0, 0, 255), // Warna Merah Terang (RGBA)
                1.5f                      // Ketebalan garis (1.5 pixel)
            );
        }
    }
}

void DrawMenu()
{
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    {
        Begin(OBFUSCATE("ZyCheats"));
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyResizeDown;
        if (BeginTabBar("Menu", tab_bar_flags)) {
            
            // TAB ESP MENU BARU
            if (BeginTabItem(OBFUSCATE("Visual / ESP"))) {
                Checkbox(OBFUSCATE("Aktifkan ESP Line"), &enable_esp_line);
                EndTabItem();
            }

            if (BeginTabItem(OBFUSCATE("Account"))) {
                if (Button(OBFUSCATE("Add Currency"))) {
                    addCurrency = true;
                }
                TextUnformatted(OBFUSCATE("Adds 1000 gems"));
                if (Button(OBFUSCATE("Add Skins"))) {
                    addSkins = true;
                }
                Checkbox(OBFUSCATE("Everything unlocked"), &everythingUnlocked);
                Checkbox(OBFUSCATE("Free Items"), &freeItems);
                Checkbox(OBFUSCATE("Show Items"), &showAllItems);
                EndTabItem();
            }
            EndTabBar();
        }
        Patches();
        End();
    }
}

void SetupImgui() {
    IMGUI_CHECKVERSION();
    CreateContext();
    ImGuiIO &io = GetIO();
    io.DisplaySize = ImVec2((float) glWidth, (float) glHeight);
    ImGui_ImplOpenGL3_Init("#version 100");
    StyleColorsDark();
    GetStyle().ScaleAllSizes(7.0f);
    io.Fonts->AddFontFromMemoryTTF(Roboto_Regular, 30, 30.0f);
}

EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);

    if (!setupimg)
    {
        SetupImgui();
        setupimg = true;
    }

    ImGuiIO &io = GetIO();
    ImGui_ImplOpenGL3_NewFrame();
    NewFrame();

    // 1. Eksekusi fungsi gambar Garis ESP di latar belakang layar
    DrawESP();

    // 2. Munculkan Mod Menu ZyCheats di atasnya
    DrawMenu();

    EndFrame();
    Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

#endif //ZYGISK_MENU_TEMPLATE_MENU_H
