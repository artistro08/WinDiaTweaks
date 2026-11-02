// diatweaks.exe - attaches DiaTweaksTap.dll to a running Dia (Windows) and manages tweaks.conf.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <tlhelp32.h>
#include <appmodel.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "shared.h"

static std::wstring g_dir;  // folder of diatweaks.exe

static std::wstring Lower(std::wstring s) {
    for (auto& c : s) c = (wchar_t)towlower(c);
    return s;
}

// ---------------------------------------------------------------- finding Dia

struct WinSearch {
    DWORD pid = 0;
};

static BOOL CALLBACK FindWinUIWindow(HWND hwnd, LPARAM lp) {
    auto* ws = reinterpret_cast<std::vector<DWORD>*>(lp);
    if (!IsWindowVisible(hwnd)) return TRUE;
    wchar_t cls[128];
    GetClassNameW(hwnd, cls, 128);
    if (wcscmp(cls, L"WinUIDesktopWin32WindowClass") != 0) return TRUE;
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    ws->push_back(pid);
    return TRUE;
}

static std::vector<DWORD> DiaPids() {
    std::vector<DWORD> out;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe{sizeof(pe)};
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe))
        if (_wcsicmp(pe.szExeFile, L"Dia.exe") == 0) out.push_back(pe.th32ProcessID);
    CloseHandle(snap);
    return out;
}

// The browser (UI) process is the Dia.exe that owns a WinUI window.
static DWORD FindDiaUiPid() {
    auto dia = DiaPids();
    std::vector<DWORD> winPids;
    EnumWindows(FindWinUIWindow, (LPARAM)&winPids);
    for (DWORD p : winPids)
        for (DWORD d : dia)
            if (p == d) return p;
    return 0;
}

static std::wstring ProcessPath(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return {};
    wchar_t buf[MAX_PATH];
    DWORD n = MAX_PATH;
    std::wstring r;
    if (QueryFullProcessImageNameW(h, 0, buf, &n)) r = buf;
    CloseHandle(h);
    return r;
}

static bool TapLoaded(DWORD pid) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return false;
    MODULEENTRY32W me{sizeof(me)};
    bool found = false;
    for (BOOL ok = Module32FirstW(snap, &me); ok; ok = Module32NextW(snap, &me))
        if (_wcsicmp(me.szModule, L"DiaTweaksTap.dll") == 0) found = true;
    CloseHandle(snap);
    return found;
}

// ---------------------------------------------------------------- launching with the tap

static std::wstring W(const std::string& s);
static std::string N(const std::wstring& w);

static std::wstring Guid() {
    LPOLESTR s = nullptr;
    StringFromCLSID(CLSID_DiaTweaksTap, &s);
    std::wstring r = s ? s : L"";
    CoTaskMemFree(s);
    return r;  // "{....}"
}

// Dia's install folder from the package registry (works with Dia closed). Family name is fixed.
static std::wstring DiaPackageDir() {
    const wchar_t* family = L"TheBrowserCompany.Dia_ttt1ap7aakyb4";
    UINT32 count = 0, bufLen = 0;
    long r = FindPackagesByPackageFamily(family, PACKAGE_FILTER_HEAD, &count, nullptr, &bufLen, nullptr, nullptr);
    if (r != ERROR_INSUFFICIENT_BUFFER || !count) return {};
    std::vector<wchar_t*> names(count);
    std::vector<wchar_t> buf(bufLen);
    if (FindPackagesByPackageFamily(family, PACKAGE_FILTER_HEAD, &count, names.data(), &bufLen, buf.data(),
                                    nullptr) != ERROR_SUCCESS)
        return {};
    UINT32 pathLen = 0;
    r = GetPackagePathByFullName(names[0], &pathLen, nullptr);
    if (r != ERROR_INSUFFICIENT_BUFFER) return {};
    std::vector<wchar_t> path(pathLen);
    if (GetPackagePathByFullName(names[0], &pathLen, path.data()) != ERROR_SUCCESS) return {};
    return std::wstring(path.data());
}

// Full path to Dia's real executable. Prefer a running Dia (also caches it), else the package
// registry. Used as a fallback launch path when app activation isn't available.
static std::wstring DiaExePath() {
    DWORD pid = FindDiaUiPid();
    if (!pid)
        for (DWORD p : DiaPids()) { pid = p; break; }
    std::wstring cache = g_dir + L"\\dia_path.txt";
    if (pid) {
        std::wstring p = ProcessPath(pid);
        if (!p.empty()) {
            std::ofstream out(cache, std::ios::binary);
            std::string n = N(p);
            out.write(n.data(), n.size());
            return p;
        }
    }
    std::wstring dir = DiaPackageDir();
    if (!dir.empty()) return dir + L"\\Dia.exe";
    std::ifstream in(cache, std::ios::binary);
    std::string s((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) s.pop_back();
    return W(s);
}

// Put DiaTweaksTap.dll into a running Dia. The add-on then attaches itself from inside Dia via the
// in-process WinUI diagnostics API. Nothing in Dia's install folder is touched.
static bool InjectTap(DWORD pid) {
    if (TapLoaded(pid)) return true;
    std::wstring dll = g_dir + L"\\DiaTweaksTap.dll";
    if (GetFileAttributesW(dll.c_str()) == INVALID_FILE_ATTRIBUTES) {
        fwprintf(stderr, L"%ls is missing. Run build.cmd first.\n", dll.c_str());
        return false;
    }
    HANDLE proc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
                                  PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                              FALSE, pid);
    if (!proc) {
        fwprintf(stderr, L"Can't open Dia (error %lu).\n", GetLastError());
        return false;
    }
    size_t bytes = (dll.size() + 1) * sizeof(wchar_t);
    void* mem = VirtualAllocEx(proc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    bool ok = mem && WriteProcessMemory(proc, mem, dll.c_str(), bytes, nullptr);
    if (ok) {
        auto loadLib =
            (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
        HANDLE th = CreateRemoteThread(proc, nullptr, 0, loadLib, mem, 0, nullptr);
        if (!th) {
            ok = false;
        } else {
            WaitForSingleObject(th, 15000);
            CloseHandle(th);
        }
    }
    if (mem) VirtualFreeEx(proc, mem, 0, MEM_RELEASE);
    CloseHandle(proc);
    if (!ok) {
        fwprintf(stderr, L"Couldn't load the add-on into Dia (error %lu).\n", GetLastError());
        return false;
    }
    for (int i = 0; i < 60 && !TapLoaded(pid); i++) Sleep(100);
    return TapLoaded(pid);
}

// Start Dia the normal way: through the package's own activation path, so it comes up exactly as the
// user expects. Dia hands its UI off to a broker-launched process, so environment variables can't be
// passed in - the add-on is attached once Dia is up instead.
static bool LaunchTweaked() {
    std::wstring exe = DiaExePath();
    if (exe.empty()) {
        fwprintf(stderr, L"Can't find Dia's location. Open Dia once (normally), then run this again.\n");
        return false;
    }
    HINSTANCE h = ShellExecuteW(nullptr, L"open", L"explorer.exe",
                                L"shell:AppsFolder\\TheBrowserCompany.Dia_ttt1ap7aakyb4!Dia", nullptr,
                                SW_SHOWNORMAL);
    if ((INT_PTR)h > 32) return true;

    std::wstring cwd = exe.substr(0, exe.find_last_of(L'\\'));
    STARTUPINFOW si{sizeof(si)};
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> buf(exe.begin(), exe.end());
    buf.push_back(0);
    BOOL ok = CreateProcessW(exe.c_str(), buf.data(), nullptr, nullptr, FALSE, 0, nullptr, cwd.c_str(),
                             &si, &pi);
    if (!ok) {
        fwprintf(stderr, L"Could not start Dia (error %lu)\n", GetLastError());
        return false;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

static void Signal(LPCWSTR name) {
    HANDLE ev = OpenEventW(EVENT_MODIFY_STATE, FALSE, name);
    if (ev) SetEvent(ev), CloseHandle(ev);
}

// ---------------------------------------------------------------- config

struct TweakLine {
    size_t line;
    std::wstring key, title;
    bool on;
};

static std::vector<std::string> ReadLines() {
    std::vector<std::string> lines;
    std::ifstream in(g_dir + L"\\tweaks.conf");
    std::string l;
    while (std::getline(in, l)) {
        if (!l.empty() && l.back() == '\r') l.pop_back();
        lines.push_back(l);
    }
    return lines;
}

static std::wstring W(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}
static std::string N(const std::wstring& w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

static std::vector<TweakLine> ParseTweaks(const std::vector<std::string>& lines) {
    std::vector<TweakLine> out;
    for (size_t i = 0; i < lines.size(); i++) {
        std::wstring l = W(lines[i]);
        size_t a = l.find_first_not_of(L" \t");
        if (a == std::wstring::npos || l[a] != L'[') continue;
        size_t e = l.find(L']', a);
        if (e == std::wstring::npos) continue;
        TweakLine t{i, l.substr(a + 1, e - a - 1), L"", l.find(L"on", e) != std::wstring::npos};
        for (size_t j = i + 1; j < lines.size(); j++) {
            std::wstring m = W(lines[j]);
            size_t b = m.find_first_not_of(L" \t");
            if (b != std::wstring::npos && m[b] == L'[') break;
            if (b != std::wstring::npos && m.compare(b, 6, L"title:") == 0) {
                t.title = m.substr(b + 6);
                t.title.erase(0, t.title.find_first_not_of(L" \t"));
                break;
            }
        }
        out.push_back(t);
    }
    return out;
}

static void ListTweaks() {
    auto tweaks = ParseTweaks(ReadLines());
    for (auto& t : tweaks)
        wprintf(L"  [%ls] %-26ls %ls\n", t.on ? L"on " : L"off", t.key.c_str(), t.title.c_str());
}

static int SetTweak(const std::wstring& which, bool on) {
    auto lines = ReadLines();
    auto tweaks = ParseTweaks(lines);
    std::wstring w = Lower(which);
    bool hit = false;
    for (auto& t : tweaks) {
        if (w != L"all" && Lower(t.key) != w && Lower(t.title) != w) continue;
        lines[t.line] = "[" + N(t.key) + "] " + (on ? "on" : "off");
        wprintf(L"%ls %ls\n", on ? L"Enabled " : L"Disabled", t.key.c_str());
        hit = true;
    }
    if (!hit) {
        fwprintf(stderr, L"No tweak named \"%ls\". Run: diatweaks list\n", which.c_str());
        return 1;
    }
    std::ofstream out(g_dir + L"\\tweaks.conf", std::ios::binary);
    for (auto& l : lines) out << l << "\n";
    out.close();
    Signal(DT_EVENT_RELOAD);
    return 0;
}

// ---------------------------------------------------------------- launching

static DWORD WaitForTap(int seconds) {
    for (int i = 0; i < seconds * 4; i++) {
        DWORD p = FindDiaUiPid();
        if (p && TapLoaded(p)) return p;
        Sleep(250);
    }
    return 0;
}

static BOOL CALLBACK CloseWindows(HWND hwnd, LPARAM lp) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    for (DWORD d : *reinterpret_cast<std::vector<DWORD>*>(lp))
        if (d == pid) PostMessageW(hwnd, WM_CLOSE, 0, 0);
    return TRUE;
}

// Close every running Dia process so the next launch is a fresh instance we can attach to.
static void CloseDia() {
    auto pids = DiaPids();
    if (pids.empty()) return;
    EnumWindows(CloseWindows, (LPARAM)&pids);
    for (int i = 0; i < 40 && !DiaPids().empty(); i++) Sleep(250);
    for (DWORD p : DiaPids()) {
        HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, p);
        if (h) TerminateProcess(h, 0), CloseHandle(h);
    }
    for (int i = 0; i < 20 && !DiaPids().empty(); i++) Sleep(250);
}

// ---------------------------------------------------------------- commands

// Start a tweaked Dia, closing any plain instance first. Returns the tapped pid (0 on failure).
static DWORD StartTweaked(bool quiet) {
    CloseDia();
    if (!LaunchTweaked()) return 0;

    DWORD pid = 0;
    for (int i = 0; i < 160 && !pid; i++) {
        Sleep(250);
        pid = FindDiaUiPid();
    }
    if (!pid) {
        fwprintf(stderr, L"Dia didn't come up. Try opening Dia yourself, then run: diatweaks\n");
        return 0;
    }
    if (!InjectTap(pid)) return 0;

    pid = WaitForTap(30);
    if (!pid) {
        fwprintf(stderr, L"Dia started but tweaks didn't load. See %ls\\tap.log\n", g_dir.c_str());
        return 0;
    }
    if (!quiet) wprintf(L"Dia running with tweaks (pid %lu)\n", pid);
    return pid;
}

static int CmdApply() {
    DWORD pid = FindDiaUiPid();
    if (pid && TapLoaded(pid)) {
        Signal(DT_EVENT_RELOAD);
        wprintf(L"Tweaks reloaded in Dia (pid %lu)\n", pid);
        return 0;
    }
    return StartTweaked(false) ? 0 : 1;
}

static int CmdDump() {
    DWORD pid = FindDiaUiPid();
    if (!pid || !TapLoaded(pid)) {
        pid = StartTweaked(true);
        if (!pid) return 1;
        Sleep(1500);  // let the initial tree replay finish
    }
    HANDLE done = CreateEventW(nullptr, FALSE, FALSE, DT_EVENT_DONE);
    Signal(DT_EVENT_DUMP);
    if (WaitForSingleObject(done, 15000) != WAIT_OBJECT_0) {
        fwprintf(stderr, L"Dia didn't answer. See %ls\\tap.log\n", g_dir.c_str());
        return 1;
    }
    wprintf(L"Wrote %ls\\tree.txt\n", g_dir.c_str());
    return 0;
}

static int CmdStatus() {
    DWORD pid = FindDiaUiPid();
    if (!pid) wprintf(L"Dia: not running\n");
    else
        wprintf(L"Dia: running (pid %lu)\n  %ls\nTweaks loaded: %ls\n", pid, ProcessPath(pid).c_str(),
                TapLoaded(pid) ? L"yes" : L"no");
    wprintf(L"\nTweaks:\n");
    ListTweaks();
    return 0;
}

// Ask the add-on to describe live elements (for making tweaks): names go to probe.txt, the answer
// is appended to probe.out. A trailing * also lists everything under that element.
static int CmdProbe(int argc, wchar_t** argv) {
    DWORD pid = FindDiaUiPid();
    if (!pid || !TapLoaded(pid)) {
        fwprintf(stderr, L"Dia isn't running with tweaks. Run: diatweaks\n");
        return 1;
    }
    std::wstring out = g_dir + L"\\probe.out";
    DeleteFileW(out.c_str());
    {
        std::ofstream f(g_dir + L"\\probe.txt", std::ios::binary);
        for (int i = 2; i < argc; i++) f << N(argv[i]) << "\n";
    }
    for (int i = 0; i < 40; i++) {
        Sleep(250);
        if (GetFileAttributesW((g_dir + L"\\probe.txt").c_str()) == INVALID_FILE_ATTRIBUTES) {
            Sleep(300);
            std::ifstream in(out);
            std::string l;
            while (std::getline(in, l)) printf("%s\n", l.c_str());
            return 0;
        }
    }
    fwprintf(stderr, L"Dia didn't answer.\n");
    return 1;
}

// Set a tweak's value (for tweaks with a choice or text, e.g. windowMaterial acrylic).
static int CmdSet(const std::wstring& key, const std::wstring& value) {
    auto lines = ReadLines();
    auto tweaks = ParseTweaks(lines);
    for (auto& t : tweaks) {
        if (Lower(t.key) != Lower(key)) continue;
        size_t insertAt = t.line + 1;
        bool done = false;
        for (size_t j = t.line + 1; j < lines.size(); j++) {
            std::wstring m = W(lines[j]);
            size_t b = m.find_first_not_of(L" \t");
            if (b != std::wstring::npos && m[b] == L'[') break;
            if (b != std::wstring::npos && m.compare(b, 6, L"value:") == 0) {
                lines[j] = "value: " + N(value);
                done = true;
                break;
            }
            if (b != std::wstring::npos && m.compare(b, 6, L"title:") == 0) insertAt = j + 1;
        }
        if (!done) lines.insert(lines.begin() + insertAt, "value: " + N(value));
        std::ofstream out(g_dir + L"\\tweaks.conf", std::ios::binary);
        for (auto& l : lines) out << l << "\n";
        out.close();
        Signal(DT_EVENT_RELOAD);
        wprintf(L"%ls = %ls\n", t.key.c_str(), value.c_str());
        return 0;
    }
    fwprintf(stderr, L"No tweak named \"%ls\". Run: diatweaks list\n", key.c_str());
    return 1;
}

static void Usage() {
    wprintf(
        L"DiaTweaks for Windows (unofficial)\n\n"
        L"  diatweaks                 start Dia with tweaks (or reload if already tweaked)\n"
        L"  diatweaks restart         close Dia and start it again with tweaks\n"
        L"  diatweaks list            show tweaks and whether they're on\n"
        L"  diatweaks enable <name>   turn a tweak on (name or 'all')\n"
        L"  diatweaks disable <name>  turn a tweak off\n"
        L"  diatweaks status          Dia process + tweak state\n"
        L"  diatweaks set <name> <v>  set a tweak's option (e.g. set windowMaterial acrylic)\n"
        L"  diatweaks dump            write Dia's UI tree to tree.txt (for making tweaks)\n"
        L"  diatweaks probe <names>   describe live elements by name (for making tweaks)\n\n"
        L"Every tweak can also be switched in Dia: Settings > Patches.\n"
        L"Tweaks that are on apply the moment Dia starts, and live-reload on enable/disable.\n");
}

int wmain(int argc, wchar_t** argv) {
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    g_dir = self;
    g_dir = g_dir.substr(0, g_dir.find_last_of(L'\\'));
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    std::wstring cmd = argc > 1 ? Lower(argv[1]) : L"apply";
    if (cmd == L"apply") return CmdApply();
    if (cmd == L"launch" || cmd == L"restart") return StartTweaked(false) ? 0 : 1;
    if (cmd == L"list") return ListTweaks(), 0;
    if (cmd == L"status") return CmdStatus();
    if (cmd == L"dump") return CmdDump();
    if (cmd == L"probe") return CmdProbe(argc, argv);
    if (cmd == L"set" && argc > 3) return CmdSet(argv[2], argv[3]);
    if ((cmd == L"enable" || cmd == L"disable") && argc > 2) {
        std::wstring name = argv[2];
        for (int i = 3; i < argc; i++) name += std::wstring(L" ") + argv[i];
        int r = SetTweak(name, cmd == L"enable");
        if (!r) {
            DWORD pid = FindDiaUiPid();
            if (pid && TapLoaded(pid)) Signal(DT_EVENT_RELOAD);
        }
        return r;
    }
    Usage();
    return cmd == L"help" || cmd == L"-h" || cmd == L"--help" ? 0 : 1;
}
