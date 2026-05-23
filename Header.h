#pragma once
#include <windows.h>
#include <iostream>
#include <vector>
#include <TlHelp32.h>
#include <cstdint> // uintptr_t tipinin tanınması için kesinlikle şart!

namespace offsets
{
	// --- [ ANA ÇEKİRDEK ADRESLER (Core Offsets) ] ---
	constexpr uintptr_t g_world = 0x7E93198;
	constexpr uintptr_t g_objects = 0x7D12B30;
	constexpr uintptr_t g_names = 0x7BF8048;

	// --- [ UNREAL ENGINE MOTOR İÇ ADRESLERİ ] ---
	constexpr uintptr_t u_object_name = 0x18;
	constexpr uintptr_t u_object_class = 0x10;
	constexpr uintptr_t u_field_next = 0x28;
	constexpr uintptr_t u_struct_child_properties = 0x50;

	// --- [ OYUNCU VE DÜNYA AKTORLARI (ESP / Radar İçin) ] ---
	constexpr uintptr_t persistent_level = 0x30;
	constexpr uintptr_t actor_list = 0xA0; 
}

namespace variables
{
	inline HANDLE process_handle = nullptr;
	inline uintptr_t base_address = 0;
}

// Fonksiyon bildirimleri (Prototipler)
bool attach_process(const wchar_t* process_name);
uintptr_t get_module_base(const wchar_t* module_name);

// Bellek okuma şablonu
template <typename T>
T read_mem(uintptr_t address)
{
	T buffer{};
	if (variables::process_handle != nullptr) {
		ReadProcessMemory(variables::process_handle, (LPCVOID)address, &buffer, sizeof(T), nullptr);
	}
	return buffer;
}

// Bellek yazma şablonu
template <typename T>
void write_mem(uintptr_t address, T value)
{
	if (variables::process_handle != nullptr) {
		WriteProcessMemory(variables::process_handle, (LPVOID)address, &value, sizeof(T), nullptr);
	}
}
