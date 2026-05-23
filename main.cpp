#include "Header.h"

// --- [ ALT SEVİYE WINDOWS YAPILARI - MANUEL TANIMLAMALAR ] ---
typedef LONG NTSTATUS;
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PVOID PebBaseAddress; 
    ULONG_PTR AffinityMask;
    LONG BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION;

typedef NTSTATUS(NTAPI* pNtQueryInformationProcess)(
    HANDLE ProcessHandle,
    ULONG ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
);

// --- [ SÜRECE BAĞLANMA FONKSİYONU ] ---
bool attach_process(const wchar_t* process_name)
{
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    if (!Process32FirstW(snapshot, &pe32)) {
        CloseHandle(snapshot);
        return false;
    }

    do {
        if (_wcsicmp(pe32.szExeFile, process_name) == 0) {
            variables::process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
            if (variables::process_handle != nullptr) {
                variables::base_address = get_module_base(process_name);
                CloseHandle(snapshot);
                return true;
            }
        }
    } while (Process32NextW(snapshot, &pe32));

    CloseHandle(snapshot);
    return false;
}

// --- [ MODÜL TABAN ADRESİNİ BULMA FONKSİYONU ] ---
uintptr_t get_module_base(const wchar_t* module_name)
{
    MODULEENTRY32W me32;
    me32.dwSize = sizeof(MODULEENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetProcessId(variables::process_handle));
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    if (!Module32FirstW(snapshot, &me32)) {
        CloseHandle(snapshot);
        return 0;
    }

    do {
        if (_wcsicmp(me32.szModule, module_name) == 0) {
            CloseHandle(snapshot);
            return (uintptr_t)me32.modBaseAddr;
        }
    } while (Module32NextW(snapshot, &me32));

    CloseHandle(snapshot);
    return 0;
}

// --- [ ANA DÖNGÜ (MAIN) ] ---
int main()
{
    SetConsoleTitleW(L"Pro Soccer Online External Cheat");

    std::wcout << L"[*] Oyun bekleniyor...\n";

    while (!attach_process(L"ProSoccerOnline-Win64-Shipping.exe")) {
        Sleep(500);
    }

    std::wcout << L"[+] Oyuna basariyla baglanildi!\n";
    std::wcout << L"[+] Base Adresi: 0x" << std::hex << variables::base_address << L"\n";

    // PEB bulma adımları
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        pNtQueryInformationProcess NtQueryInformationProcess = 
            (pNtQueryInformationProcess)GetProcAddress(hNtdll, "NtQueryInformationProcess");

        if (NtQueryInformationProcess) {
            PROCESS_BASIC_INFORMATION pbi;
            ULONG returnLength = 0;
            
            NTSTATUS status = NtQueryInformationProcess(variables::process_handle, 0, &pbi, sizeof(pbi), &returnLength);
            if (NT_SUCCESS(status)) {
                std::wcout << L"[+] Oyunun PEB Adresi: 0x" << std::hex << (uintptr_t)pbi.PebBaseAddress << L"\n";
            }
        }
    }

    // GWorld okuma testi
    uintptr_t gworld_ptr = read_mem<uintptr_t>(variables::base_address + offsets::g_world);
    std::wcout << L"[+] Hafizadan Okunan GWorld: 0x" << std::hex << gworld_ptr << L"\n";

    std::wcout << L"[*] Hile aktif. Kapatmak icin konsolu kapatabilirsiniz.\n";
    
    while (true) {
        Sleep(1000);
    }

    return 0;
}

