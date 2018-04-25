#ifndef PhotoshopHelpers_h
#define PhotoshopHelpers_h

#include "PIUtilities.h"
#include "PIFilter.h"

extern FilterRecord *gFilterRecord;
extern HINSTANCE g_hInstance;

#include <string>
using namespace std;

template<typename T>
inline T clamp(T i, T low, T high)
{
    return max(low, min(i, high));
}

namespace StringUtil
{
    string vssprintf(const char *szFormat, va_list va);
    string ssprintf(const char *fmt, ...);
}

class ExceptionSPErr: public exception
{
public:
    ExceptionSPErr(string sText, SPErr iErr) throw(): exception(StringUtil::ssprintf("%s: %i", sText.c_str(), iErr).c_str())
    {
        m_iErr = iErr;
    }
    SPErr GetError() const { return m_iErr; }

private:
    SPErr m_iErr;
};

#endif