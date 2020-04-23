/*
 * Copyright 2020 transmission.aquitaine@yahoo.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>



#ifdef _MSC_VER /* Microsoft compiler */

#include <tchar.h>

#ifdef _UNICODE
#define fopen_s _wfopen_s
#endif

#endif

#ifdef __GNUC__ /* GNU compiler */

#define TCHAR     char
#define _T(s)     s
#define _tmain    main
#define _tprintf  printf
#define _ftprintf fprintf
#define _tfopen   fopen

typedef int errno_t;

static errno_t fopen_s(FILE **f, const char *name, const char *mode)
    {
    *f = fopen(name, mode);
    return f ? 0 : 1;
    }

#endif



union EndianTester
    {
    unsigned long ul;
    char ca[4];
    };

int _tmain(int argc, TCHAR* argv[])
    {
    FILE *fout;
    errno_t e = fopen_s(&fout, _T("addr_width.h"), _T("w"));
    if (e)
        {
        _tprintf(_T("Unable to open addr_width.h for writing.\n"));
        return 1;
        }
    _ftprintf(fout, _T("\n#ifndef ADDR_WIDTH\n"));
    _ftprintf(fout, _T("#define ADDR_WIDTH (%u)\n"), (unsigned int)(8*sizeof(void*)));
    _ftprintf(fout, _T("#endif\n\n"));
    EndianTester et;
    et.ul = 1;
    if (et.ca[0])
        {
        _ftprintf(fout, _T("#ifndef IS_INTEL_ENDIAN\n"));
        _ftprintf(fout, _T("#define IS_INTEL_ENDIAN (1)\n"));
        _ftprintf(fout, _T("#endif\n\n"));
        _ftprintf(fout, _T("#ifdef IS_MOTOROLA_ENDIAN\n"));
        _ftprintf(fout, _T("#undef IS_MOTOROLA_ENDIAN\n"));
        _ftprintf(fout, _T("#endif\n"));
        _tprintf(_T("\n     **************************************************\n"));
        _tprintf(_T("     *   This is a %u-bit little endian executable.   *\n"), (unsigned int)(8*sizeof(void*)));
        _tprintf(_T("     **************************************************\n\n"));
        }
    else
        {
        _ftprintf(fout, _T("#ifdef IS_INTEL_ENDIAN\n"));
        _ftprintf(fout, _T("#undef IS_INTEL_ENDIAN\n"));
        _ftprintf(fout, _T("#endif\n\n"));
        _ftprintf(fout, _T("#ifndef IS_MOTOROLA_ENDIAN\n"));
        _ftprintf(fout, _T("#define IS_MOTOROLA_ENDIAN (1)\n"));
        _ftprintf(fout, _T("#endif\n"));
        _tprintf(_T("\n     ********************************************\n"));
        _tprintf(_T("     *   This a %u-bit big endian executable.   *\n"), (unsigned int)(8*sizeof(void*)));
        _tprintf(_T("     ********************************************\n\n"));
        }
    fclose(fout);
    return 0;
    }
