#include "PhotoshopHelpers.h"

FilterRecord *gFilterRecord;
HINSTANCE g_hInstance;

string StringUtil::vssprintf(const char *szFormat, va_list va)
{
    int iBytes = vsnprintf(NULL, 0, szFormat, va);
    char *pBuf = (char*) _alloca(iBytes + 1);
    vsnprintf(pBuf, iBytes + 1, szFormat, va);
    return string(pBuf, iBytes);
}

string StringUtil::ssprintf(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    return vssprintf(fmt, va);
}
