#pragma once
#include <windows.h>
#include <string>

// Definitions for WinDivert
#define WINDIVERT_LAYER_NETWORK 0

typedef HANDLE (*WinDivertOpen_t)(const char* filter, UINT32 layer, INT16 priority, UINT64 flags);
typedef BOOL (*WinDivertRecv_t)(HANDLE handle, PVOID pPacket, UINT packetLen, UINT* pRecvLen, PVOID pAddr);
typedef BOOL (*WinDivertClose_t)(HANDLE handle);

inline WinDivertOpen_t pWinDivertOpen = nullptr;
inline WinDivertRecv_t pWinDivertRecv = nullptr;
inline WinDivertClose_t pWinDivertClose = nullptr;

inline bool LoadWinDivert() {
    HMODULE hMod = LoadLibraryA("WinDivert64.dll");
    if (!hMod) return false;
    
    pWinDivertOpen = (WinDivertOpen_t)GetProcAddress(hMod, "WinDivertOpen");
    pWinDivertRecv = (WinDivertRecv_t)GetProcAddress(hMod, "WinDivertRecv");
    pWinDivertClose = (WinDivertClose_t)GetProcAddress(hMod, "WinDivertClose");
    
    return pWinDivertOpen && pWinDivertRecv && pWinDivertClose;
}
