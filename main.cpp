#include <iostream> 
#include <vector> 
#include <windows.h>
#include <TlHelp32.h>  
#include <cstdint>
#include <string>

// --- [ ADRES VE OFSET YAPILANDIRMASI (UE 5.4.4) ] ---
namespace Offsets {
    constexpr uintptr_t FovBase = 0x04611F80;
    constexpr uintptr_t StaminaBase = 0x0461CF28;
    constexpr uintptr_t PositionBase = 0x041C9A00;
}

// --- [ HAFIZA YÖNETİM SİSTEMİ ] ---
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
                } while (Module32NextW(hSnap, &me32)); // Hata veren kısım Module32NextW olarak düzeltildi
            }
            CloseHandle(hSnap);
        }
        return base_addr;
    }

    uintptr_t ReadPointerChain(HANDLE hProc, uintptr_t base, std::vector<unsigned int> offsets) {
        uintptr_t addr = base;
        for (unsigned int i = 0; i < offsets.size(); ++i) {
            if (!ReadProcessMemory(hProc, (LPCVOID)addr, &addr, sizeof(uintptr_t), nullptr)) return 0;
            addr += offsets[i];
        }
        return addr;
    }
}

// --- [ DURUM VE DEĞER DEĞİŞKENLERİ ] ---
struct CheatStatus {
    bool fov = false;
    bool stamina = false;
    bool speed = false;
    bool magic_hands = false;
    bool bug_ball = false;
    bool dribbling = false;
    bool drone = false;

    float val_fov = 110.0f;
    float val_speed = 2.5f;
    float val_kick = 5.0f;
    float val_dribbling = 3.0f;
    float val_drone = 25.0f;
} status;

// --- [ KONSOL ARAYÜZÜNÜ ÇİZME FONKSİYONU ] ---
void MenuCiz() {
    system("cls");
    std::cout << "==================================================\n";
    std::cout << "     Ohiohook v2.5 - Canli Yonetim Paneli        \n";
    std::cout << "     skyze x taskmanager | Durum Kontrolu         \n";
    std::cout << "==================================================\n\n";

    auto Bas = [](std::string isim, bool durum, std::string tus, std::string ek = "") {
        std::cout << " [" << tus << "] " << isim << ": " 
                  << (durum ? "[ON]" : "[OFF]") 
                  << " " << ek << "\n";
    };

    Bas("FOV Modifikasyonu", status.fov, "1", "(Deger: " + std::to_string((int)status.val_fov) + ")");
    Bas("Sinizsiz Stamina  ", status.stamina, "2");
    Bas("Speed Hack        ", status.speed, "3", "(Hiz: " + std::to_string(status.val_speed).substr(0,3) + ")");
    Bas("Magic Hands       ", status.magic_hands, "4", "(Guc: " + std::to_string(status.val_kick).substr(0,3) + ")");
    Bas("Bug Ball          ", status.bug_ball, "5");
    Bas("Dribbling Factor  ", status.dribbling, "6", "(Faktor: " + std::to_string(status.val_dribbling).substr(0,3) + ")");
    Bas("Drone Camera      ", status.drone, "7", "(Yukseklik: " + std::to_string(status.val_drone).substr(0,3) + ")");
    
    std::cout << "\n [L] Konumu Yukselt (Add Position)\n";
    std::cout << " [M] Degerleri Yeniden Ayarla / Guncelle\n";
    std::cout << "==================================================\n";
    std::cout << "[*] Acip kapatmak icin klavyeden numaralara basin...\n";
}

void DegerleriAl() {
    system("cls");
    std::cout << "[=== DEGER AYARLAMA MENUSU ===]\n\n";
    std::cout << "[>] Yeni FOV Degeri girin (Orn: 110): "; std::cin >> status.val_fov;
    std::cout << "[>] Yeni SPEED HACK Hiz Degeri (Orn: 2.5): "; std::cin >> status.val_speed;
    std::cout << "[>] Yeni MAGIC HANDS Vurus Gucu: "; std::cin >> status.val_kick;
    std::cout << "[>] Yeni DRIBBLING FACTOR Degeri: "; std::cin >> status.val_dribbling;
    std::cout << "[>] Yeni DRONE CAMERA Yukseklik Degeri: "; std::cin >> status.val_drone;
    std::cout << "\n[+] Degerler kaydedildi! Menuye donuluyor...";
    Sleep(1000);
}

int main() {
    SetConsoleTitleW(L"Pro Soccer Online Ohiohook Toggle Menu");
    
    DegerleriAl();

    std::cout << "\n[*] Oyun bekleniyor... Lutfen Pro Soccer Online baslatin.\n";
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
        if (GetAsyncKeyState('4') & 0x1) { status.magic_hands = !status.magic_hands; girdiOldu = true; }
        if (GetAsyncKeyState('5') & 0x1) { status.bug_ball = !status.bug_ball; girdiOldu = true; }
        if (GetAsyncKeyState('6') & 0x1) { status.dribbling = !status.dribbling; girdiOldu = true; }
        if (GetAsyncKeyState('7') & 0x1) { status.drone = !status.drone; girdiOldu = true; }
        
        if (GetAsyncKeyState('M') & 0x1) { 
            DegerleriAl(); 
            girdiOldu = true; 
        }

        if (girdiOldu) {
            MenuCiz();
        }
        
        if (status.fov) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::FovBase, { 0x30, 0x260, 0x558, 0x1F8 });
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &status.val_fov, sizeof(float), nullptr);
        }

        if (status.stamina) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::StaminaBase, { 0x0, 0xA0, 0x578, 0xA0, 0x50, 0x6C8 });
            float fullStamina = 1.0f;
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &fullStamina, sizeof(float), nullptr);
        }

        if (status.speed) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::FovBase, { 0x30, 0x5C8, 0xF8, 0x20, 0x98 });
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &status.val_speed, sizeof(float), nullptr);
        }

        if (status.magic_hands) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::FovBase, { 0x30, 0x50, 0x610 });
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &status.val_kick, sizeof(float), nullptr);
        }

        if (status.bug_ball) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::FovBase, { 0x30, 0x50, 0x610 });
            float bugValue = 999999.0f;
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &bugValue, sizeof(float), nullptr);
        }

        if (status.dribbling) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::FovBase, { 0x30, 0x130, 0x20, 0x98 });
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &status.val_dribbling, sizeof(float), nullptr);
        }

        if (GetAsyncKeyState('L') & 0x8000) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::PositionBase, { 0x0, 0x120, 0x32C });
            if (addr) {
                float currentPos = 0.0f;
                ReadProcessMemory(Memory::process_handle, (LPCVOID)addr, &currentPos, sizeof(float), nullptr);
                currentPos += 1.5f; 
                WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &currentPos, sizeof(float), nullptr);
                Sleep(100); 
            }
        }

        if (status.drone) {
            uintptr_t addr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::StaminaBase, { 0x0, 0xA0, 0x558, 0x11C });
            if (addr) WriteProcessMemory(Memory::process_handle, (LPVOID)addr, &status.val_drone, sizeof(float), nullptr);
        }

        Sleep(20); 
    }

    if (Memory::process_handle) CloseHandle(Memory::process_handle);
    return 0;
}
