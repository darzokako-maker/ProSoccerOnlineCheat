#include <iostream> 
#include <vector> 
#include <windows.h>
#include <TlHelp32.h>  
#include <cstdint>
#include <string>

// --- [ GÖRSELLERDEN ALINAN VE DÖNGÜYE UYARLANAN OFSETLER ] ---
namespace Offsets {
    constexpr uintptr_t MainBase1 = 0x0461CF28; // Hız ve Stamina Ana Adresi
    constexpr uintptr_t MainBase2 = 0x04611F80; // FOV ve GWorld Ana Adresi
    constexpr uintptr_t PositionBase = 0x041C9A00; // Kendimizi yükseltme adresi

    // Sabit Zincirler
    const std::vector<unsigned int> SpeedChain    = { 0x0, 0xA0, 0x558, 0x18, 0x118 };
    const std::vector<unsigned int> StaminaChain  = { 0x0, 0xA0, 0x578, 0xA0, 0x50, 0x6C8 };
    const std::vector<unsigned int> FovChain      = { 0x30, 0x260, 0x558, 0x1F8 };
    const std::vector<unsigned int> BallGlowChain = { 0x30, 0x50, 0x610 };

    // --- OYUNCU DÖNGÜSÜ İÇİN UNREAL ENGINE GENEL YAPISI ---
    // GWorld -> PersistentLevel -> OwningGameInstance -> LocalPlayers -> ...
    constexpr uintptr_t LevelOffset = 0x30;            // UWorld -> PersistentLevel
    constexpr uintptr_t ActorArrayOffset = 0x98;       // ULevel -> AActor Array (Oyuncu ve Objelerin Listesi)
    constexpr uintptr_t ActorCountOffset = 0xA0;       // ULevel -> AActor Count (Listedeki Eleman Sayısı)
    constexpr uintptr_t RootComponentOffset = 0x130;   // AActor -> RootComponent
    constexpr uintptr_t LocationOffset = 0x11C;        // USceneComponent -> RelativeLocation (Z ekseni genelde +8 veya +4 ofsetindedir)
}

// --- [ HAFIZA YÖNETİCİSİ ] ---
namespace Memory {
    HANDLE process_handle = nullptr;
    uintptr_t base_address = 0;

    DWORD GetProcessId(const wchar_t* process_name) {
        DWORD process_id = 0;
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32W);
            if (Process32FirstW(hSnap, &pe32)) {
                do {
                    if (_wcsicmp(pe32.szExeFile, process_name) == 0) {
                        process_id = pe32.th32ProcessID;
                        break;
                    }
                } while (Process32NextW(hSnap, &pe32));
            }
            CloseHandle(hSnap);
        }
        return process_id;
    }

    uintptr_t GetModuleBase(DWORD process_id, const wchar_t* module_name) {
        uintptr_t base_addr = 0;
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id);
        if (hSnap != INVALID_HANDLE_VALUE) {
            MODULEENTRY32W me32;
            me32.dwSize = sizeof(MODULEENTRY32W);
            if (Module32FirstW(hSnap, &me32)) {
                do {
                    if (_wcsicmp(me32.szModule, module_name) == 0) {
                        base_addr = (uintptr_t)me32.modBaseAddr;
                        break;
                    }
                } while (Module32NextW(hSnap, &me32));
            }
            CloseHandle(hSnap);
        }
        return base_addr;
    }

    uintptr_t ReadPointerChain(HANDLE hProc, uintptr_t base, const std::vector<unsigned int>& offsets) {
        uintptr_t addr = base;
        for (size_t i = 0; i < offsets.size(); ++i) {
            uintptr_t next_addr = 0;
            if (!ReadProcessMemory(hProc, (LPCVOID)addr, &next_addr, sizeof(uintptr_t), nullptr)) {
                return 0; 
            }
            addr = next_addr + offsets[i];
        }
        return addr;
    }
}

struct CheatStatus {
    bool fov = false;
    bool stamina = false;
    bool speed = false;
    bool ball_glow = false; 
    bool troll_players = false; // Senin dediğin mantıkla çalışan döngülü hile

    float val_fov = 115.0f;
    float val_speed = 3.0f; 
    float val_glow = 999999.0f; 
} status;

void MenuCiz() {
    system("cls");
    std::cout << "==================================================\n";
    std::cout << "     Ohiohook v5.5 - Dongulu Real-Troll Paneli    \n";
    std::cout << "     Yahya'nin Onayladigi Dogru Algoritma        \n";
    std::cout << "==================================================\n\n";

    auto Bas = [](std::string isim, bool durum, std::string tus, std::string ek = "") {
        std::cout << " [" << tus << "] " << isim << ": " 
                  << (durum ? "[ON]" : "[OFF]") 
                  << " " << ek << "\n";
    };

    Bas("FOV Gorus Acisi      ", status.fov, "1", "(Deger: " + std::to_string((int)status.val_fov) + ")");
    Bas("Sinirsiz Stamina    ", status.stamina, "2");
    Bas("Speed Hack (Hiz)    ", status.speed, "3", "(Deger: " + std::to_string(status.val_speed).substr(0,3) + ")");
    Bas("Topu Parlat (Glow)  ", status.ball_glow, "4");
    Bas("Oyunculari Havaya Uc", status.troll_players, "5", "*DONGULU GERCEK TROLL*");
    
    std::cout << "\n [L] Kendini Yukselt (Add Position)\n";
    std::cout << " [M] Ayarlari Canli Guncelle\n";
    std::cout << "==================================================\n";
    std::cout << "[*] Ozellikleri acip kapatmak icin sayilara basin...\n";
}

void DegerleriAl() {
    system("cls");
    std::cout << "[=== HILE DEGER YAPILANDIRMASI ===]\n\n";
    std::cout << "[>] FOV Degeri (Orn: 115): "; std::cin >> status.val_fov;
    std::cout << "[>] SPEED Hiz Kat sayisi (Orn: 3.5): "; std::cin >> status.val_speed;
    std::col << "\n[+] Veriler kaydedildi! Menuye donuluyor...";
    Sleep(1000);
}

int main() {
    SetConsoleTitleW(L"Pro Soccer Online External Hook v5.5");
    DegerleriAl();

    std::cout << "\n[*] Oyun bekleniyor... Lutfen Pro Soccer Online'i baslatin.\n";
    const wchar_t* oyunAdi = L"ProSoccerOnline-Win64-Shipping.exe";
    DWORD pID = 0;
    while (pID == 0) {
        pID = Memory::GetProcessId(oyunAdi);
        Sleep(500);
    }

    Memory::process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
    if (!Memory::process_handle) {
        std::cout << "[-] Hafiza kapisi acilamadi! Yonetici olarak calistirin.\n";
        system("pause");
        return 0;
    }

    Memory::base_address = Memory::GetModuleBase(pID, oyunAdi);
    MenuCiz();

    while (true) {
        bool girdiOldu = false;

        if (GetAsyncKeyState('1') & 0x1) { status.fov = !status.fov; girdiOldu = true; }
        if (GetAsyncKeyState('2') & 0x1) { status.stamina = !status.stamina; girdiOldu = true; }
        if (GetAsyncKeyState('3') & 0x1) { status.speed = !status.speed; girdiOldu = true; }
        if (GetAsyncKeyState('4') & 0x1) { status.ball_glow = !status.ball_glow; girdiOldu = true; }
        if (GetAsyncKeyState('5') & 0x1) { status.troll_players = !status.troll_players; girdiOldu = true; }
        if (GetAsyncKeyState('M') & 0x1) { DegerleriAl(); girdiOldu = true; }

        if (girdiOldu) MenuCiz();

        // --- TEKIL HILE YAZIMLARI ---
        if (status.fov) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::MainBase2, Offsets::FovChain);
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &status.val_fov, sizeof(float), nullptr);
        }
        if (status.stamina) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::MainBase1, Offsets::StaminaChain);
            float fullStamina = 1.0f; 
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &fullStamina, sizeof(float), nullptr);
        }
        if (status.speed) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::MainBase1, Offsets::SpeedChain);
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &status.val_speed, sizeof(float), nullptr);
        }
        if (status.ball_glow) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::MainBase2, Offsets::BallGlowChain);
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &status.val_glow, sizeof(float), nullptr);
        }

        // --- YAHYA'NIN ALGORİTMASI: DÖNGÜLÜ MASS TROLL (HATA VERMEDEN UÇURMA) ---
        if (status.troll_players) {
            // 1. GWorld adresini oku
            uintptr_t gWorld = 0;
            ReadProcessMemory(Memory::process_handle, (LPCVOID)(Memory::base_address + Offsets::MainBase2), &gWorld, sizeof(gWorld), nullptr);
            
            if (gWorld) {
                // 2. PersistentLevel adresine gir
                uintptr_t persistentLevel = 0;
                ReadProcessMemory(Memory::process_handle, (LPCVOID)(gWorld + Offsets::LevelOffset), &persistentLevel, sizeof(persistentLevel), nullptr);
                
                if (persistentLevel) {
                    // 3. Oyuncu/Aktör dizisinin başlangıcını ve listedeki eleman sayısını çek
                    uintptr_t actorArray = 0;
                    int actorCount = 0;
                    ReadProcessMemory(Memory::process_handle, (LPCVOID)(persistentLevel + Offsets::ActorArrayOffset), &actorArray, sizeof(actorArray), nullptr);
                    ReadProcessMemory(Memory::process_handle, (LPCVOID)(persistentLevel + Offsets::ActorCountOffset), &actorCount, sizeof(actorCount), nullptr);
                    
                    // Güvenlik sınırı (Oyunun çökmemesi için döngüyü maksimum 200 elemanla sınırlayalım)
                    if (actorCount > 200) actorCount = 200;

                    // 4. İŞTE O DÖNGÜ: Tüm oyuncuları tek tek tarıyoruz
                    for (int i = 0; i < actorCount; i++) {
                        uintptr_t currentActor = 0;
                        // Dizideki sıradaki aktörün pointer adresini oku (Her pointer 8 byte yer kaplar: i * 8)
                        ReadProcessMemory(Memory::process_handle, (LPCVOID)(actorArray + (i * 8)), &currentActor, sizeof(currentActor), nullptr);
                        
                        if (currentActor) {
                            // 5. Aktörün içindeki RootComponent (Konum merkezini) oku
                            uintptr_t rootComponent = 0;
                            ReadProcessMemory(Memory::process_handle, (LPCVOID)(currentActor + Offsets::RootComponentOffset), &rootComponent, sizeof(rootComponent), nullptr);
                            
                            if (rootComponent) {
                                // 6. Konumun tam Z eksenine (Yükseklik) ulaşıp 8500.0f değerini basıyoruz!
                                uintptr_t zLocationAddr = rootComponent + Offsets::LocationOffset + 0x8; // +0x8 Z eksenidir
                                float yeniYukseklik = 8500.0f;
                                WriteProcessMemory(Memory::process_handle, (LPVOID)zLocationAddr, &yeniYukseklik, sizeof(float), nullptr);
                            }
                        }
                    }
                }
            }
        }

        // Kendini yükseltme kısayolu (L tuşu)
        if (GetAsyncKeyState('L') & 0x8000) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::PositionBase, { 0x0, 0x120, 0x32C });
            if (addr) {
                float currentPos = 0.0f;
                ReadProcessMemory(Memory::process_handle, (LPCVOID)addr, &currentPos, sizeof(float), nullptr);
                currentPos += 2.0f; 
                WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &currentPos, sizeof(float), nullptr);
                Sleep(50); 
            }
        }

        Sleep(25); 
    }

    if (Memory::process_handle) CloseHandle(Memory::process_handle);
    return 0;
}
