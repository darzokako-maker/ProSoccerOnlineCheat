#include <iostream> 
#include <vector> 
#include <windows.h>
#include <TlHelp32.h>  
#include <cstdint>

// --- [ ADRES VE OFSET YAPILANDIRMASI (UE 5.4.4) ] ---
namespace Offsets {
    constexpr uintptr_t GWorld = 0x7E93198;
    constexpr uintptr_t ULevel_Actors = 0xA0;
    
    // Eski çoklu pointer zincirleri yerine doğrudan Dumper-7 yapısı veya
    // Oyunun stabil alt-bellek haritasına yönelik test ofsetleri kullanılacaktır.
    // Orijinal projedeki statik örnek ofset yapısı korunmuştur.
    constexpr uintptr_t FovBase = 0x04611F80;
    constexpr uintptr_t StaminaBase = 0x0461CF28;
    constexpr uintptr_t PositionBase = 0x041C9A00;
}

// --- [ HAFIZA YÖNETİM SİSTEMİ ] ---
namespace Memory {
    HANDLE process_handle = nullptr;
    uintptr_t base_address = 0;

    // Süreç ID'sini bulan fonksiyon (Unicode / Geniş Karakter Destekli)
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

    // Oyunun ana modül adresini çeken fonksiyon
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

    // Pointer zincirlerini güvenli bir şekilde çözen şablon
    uintptr_t ReadPointerChain(HANDLE hProc, uintptr_t base, std::vector<unsigned int> offsets) {
        uintptr_t addr = base;
        for (unsigned int i = 0; i < offsets.size(); ++i) {
            ReadProcessMemory(hProc, (LPCVOID)addr, &addr, sizeof(uintptr_t), nullptr);
            addr += offsets[i];
        }
        return addr;
    }
}

int main() {
    // Görsel ASCII Karşılama Ekranı
    SetConsoleTitleW(L"Pro Soccer Online Ohiohook External v2.0");
    system("Color 0E");
    std::cout << "  OOO   H   H  IIIII  OOO  \n"
                 " O   O  H   H    I   O   O \n"
                 " O   O  HHHHH    I   O   O \n"
                 " O   O  H   H    I   O   O \n"
                 "  OOO   H   H  IIIII  OOO  \n\n";
    Sleep(800);
    system("cls");

    std::cout << "====================================\n";
    std::cout << "       ohiohook - External Source   \n";
    std::cout << "       skyze x taskmanager          \n";
    std::cout << "====================================\n\n";

    // Girdi Değerlerini Alma Alanı
    float IstedigimizFov, IstedigimizSpeed, IstedigimizKickPower, IstedigimizDribbling, IstedigimizDrone;
    
    std::cout << "[>] FOV Degeri girin (Orn: 110): "; std::cin >> IstedigimizFov;
    std::cout << "[>] SPEED HACK Degeri girin (Orn: 2.5): "; std::cin >> IstedigimizSpeed;
    std::cout << "[>] MAGIC HANDS Degeri girin: "; std::cin >> IstedigimizKickPower;
    std::cout << "[>] DRIBBLING FACTOR Degeri girin: "; std::cin >> IstedigimizDribbling;
    std::cout << "[>] DRONE CAMERA Degeri girin: "; std::cin >> IstedigimizDrone;

    std::cout << "\n[*] Oyun bekleniyor... Lutfen Pro Soccer Online baslatin.\n";

    // Sürece tek seferlik ve kalıcı bağlanma döngüsü
    const wchar_t* oyunAdi = L"ProSoccerOnline-Win64-Shipping.exe";
    DWORD pID = 0;
    while (pID == 0) {
        pID = Memory::GetProcessId(oyunAdi);
        Sleep(500);
    }

    Memory::process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
    if (!Memory::process_handle) {
        std::cout << "[-] Oyuna baglanilamadi! Yonetici olarak calistirmayi deneyin.\n";
        system("pause");
        return 0;
    }

    Memory::base_address = Memory::GetModuleBase(pID, oyunAdi);
    std::cout << "[+] Baglanti Basarili! Base Adresi: 0x" << std::hex << Memory::base_address << "\n\n";
    
    std::cout << "[=== KISAYOL TUSLARI AKTIF ===]\n";
    std::cout << "[R] -> FOV'u Uygula\n";
    std::cout << "[Q] -> SINIRSIZ STAMINA\n";
    std::cout << "[H] -> SPEED HACK Aktif Et\n";
    std::cout << "[F] -> MAGIC HANDS Kilitle\n";
    std::cout << "[Z] -> BUG BALL\n";
    std::cout << "[V] -> DRIBBLING YUKLE\n";
    std::cout << "[L] -> KONUMU YUKSELT (ADD POSITION)\n";
    std::cout << "[X] -> DRONE kamerasina gec\n\n";

    // Ana Hile Döngüsü (Performans optimizasyonu için Sleep eklendi)
    while (true) {
        // FOV (R Tuşu)
        if (GetAsyncKeyState('R') & 0x8000) {
            uintptr_t targetAddr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::FovBase, { 0x30, 0x260, 0x558, 0x1F8 });
            WriteProcessMemory(Memory::process_handle, (LPVOID)targetAddr, &IstedigimizFov, sizeof(float), nullptr);
        }

        // STAMINA (Q Tuşu)
        if (GetAsyncKeyState('Q') & 0x8000) {
            uintptr_t targetAddr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::StaminaBase, { 0x0, 0xA0, 0x578, 0xA0, 0x50, 0x6C8 });
            float fullStamina = 1.0f;
            WriteProcessMemory(Memory::process_handle, (LPVOID)targetAddr, &fullStamina, sizeof(float), nullptr);
        }

        // SPEEDHACK (H Tuşu)
        if (GetAsyncKeyState('H') & 0x8000) {
            uintptr_t targetAddr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::FovBase, { 0x30, 0x5C8, 0xF8, 0x20, 0x98 });
            WriteProcessMemory(Memory::process_handle, (LPVOID)targetAddr, &IstedigimizSpeed, sizeof(float), nullptr);
        }

        // MAGIC HANDS (F Tuşu)
        if (GetAsyncKeyState('F') & 0x8000) {
            uintptr_t targetAddr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::FovBase, { 0x30, 0x50, 0x610 });
            WriteProcessMemory(Memory::process_handle, (LPVOID)targetAddr, &IstedigimizKickPower, sizeof(float), nullptr);
        }

        // BUG BALL (Z Tuşu)
        if (GetAsyncKeyState('Z') & 0x8000) {
            uintptr_t targetAddr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::FovBase, { 0x30, 0x50, 0x610 });
            float bugValue = 999999.0f;
            WriteProcessMemory(Memory::process_handle, (LPVOID)targetAddr, &bugValue, sizeof(float), nullptr);
        }

        // DRIBBLING FACTOR (V Tuşu)
        if (GetAsyncKeyState('V') & 0x8000) {
            uintptr_t targetAddr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::FovBase, { 0x30, 0x130, 0x20, 0x98 });
            WriteProcessMemory(Memory::process_handle, (LPVOID)targetAddr, &IstedigimizDribbling, sizeof(float), nullptr);
        }

        // ADD POSITION (L Tuşu)
        if (GetAsyncKeyState('L') & 0x8000) {
            uintptr_t targetAddr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::PositionBase, { 0x0, 0x120, 0x32C });
            float currentPos = 0.0f;
            ReadProcessMemory(Memory::process_handle, (LPCVOID)targetAddr, &currentPos, sizeof(float), nullptr);
            currentPos += 1.0f;
            WriteProcessMemory(Memory::process_handle, (LPVOID)targetAddr, &currentPos, sizeof(float), nullptr);
        }

        // DRONE CAMERA (X Tuşu)
        if (GetAsyncKeyState('X') & 0x8000) {
            uintptr_t targetAddr = Memory::ReadPointerChain(Memory::process_handle, Memory::base_address + Offsets::StaminaBase, { 0x0, 0xA0, 0x558, 0x11C });
            WriteProcessMemory(Memory::process_handle, (LPVOID)targetAddr, &IstedigimizDrone, sizeof(float), nullptr);
        }

        Sleep(30); // CPU'yu %100 sömürmemesi ve stabil tarama yapması için hayati gecikme
    }

    if (Memory::process_handle) CloseHandle(Memory::process_handle);
    return 0;
}

