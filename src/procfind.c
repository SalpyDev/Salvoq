
#include "procfind.h"
#include <ctype.h>
#include <stdio.h>

static int ieq(const wchar_t *a, const wchar_t *b) {
    while (*a && *b) { if (towlower(*a) != towlower(*b)) return 0; a++; b++; }
    return *a == *b;
}

DWORD procfind_pick_root(const wchar_t *exe) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe;
    DWORD best = 0;
    if (snap == INVALID_HANDLE_VALUE) return 0;
    pe.dwSize = sizeof pe;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (ieq(pe.szExeFile, exe)) {
                int parent_is_match = 0;
                PROCESSENTRY32W p2 = pe;
                p2.dwSize = sizeof p2;
                if (Process32FirstW(snap, &p2)) {
                    do {
                        if (p2.th32ProcessID == pe.th32ParentProcessID && ieq(p2.szExeFile, exe)) { parent_is_match = 1; break; }
                    } while (Process32NextW(snap, &p2));
                }
                if (!parent_is_match && (!best || pe.th32ProcessID < best)) best = pe.th32ProcessID;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return best;
}
