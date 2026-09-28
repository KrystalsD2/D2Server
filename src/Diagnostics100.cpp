#include "Diagnostics100.h"
#include <cstdio>
#include <mutex>
#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif
namespace d2server100 {
void DiagnosticLog100(const char* text) noexcept {
    static std::mutex m; std::lock_guard<std::mutex> lock(m);
    if (!text) return;
    char path[512]="D2Server100_v0_4.log";
#if defined(_WIN32)
    char mod[MAX_PATH]{}; DWORD n=GetModuleFileNameA(nullptr,mod,MAX_PATH);
    if (n && n<MAX_PATH) {
        char* s=mod+n; while(s>mod && s[-1]!='\\' && s[-1]!='/') --s;
        *s='\0'; std::snprintf(path,sizeof(path),"%sD2Server100_v0_4.log",mod);
    }
#endif
    if (FILE* f=std::fopen(path,"ab")) { std::fprintf(f,"%s\r\n",text); std::fclose(f); }
}
}
