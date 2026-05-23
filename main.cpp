#include <iostream> 
#include <vector> 
#include <windows.h>
#include <TlHelp32.h>  
#include <cstdint>
#include <string>

// --- [ GÜNCEL VE KESİN OFSET YAPISI (64-BIT UYUMLU) ] ---
namespace Offsets {
    // NOT: Oyun güncellendiğinde Cheat Engine ile sadece bu 3 ana adresi (Base) yenilemen yeterlidir.
    uintptr_t MainBase1 = 0x0461CF28; // Hız ve Stamina Ana Adresi
    uintptr_t MainBase2 = 0x04611F80; // FOV, Top ve Oyuncu Verileri Ana Adresi
    uintptr_t PositionBase = 0x041C9A00; // Konum Değiştirme Taban Adresi

    // Tam Pointer Zincirleri (Unreal Engine 64-bit Yapısı)
    const std::vector<unsigned int> SpeedChain    = { 0x0, 0xA0, 0x558, 0x18, 0x118 };
    const std::vector<unsigned int> StaminaChain  = { 0x0, 0xA0, 0x578, 0xA0, 0x50, 0x6C8 };
    const std::vector<unsigned int> FovChain      = { 0x30, 0x260, 0x558, 0x1F8 };
    const std::vector<unsigned int> BallGlowChain = { 0x30, 0x50, 0x610 }; 
    const std::vector<unsigned int> LocalPlayerZ  = { 0x0, 0x120, 0x32C }; // Kendi Z koordinatın
}

// --- [ GÜVENLİ BELLEK YÖNETİCİSİ ] ---
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

    // 64-Bit Kesin Okuma Yapan Pointer Zincir Çözücü
    uintptr_t ReadPointerChain(HANDLE hProc, uintptr_t base, const std::vector<unsigned int>& offsets) {
        uintptr_t addr = base;
        for (size_t i = 0; i < offsets.size(); ++i) {
            uintptr_t next_addr = 0;
            // 64-bit mimaride pointer'lar her zaman 8 byte (sizeof(uintptr_t)) olarak okunmalıdır
            if (!ReadProcessMemory(hProc, (LPCVOID)addr, &next_addr, sizeof(uintptr_t), nullptr)) {
                return 0; 
            }
            addr = next_addr + offsets[i];
        }
        return addr;
    }
}

// --- [ HİLE PANELİ DURUM KONTROLÜ ] ---
struct CheatStatus {
    bool fov = false;
    bool stamina = false;
    bool speed = false;
    bool ball_glow = false; 
    bool fly_up = false; // Güvenli uçma/yükselme modu

    float val_fov = 115.0f;
    float val_speed = 3.0f; 
    float val_glow = 9999.0f; 
} status;

void MenuCiz() {
    system("cls");
    std::cout << "==================================================\n";
    std::cout << "     Ohiohook v5.5 - VIP Canli Yonetim Paneli     \n";
    std::cout << "          64-Bit Stabilized Architecture          \n";
    std::cout << "==================================================\n\n";

    auto Bas = [](std::string isim, bool durum, std::string tus, std::string ek = "") {
        std::cout << " [" << tus << "] " << isim << ": " 
                  << (durum ? "[\x1B[32mON\x1B[0m]" : "[\x1B[31mOFF\x1B[0m]") 
                  << " " << ek << "\n";
    };

    Bas("FOV Gorus Acisi      ", status.fov, "1", "(Deger: " + std::to_string((int)status.val_fov) + ")");
    Bas("Sinirsiz Stamina    ", status.stamina, "2");
    Bas("Speed Hack (Hiz)    ", status.speed, "3", "(Deger: " + std::to_string(status.val_speed).substr(0,3) + ")");
    Bas("Topu Parlat (Glow)  ", status.ball_glow, "4");
    Bas("Oto Yukari Ucma     ", status.fly_up, "5", "(Yorunge Modu)");
    
    std::cout << "\n [L] Kendini Yukselt (Anlik Işınlanma)\n";
    std::cout << " [M] Ayarlari Canli Guncelle\n";
    std::cout << "==================================================\n";
    std::cout << "[*] Seçim yapmak için klavyedeki sayılara basın...\n";
}

void DegerleriAl() {
    system("cls");
    std::cout << "[=== HILE DEGER YAPILANDIRMASI ===]\n\n";
    std::cout << "[>] FOV Degeri (Orn: 115): "; std::cin >> status.val_fov;
    std::cout << "[>] SPEED Hiz Kat sayisi (Orn: 3.5): "; std::cin >> status.val_speed;
    std::cout << "\n[+] Veriler esitlendi! Menuye donuluyor...";
    Sleep(800);
}

int main() {
    SetConsoleTitleW(L"Pro Soccer Online External Hook v5.5");
    
    std::cout << "[*] Oyun bekleniyor... Lutfen Pro Soccer Online'i baslatin.\n";
    const wchar_t* oyunAdi = L"ProSoccerOnline-Win64-Shipping.exe";
    
    DWORD pID = 0;
    while (pID == 0) {
        pID = Memory::GetProcessId(oyunAdi);
        Sleep(500);
    }

    // Belleğe tam yetkiyle bağlanmayı dene
    Memory::process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
    if (!Memory::process_handle) {
        std::cout << "\n[-] Hafiza kapisi acilamadi!\n";
        std::cout << "[!] SEBEP: Anti-cheat (EAC) aktif veya programı 'Yonetici Olarak' calistirmadiniz.\n";
        system("pause");
        return 0;
    }

    Memory::base_address = Memory::GetModuleBase(pID, oyunAdi);
    if (Memory::base_address == 0) {
        std::cout << "[-] Modul taban adresi alinamadi!\n";
        CloseHandle(Memory::process_handle);
        system("pause");
        return 0;
    }

    DegerleriAl();
    MenuCiz();

    // Ana Döngü
    while (true) {
        bool girdiOldu = false;

        if (GetAsyncKeyState('1') & 0x1) { status.fov = !status.fov; girdiOldu = true; }
        if (GetAsyncKeyState('2') & 0x1) { status.stamina = !status.stamina; girdiOldu = true; }
        if (GetAsyncKeyState('3') & 0x1) { status.speed = !status.speed; girdiOldu = true; }
        if (GetAsyncKeyState('4') & 0x1) { status.ball_glow = !status.ball_glow; girdiOldu = true; }
        if (GetAsyncKeyState('5') & 0x1) { status.fly_up = !status.fly_up; girdiOldu = true; }
        
        if (GetAsyncKeyState('M') & 0x1) { 
            DegerleriAl(); 
            girdiOldu = true; 
        }

        if (girdiOldu) {
            MenuCiz();
        }

        // --- CANLI BELLEK YAZMA İŞLEMLERİ ---

        // 1. FOV Hilesi
        if (status.fov) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::MainBase2, Offsets::FovChain);
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &status.val_fov, sizeof(float), nullptr);
        }

        // 2. Sınırsız Stamina Hilesi
        if (status.stamina) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::MainBase1, Offsets::StaminaChain);
            float fullStamina = 1.0f; 
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &fullStamina, sizeof(float), nullptr);
        }

        // 3. Hız Hilesi
        if (status.speed) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::MainBase1, Offsets::SpeedChain);
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &status.val_speed, sizeof(float), nullptr);
        }

        // 4. Top Parlatma Hilesi
        if (status.ball_glow) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::MainBase2, Offsets::BallGlowChain);
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &status.val_glow, sizeof(float), nullptr);
        }

        // 5. Sürekli Yukarı Doğru Uçma (Z eksenini sürekli arttırarak havada tutar)
        if (status.fly_up) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::PositionBase, Offsets::LocalPlayerZ);
            if (addr) {
                float currentPos = 0.0f;
                ReadProcessMemory(Memory::process_handle, (LPCVOID)addr, &currentPos, sizeof(float), nullptr);
                currentPos += 1.5f; 
                WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &currentPos, sizeof(float), nullptr);
            }
        }

        // Kısayol: L Tuşuna Basılı Tutulduğunda Kendini Yukarı Fırlatma
        if (GetAsyncKeyState('L') & 0x8000) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::PositionBase, Offsets::LocalPlayerZ);
            if (addr) {
                float currentPos = 0.0f;
                ReadProcessMemory(Memory::process_handle, (LPCVOID)addr, &currentPos, sizeof(float), nullptr);
                currentPos += 8.0f; 
                WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &currentPos, sizeof(float), nullptr);
                Sleep(20); 
            }
        }

        Sleep(20); // CPU Stabilizasyonu
    }

    if (Memory::process_handle) CloseHandle(Memory::process_handle);
    return 0;
}
