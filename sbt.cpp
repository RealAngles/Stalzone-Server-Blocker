#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma execution_character_set("utf-8")
#endif
#include <windows.h>
#include <winhttp.h>
#include <d3d11.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <set>
#include <thread>
#include <atomic>
#include <fstream>
#include <algorithm>
#include <mutex>
#include "json.hpp"
#include "windivert_dynamic.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
using json = nlohmann::json;
HINSTANCE g_hInst;
HWND g_hWnd;
struct ServerInfo {
    std::wstring name;
    std::wstring address;
};
struct PoolInfo {
    std::wstring name;
    std::wstring region;
    std::vector<ServerInfo> tunnels;
};
std::recursive_mutex g_PoolsMutex;
std::vector<PoolInfo> g_Pools;
std::set<std::string> g_SelectedAddresses;
std::set<std::string> g_ExpandedPools;
std::string g_CurrentRegion = "RU";
std::atomic<bool> g_bBlocking = false;
HANDLE g_hDivert = INVALID_HANDLE_VALUE;
std::thread g_BlockThread;
const ImVec4 COL_BG = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
const ImVec4 COL_BTN_BG = ImVec4(153.0f / 255.0f, 50.0f / 255.0f, 204.0f / 255.0f, 1.0f); 
const ImVec4 COL_BTN_BORDER = ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f, 1.0f); 
const ImVec4 COL_BTN_STOP_BG = ImVec4(255.0f / 255.0f, 68.0f / 255.0f, 68.0f / 255.0f, 1.0f); 
const ImVec4 COL_BTN_STOP_BORDER = ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 1.0f); 
const ImVec4 COL_TEXT = ImVec4(224.0f / 255.0f, 224.0f / 255.0f, 224.0f / 255.0f, 1.0f); 
const ImVec4 COL_LIST_BG = ImVec4(10.0f / 255.0f, 10.0f / 255.0f, 10.0f / 255.0f, 1.0f); 
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void StartBlocking();
void StopBlocking();
void UpdateBlockingIfActive();
std::wstring Utf8ToWstring(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
std::string WstringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
bool IsUserAnAdmin() {
    BOOL fIsRunAsAdmin = FALSE;
    PSID pAdministratorsGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdministratorsGroup)) {
        CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin);
        FreeSid(pAdministratorsGroup);
    }
    return fIsRunAsAdmin != FALSE;
}
void RelaunchAsAdmin() {
    wchar_t szPath[MAX_PATH];
    if (GetModuleFileNameW(NULL, szPath, MAX_PATH)) {
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = szPath;
        sei.hwnd = NULL;
        sei.nShow = SW_NORMAL;
        if (ShellExecuteExW(&sei)) {
            ExitProcess(0);
        }
    }
}
std::wstring GetExeDirW() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
    return std::wstring(buffer).substr(0, pos);
}
void LoadSettings() {
    std::wstring path = GetExeDirW() + L"\\Settings.json";
    std::ifstream f(path);
    if (!f.is_open()) {
        f.open("Settings.json");
    }
    if (f.is_open()) {
        try {
            json j;
            f >> j;
            if (j.contains("selected_servers") && j["selected_servers"].is_array()) {
                for (const auto& item : j["selected_servers"]) {
                    if (item.is_string()) {
                        g_SelectedAddresses.insert(item.get<std::string>());
                    }
                }
            }
        }
        catch (...) {}
        f.close();
    }
}
void SaveSettings() {
    std::wstring path = GetExeDirW() + L"\\Settings.json";
    json j;
    std::ifstream inFile(path);
    if (!inFile.is_open()) {
        inFile.open("Settings.json");
    }
    if (inFile.is_open()) {
        try {
            inFile >> j;
        }
        catch (...) {}
        inFile.close();
    }
    j["language"] = "ru";
    j["region"] = "RU";
    std::vector<std::string> servers;
    std::vector<std::string> names;
    {
        std::lock_guard<std::recursive_mutex> lock(g_PoolsMutex);
        for (const auto& addr : g_SelectedAddresses) {
            servers.push_back(addr);
            std::wstring wAddr = Utf8ToWstring(addr);
            std::string foundName = "";
            for (const auto& pool : g_Pools) {
                for (const auto& server : pool.tunnels) {
                    if (server.address == wAddr) {
                        foundName = WstringToUtf8(server.name);
                        break;
                    }
                }
                if (!foundName.empty()) break;
            }
            if (!foundName.empty()) {
                names.push_back(foundName);
            }
            else {
                names.push_back(addr);
            }
        }
    }
    j["selected_servers"] = servers;
    j["selected_names"] = names;
    std::ofstream outFile(path);
    if (!outFile.is_open()) {
        outFile.open("Settings.json");
    }
    if (outFile.is_open()) {
        outFile << j.dump(2);
        outFile.close();
    }
}
void UninstallSBTDriver() {
    SC_HANDLE hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM) {
        SC_HANDLE hService = OpenServiceW(hSCM, L"SBTDriver", SERVICE_ALL_ACCESS);
        if (hService) {
            SERVICE_STATUS status;
            ControlService(hService, SERVICE_CONTROL_STOP, &status);
            Sleep(500);
            DeleteService(hService);
            CloseServiceHandle(hService);
            Sleep(500);
        }
        CloseServiceHandle(hSCM);
    }
}
bool InstallSBTDriver() {
    UninstallSBTDriver();
    std::wstring exeDir = GetExeDirW();
    std::wstring sourceSys = exeDir + L"\\WinDivert64.sys";
    SC_HANDLE hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCM) return false;
    SC_HANDLE hService = CreateServiceW(hSCM, L"SBTDriver", L"SBTDriver", SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, sourceSys.c_str(),
        NULL, NULL, NULL, NULL, NULL);
    if (!hService) {
        if (GetLastError() == ERROR_SERVICE_EXISTS) {
            hService = OpenServiceW(hSCM, L"SBTDriver", SERVICE_ALL_ACCESS);
        }
        else {
            CloseServiceHandle(hSCM);
            return false;
        }
    }
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    return true;
}
bool StartSBTDriver() {
    SC_HANDLE hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCM) return false;
    SC_HANDLE hService = OpenServiceW(hSCM, L"SBTDriver", SERVICE_ALL_ACCESS);
    if (!hService) {
        CloseServiceHandle(hSCM);
        return false;
    }
    SERVICE_STATUS status;
    if (QueryServiceStatus(hService, &status)) {
        if (status.dwCurrentState != SERVICE_RUNNING && status.dwCurrentState != SERVICE_START_PENDING) {
            StartServiceW(hService, 0, NULL);
            Sleep(500);
        }
    }
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    return true;
}
std::string FetchUrl(const std::wstring& url) {
    std::string result;
    HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return result;
    URL_COMPONENTS urlComp = { 0 };
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostName[256] = { 0 };
    wchar_t urlPath[1024] = { 0 };
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 1024;
    if (WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp)) {
        HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
        if (hConnect) {
            DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
            if (hRequest) {
                DWORD dwSecurityFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                    SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
                    SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                    SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
                WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwSecurityFlags, sizeof(dwSecurityFlags));
                LPCWSTR additionalHeaders = L"Accept: application/json, text/plain, */*\r\nAccept-Language: ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7\r\n";
                if (WinHttpSendRequest(hRequest, additionalHeaders, -1, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                    if (WinHttpReceiveResponse(hRequest, NULL)) {
                        DWORD size = 0;
                        DWORD downloaded = 0;
                        do {
                            if (WinHttpQueryDataAvailable(hRequest, &size) && size > 0) {
                                char* buf = new char[size + 1];
                                if (WinHttpReadData(hRequest, (LPVOID)buf, size, &downloaded)) {
                                    buf[downloaded] = 0;
                                    result.append(buf, downloaded);
                                }
                                delete[] buf;
                            }
                        } while (size > 0);
                    }
                }
                WinHttpCloseHandle(hRequest);
            }
            WinHttpCloseHandle(hConnect);
        }
    }
    WinHttpCloseHandle(hSession);
    return result;
}
void LoadServers() {
    std::vector<PoolInfo> tempPools;
    std::wstring exeDir = GetExeDirW();
    std::wstring serversPath = exeDir + L"\\Servers.json";
    std::ifstream f(serversPath);
    if (!f.is_open()) {
        f.open("Servers.json");
    }
    if (f.is_open()) {
        try {
            json j;
            f >> j;
            if (j.contains("pools")) {
                for (auto& pool : j["pools"]) {
                    PoolInfo pInfo;
                    pInfo.name = Utf8ToWstring(pool.value("name", ""));
                    pInfo.region = Utf8ToWstring(pool.value("region", ""));
                    if (pool.contains("tunnels")) {
                        for (auto& tunnel : pool["tunnels"]) {
                            ServerInfo sInfo;
                            sInfo.name = Utf8ToWstring(tunnel.value("name", ""));
                            sInfo.address = Utf8ToWstring(tunnel.value("address", ""));
                            if (!sInfo.name.empty() && !sInfo.address.empty()) {
                                pInfo.tunnels.push_back(sInfo);
                            }
                        }
                    }
                    if (!pInfo.name.empty() && !pInfo.tunnels.empty()) {
                        tempPools.push_back(pInfo);
                    }
                }
            }
        }
        catch (...) {}
        f.close();
    }
    {
        std::lock_guard<std::recursive_mutex> lock(g_PoolsMutex);
        g_Pools = tempPools;
    }
    std::string ruJsonStr = FetchUrl(L"https://raw.githubusercontent.com/RealAngles/Stalzone-Server-Blocker/refs/heads/main/Servers.json");
    if (!ruJsonStr.empty()) {
        try {
            json j = json::parse(ruJsonStr);
            if (j.contains("pools")) {
                tempPools.erase(std::remove_if(tempPools.begin(), tempPools.end(), [](const PoolInfo& p) {
                    return p.region == L"RU";
                    }), tempPools.end());
                for (auto& pool : j["pools"]) {
                    PoolInfo pInfo;
                    pInfo.name = Utf8ToWstring(pool.value("name", ""));
                    pInfo.region = L"RU";
                    if (pool.contains("tunnels")) {
                        for (auto& tunnel : pool["tunnels"]) {
                            ServerInfo sInfo;
                            sInfo.name = Utf8ToWstring(tunnel.value("name", ""));
                            sInfo.address = Utf8ToWstring(tunnel.value("address", ""));
                            if (!sInfo.name.empty() && !sInfo.address.empty()) {
                                pInfo.tunnels.push_back(sInfo);
                            }
                        }
                    }
                    if (!pInfo.name.empty() && !pInfo.tunnels.empty()) {
                        tempPools.push_back(pInfo);
                    }
                }
                {
                    std::lock_guard<std::recursive_mutex> lock(g_PoolsMutex);
                    g_Pools = tempPools;
                }
            }
        }
        catch (...) {}
    }
    {
        std::lock_guard<std::recursive_mutex> lock(g_PoolsMutex);
        std::set<std::string> validAddresses;
        for (const auto& pool : g_Pools) {
            for (const auto& server : pool.tunnels) {
                validAddresses.insert(WstringToUtf8(server.address));
            }
        }

        bool changed = false;
        for (auto it = g_SelectedAddresses.begin(); it != g_SelectedAddresses.end(); ) {
            if (validAddresses.find(*it) == validAddresses.end()) {
                it = g_SelectedAddresses.erase(it);
                changed = true;
            }
            else {
                ++it;
            }
        }

        if (changed) {
            UpdateBlockingIfActive();
        }
        SaveSettings();
    }
}
void BlockThreadProc() {
    char addr[256];
    char packet[8192];
    UINT recvLen = 0;
    while (g_bBlocking) {
        if (g_hDivert == INVALID_HANDLE_VALUE) break;
        if (pWinDivertRecv(g_hDivert, packet, sizeof(packet), &recvLen, addr)) {
        }
        else {
            Sleep(10);
        }
    }
}
void StartBlocking() {
    if (!LoadWinDivert()) {
        MessageBoxW(g_hWnd, L"Не удалось загрузить WinDivert64.dll!\nУбедитесь, что библиотека лежит рядом с этой программой.", L"Ошибка", MB_ICONERROR);
        return;
    }
    if (!StartSBTDriver()) {
        MessageBoxW(g_hWnd, L"Не удалось запустить службу SBTDriver. Убедитесь, что запуск выполнен от имени Администратора.", L"Ошибка", MB_ICONERROR);
        return;
    }
    if (g_SelectedAddresses.empty()) {
        MessageBoxW(g_hWnd, L"Выберите хотя бы один сервер для блокировки.", L"Информация", MB_ICONINFORMATION);
        return;
    }
    std::string filter = "outbound and (";
    bool first = true;
    for (const auto& addr : g_SelectedAddresses) {
        std::string ip = addr.substr(0, addr.find(':'));
        if (!first) filter += " or ";
        filter += "ip.DstAddr == " + ip;
        first = false;
    }
    filter += ") and ((tcp.DstPort >= 29450 and tcp.DstPort <= 29460) or (udp.DstPort >= 29450 and udp.DstPort <= 29460))";
    g_hDivert = pWinDivertOpen(filter.c_str(), WINDIVERT_LAYER_NETWORK, 0, 0);
    if (g_hDivert == INVALID_HANDLE_VALUE) {
        MessageBoxW(g_hWnd, L"Не удалось открыть WinDivert. Убедитесь, что запуск выполнен от имени Администратора, а WinDivert64.sys находится рядом с exe.", L"Ошибка", MB_ICONERROR);
        return;
    }
    g_bBlocking = true;
    g_BlockThread = std::thread(BlockThreadProc);
}
void StopBlocking() {
    g_bBlocking = false;
    if (g_hDivert != INVALID_HANDLE_VALUE) {
        pWinDivertClose(g_hDivert);
        g_hDivert = INVALID_HANDLE_VALUE;
    }
    if (g_BlockThread.joinable()) {
        g_BlockThread.join();
    }
}
void UpdateBlockingIfActive() {
    if (g_bBlocking) {
        StopBlocking();
        if (!g_SelectedAddresses.empty()) {
            StartBlocking();
        }
    }
}
bool CustomCheckboxState(const char* label, bool* v, int state) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float square_sz = 28.0f; 
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    float width = square_sz + (label_size.x > 0.0f ? s.ItemInnerSpacing.x + label_size.x : 0.0f);
    float height = (square_sz > label_size.y) ? square_sz : label_size.y;
    std::string btn_id = "##btn_";
    btn_id += label;
    bool pressed = ImGui::InvisibleButton(btn_id.c_str(), ImVec2(width, height));
    if (pressed) {
        *v = !(*v);
    }
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImU32 bg_col = ImGui::GetColorU32(ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f));
    draw_list->AddRectFilled(pos, ImVec2(pos.x + square_sz, pos.y + square_sz), bg_col, 4.0f);
    ImU32 border_col = (state == 2) ? ImGui::GetColorU32(COL_BTN_BORDER) : ImGui::GetColorU32(COL_BTN_BG);
    draw_list->AddRect(pos, ImVec2(pos.x + square_sz, pos.y + square_sz), border_col, 4.0f, 0, 2.0f);
    if (state == 2) { 
        draw_list->AddRectFilled(ImVec2(pos.x + 6.0f, pos.y + 6.0f), ImVec2(pos.x + square_sz - 6.0f, pos.y + square_sz - 6.0f), ImGui::GetColorU32(COL_BTN_BORDER), 2.0f);
    }
    else if (state == 3) { 
        draw_list->AddRectFilled(ImVec2(pos.x + 8.0f, pos.y + 8.0f), ImVec2(pos.x + square_sz - 8.0f, pos.y + square_sz - 8.0f), ImGui::GetColorU32(COL_BTN_BG), 1.0f);
    }
    if (label_size.x > 0.0f) {
        ImVec2 text_pos = ImVec2(pos.x + square_sz + s.ItemInnerSpacing.x, pos.y + (square_sz - label_size.y) * 0.5f);
        draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), label);
    }
    return pressed;
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if (!IsUserAnAdmin()) {
        RelaunchAsAdmin();
        return 0;
    }
    InstallSBTDriver();
    g_hInst = hInstance;
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wc.lpszClassName = L"SBTImGuiClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = 660;
    int windowHeight = 639;
    int xPos = (screenWidth - windowWidth) / 2;
    int yPos = (screenHeight - windowHeight) / 2;
    HWND hwnd = CreateWindowW(L"SBTImGuiClass", L"Stalzone Server Blocker",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        xPos, yPos, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) return 1;
    g_hWnd = hwnd;
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        return 1;
    }
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    char font_path[MAX_PATH];
    GetWindowsDirectoryA(font_path, MAX_PATH);
    std::string arial_path = std::string(font_path) + "\\Fonts\\arial.ttf";
    ImFontConfig font_config;
    font_config.RasterizerMultiply = 1.2f; 
    ImFont* normal_font = io.Fonts->AddFontFromFileTTF(arial_path.c_str(), 17.0f, &font_config, io.Fonts->GetGlyphRangesCyrillic());
    ImFont* title_font = io.Fonts->AddFontFromFileTTF(arial_path.c_str(), 24.0f, &font_config, io.Fonts->GetGlyphRangesCyrillic());
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 2.0f;
    style.FrameBorderSize = 2.0f;
    style.Colors[ImGuiCol_WindowBg] = COL_BG;
    style.Colors[ImGuiCol_ChildBg] = COL_BG;
    style.Colors[ImGuiCol_PopupBg] = COL_LIST_BG;
    style.Colors[ImGuiCol_Border] = COL_BTN_BG;
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_FrameBg] = COL_LIST_BG;
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(31.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(40.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 1.0f);
    style.Colors[ImGuiCol_Button] = COL_BTN_BG;
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(139.0f / 255.0f, 0.0f / 255.0f, 139.0f / 255.0f, 1.0f); 
    style.Colors[ImGuiCol_ButtonActive] = COL_BTN_BORDER;
    style.Colors[ImGuiCol_Text] = COL_TEXT;
    style.Colors[ImGuiCol_ScrollbarBg] = COL_LIST_BG;
    style.Colors[ImGuiCol_ScrollbarGrab] = COL_BTN_BG;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = COL_BTN_BORDER;
    style.Colors[ImGuiCol_ScrollbarGrabActive] = COL_BTN_BORDER;
    style.Colors[ImGuiCol_Header] = ImVec4(31.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(45.0f / 255.0f, 45.0f / 255.0f, 45.0f / 255.0f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = COL_BTN_BG;
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    LoadSettings();
    std::thread(LoadServers).detach();
    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(644, 600));
        ImGui::Begin("MainLayout", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        bool is_blocking = g_bBlocking.load();
        ImGui::SetCursorPos(ImVec2(12, 12));
        ImGui::PushStyleColor(ImGuiCol_Border, COL_BTN_BG);
        ImGui::BeginChild("ServerTreeFrame", ImVec2(340, 576), true, ImGuiWindowFlags_None);
        ImGui::PopStyleColor();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 12));
        {
            std::lock_guard<std::recursive_mutex> lock(g_PoolsMutex);
            for (auto& pool : g_Pools) {
                std::string regUtf8 = WstringToUtf8(pool.region);
                if (regUtf8 != g_CurrentRegion) continue;
                std::string poolNameUtf8 = WstringToUtf8(pool.name);
                ImGui::PushID(poolNameUtf8.c_str());
                int checkedCount = 0;
                int totalCount = (int)pool.tunnels.size();
                for (const auto& s : pool.tunnels) {
                    if (g_SelectedAddresses.count(WstringToUtf8(s.address)) > 0) {
                        checkedCount++;
                    }
                }
                int poolState = 1; 
                if (totalCount > 0) {
                    if (checkedCount == totalCount) poolState = 2; 
                    else if (checkedCount > 0) poolState = 3; 
                }
                bool isOpen = (g_ExpandedPools.count(poolNameUtf8) > 0);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                bool toggled = ImGui::ArrowButton(("##arrow_" + poolNameUtf8).c_str(), isOpen ? ImGuiDir_Down : ImGuiDir_Right);
                if (toggled) {
                    if (isOpen) g_ExpandedPools.erase(poolNameUtf8);
                    else g_ExpandedPools.insert(poolNameUtf8);
                    isOpen = !isOpen;
                }
                ImGui::PopStyleVar();
                ImGui::SameLine();
                bool val = (poolState == 2);
                if (CustomCheckboxState("##poolcheck", &val, poolState)) {
                    for (const auto& s : pool.tunnels) {
                        std::string addrStr = WstringToUtf8(s.address);
                        if (val) {
                            g_SelectedAddresses.insert(addrStr);
                        }
                        else {
                            g_SelectedAddresses.erase(addrStr);
                        }
                    }
                    SaveSettings();
                    UpdateBlockingIfActive();
                }
                ImGui::SameLine();
                ImGui::Text("%s", poolNameUtf8.c_str());
                if (isOpen) {
                    ImGui::Indent(60.0f);
                    for (auto& server : pool.tunnels) {
                        std::string sAddr = WstringToUtf8(server.address);
                        std::string sName = WstringToUtf8(server.name);
                        ImGui::PushID(sAddr.c_str());
                        bool serverVal = (g_SelectedAddresses.count(sAddr) > 0);
                        if (CustomCheckboxState(sName.c_str(), &serverVal, serverVal ? 2 : 1)) {
                            if (serverVal) {
                                g_SelectedAddresses.insert(sAddr);
                            }
                            else {
                                g_SelectedAddresses.erase(sAddr);
                            }
                            SaveSettings();
                            UpdateBlockingIfActive();
                        }
                        ImGui::PopID();
                    }
                    ImGui::Unindent(30.0f);
                }
                ImGui::PopID();
                ImGui::Spacing();
            }
        }
        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::SetCursorPos(ImVec2(372, 12));
        ImGui::BeginGroup();
        ImGui::PushFont(title_font);
        float win_width = 264.0f;
        float text_width = ImGui::CalcTextSize("SBT").x;
        ImGui::SetCursorPosX(372.0f + (win_width - text_width) * 0.5f);
        ImGui::TextColored(COL_BTN_BORDER, "SBT");
        ImGui::PopFont();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 line1_start = ImVec2(372, ImGui::GetCursorScreenPos().y);
        ImVec2 line1_end = ImVec2(636, ImGui::GetCursorScreenPos().y);
        draw_list->AddLine(line1_start, line1_end, ImGui::GetColorU32(COL_BTN_BORDER), 2.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.0f);
        float btn_y = ImGui::GetCursorPosY();
        ImGui::SetCursorPosX(372.0f);
        ImGui::BeginDisabled(is_blocking);
        ImGui::PushStyleColor(ImGuiCol_Button, COL_BTN_BG);
        ImGui::PushStyleColor(ImGuiCol_Border, COL_BTN_BORDER);
        if (ImGui::Button((const char*)u8"Заблокировать", ImVec2(127, 45))) {
            StartBlocking();
        }
        ImGui::PopStyleColor(2);
        ImGui::EndDisabled();
        ImGui::SameLine(0, 10);
        ImGui::BeginDisabled(!is_blocking);
        ImGui::PushStyleColor(ImGuiCol_Button, COL_BTN_STOP_BG);
        ImGui::PushStyleColor(ImGuiCol_Border, COL_BTN_STOP_BORDER);
        if (ImGui::Button((const char*)u8"Разблокировать", ImVec2(127, 45))) {
            StopBlocking();
        }
        ImGui::PopStyleColor(2);
        ImGui::EndDisabled();
        ImGui::SetCursorPosY(btn_y + 60.0f);
        ImVec2 line2_start = ImVec2(372, ImGui::GetCursorScreenPos().y);
        ImVec2 line2_end = ImVec2(636, ImGui::GetCursorScreenPos().y);
        draw_list->AddLine(line2_start, line2_end, ImGui::GetColorU32(COL_BTN_BORDER), 2.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_LIST_BG);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(40.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 1.0f));
        ImGui::BeginChild("StatusContainer", ImVec2(264, 42), true);
        char statusStr[256];
        if (is_blocking) {
            sprintf_s(statusStr, (const char*)u8"Блокировка активна! (%zu)", g_SelectedAddresses.size());
        }
        else {
            sprintf_s(statusStr, (const char*)u8"Выбрано серверов: %zu", g_SelectedAddresses.size());
        }
        ImVec2 statusSize = ImGui::CalcTextSize(statusStr);
        ImGui::SetCursorPos(ImVec2((264.0f - statusSize.x) * 0.5f, (42.0f - statusSize.y) * 0.5f));
        ImGui::TextColored(COL_BTN_BORDER, "%s", statusStr);
        ImGui::EndChild();
        ImGui::PopStyleColor(2);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.0f);
        ImGui::Text((const char*)u8"Выберите нужные серверы.");
        ImGui::Text((const char*)u8"Если ");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(COL_BTN_BORDER, (const char*)u8"заблокировать");
        ImGui::SameLine(0, 0);
        ImGui::Text((const char*)u8" их, игра");
        ImGui::Text((const char*)u8"не сможет к ним подключиться.");
        ImGui::SetCursorPosY(545.0f);
        const char* credits = "by YungDaggerStab & WeedSellerBand";
        float cred_width = ImGui::CalcTextSize(credits).x;
        ImGui::SetCursorPosX(372.0f + (win_width - cred_width) * 0.5f);
        ImGui::TextDisabled("%s", credits);
        ImGui::EndGroup();
        ImGui::End();
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        HRESULT hr = g_pSwapChain->Present(1, 0); 
        if (hr == DXGI_STATUS_OCCLUDED)
            g_SwapChainOccluded = true;
    }
    StopBlocking();
    UninstallSBTDriver();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(L"SBTImGuiClass", hInstance);
    return 0;
}
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) 
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;
    CreateRenderTarget();
    return true;
}
void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}
void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}
void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg) {
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) 
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}