/* The MIT License (MIT)
 *
 * Copyright (C) 2017, djcj <djcj@gmx.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define USAGE "\n" \
  "This program is a wrapper for msvc's cl.exe and intended to be used\n" \
  "with Windows 10's \"Bash on Ubuntu on Windows\" shell.\n" \
  "It is invoked with gcc options (only a limited number) and turns\n" \
  "them into msvc options to call cl.exe.\n" \
  "The msvc options may not exactly do the same as their gcc counterparts.\n" \
  "\n" \
  "Supported GCC options (see `man gcc' for more information):\n" \
  "  -c -C -DDEFINE[=ARG] -fconstexpr-depth=num -ffp-contract=fast|off\n" \
  "  -finline-functions -fno-inline -fno-rtti -fno-threadsafe-statics\n" \
  "  -fomit-frame-pointer -fopenmp -fpermissive -fsized-deallocation -fstack-check\n" \
  "  -fstack-protector -funsigned-char -fwhole-program -g -include file -I path\n" \
  "  -llibname -L path -mavx -mavx2 -mdll -msse -msse2 -nodefaultlibs -nostdinc\n" \
  "  -nostdinc++ -nostdlib -O0 -O1 -O2 -O3 -Os -o file -print-search-dirs -shared\n" \
  "  -std=c<..>|gnu<..> -trigraphs -UDEFINE -w -Wall -Werror -Wextra\n" \
  "  -Wl,--out-implib,libname -Wl,-output-def,defname -Wl,--whole-archive -x <c|c++>\n" \
  "\n" \
  "Other options:\n" \
  "  --help                display this information\n" \
  "  --help-cl             display cl.exe's help information\n" \
  "  --help-link           display link.exe's help information\n" \
  "  --verbose             print commands\n" \
  "  --print-only          print commands and don't to anything\n" \
  "  --cl=path             path to cl.exe\n" \
  "  -Wcl,arg -Wlink,arg   parse msvc options directly to cl.exe/link.exe\n" \
  "\n" \
  "Environment variables:\n" \
  "  CL_CMD      path to cl.exe\n" \
  "\n" \
  "See also https://msdn.microsoft.com/en-us/library/19z1t1wy.aspx\n"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include <iostream>
#include <string>

#include "config.h"

#define STRNEQ(x,y,z) (strncmp(x,y,z) == 0 && len > z)
#define STR(x) std::string(x)


void fromto(const std::string &from, const std::string &to, std::string &str)
{
  size_t pos;

  for (pos = 0; (pos = str.find(from, pos)) != std::string::npos; ++pos /*pos += to.size()*/)
  {
    str.replace(pos, 1 /*from.size()*/, to);
  }
}

// C: is mounted as "/mnt/c", D: as "/mnt/d", and so on

std::string win_path(char *ch)
{
  char *rp;
  std::string str;

  rp = realpath(ch, NULL);
  str = std::string(rp);
  free(rp);

  if (str.substr(0,5) == "/mnt/" && str.substr(6,1) == "/")
  {
    str = str.substr(5,1) + ":" + str.substr(6);
  }

  fromto("/", "\\", str);
  return str;
}

std::string unix_path(std::string str)
{
  std::locale loc;
  std::string drive;

  if (str.substr(1,2) == ":\\")
  {
    drive = str.substr(0,1);
    if (drive.find_first_not_of("CDEFGHIJKLMNOPQRSTUVWXYZAB") == std::string::npos)
    {
      drive = std::tolower(drive[0], loc);
    }
    str = "/mnt/" + drive + str.substr(2);
  }

  fromto("\\", "/", str);
  return str;
}

void print_help(char *self)
{
  std::cout << "Usage: " << self << " [options] file...\n" << USAGE << std::endl;
}


int main(int argc, char **argv)
{
    std::string str, cmd, lnk, cl_cmd;
    cmd = lnk = cl_cmd = "";

    bool verbose, print_only, have_outname, do_link, default_include_paths, default_lib_paths, rtti, threadsafe_statics;
    verbose = print_only = have_outname = false;
    do_link = default_include_paths = default_lib_paths = rtti = threadsafe_statics = true;

    char *cl_cmd_env = getenv("CL_CMD");
    if (cl_cmd_env != NULL)
    {
        cl_cmd = STR(cl_cmd_env);
    }
    if (cl_cmd == "")
    {
        cl_cmd = DEFAULT_CL_CMD;
    }

    for (int i = 1; i < argc; ++i)
    {
        int len = strlen(argv[i]);
        char *arg = argv[i];
        str = STR(argv[i]);

        if (arg[0] == '-')
        {
            if (arg[1] == '-')
            {
                if (str == "--verbose")
                {
                    verbose = true;
                }
                if (str == "--print-only")
                {
                    verbose = print_only = true;
                }
                else if (STRNEQ(arg, "--cl=", 5))
                {
                    cl_cmd = STR(arg+5);
                }
                else if (str == "--help")
                {
                    print_help(argv[0]); return 0;
                }
                else if (STRNEQ(arg, "--help-", 7))
                {
                    if (str == "--help-cl")
                    {
                        cl_cmd = "'" + cl_cmd + "' /help";
                        return system(cl_cmd.c_str());
                    }
                    else if (str == "--help-link")
                    {
                        // like 'dirname(cl_cmd)'
                        cl_cmd = "'" + cl_cmd.substr(0, cl_cmd.find_last_of("/\\")) + "/link.exe'";
                        return system(cl_cmd.c_str());
                    }
                }
            }
            else
            {
                if ((arg[1] == '?' || arg[1] == 'h') && (len == 2 || str == "-help"))
                {
                    print_help(argv[0]);
                    return 0;
                }
                // -c -C -w
                if (len == 2 && (arg[1] == 'c' || arg[1] == 'C' || arg[1] == 'w'))
                {
                    cmd += " /" + std::string(arg+1);
                    if (arg[1] == 'c')
                    {
                        do_link = false;
                    }
                }
                // -x c  -x c++
                else if (arg[1] == 'x')
                {
                    if (str == "-x")
                    {
                        ++i;
                        if (i < argc)
                        {
                            if (STR(argv[i]) == "c")
                            {
                                cmd += " /TC";
                            }
                            else if (STR(argv[i]) == "c++")
                            {
                                cmd += " /TP";
                            }
                        }
                    }
                    else if (str == "-xc")
                    {
                        cmd += " /TC";
                    }
                    else if (str == "-xc++")
                    {
                        cmd += " /TP";
                    }
                }
                // -g
                else if (arg[1] == 'g' && len == 2)
                {
                    cmd += " /Zi";
                }
                // -o file
                else if (arg[1] == 'o')
                {
                    if (len == 2)
                    {
                        ++i;
                        if (i < argc)
                        {
                            lnk += " /out:'" + STR(argv[i]) + "'";
                        }
                    }
                    else
                    {
                        lnk += " /out:'" + STR(arg+2) + "'";
                    }
                    have_outname = true;
                }
                // -I path  -DDEFINE[=ARG]  -UDEFINE
                else if (arg[1] == 'I' || arg[1] == 'D' || arg[1] == 'U')
                {
                    if (len == 2)
                    {
                        ++i;
                        if (i < argc)
                        {
                            cmd += " /" + str.substr(1,1) + "'" + win_path(argv[i]) + "'";
                        }
                    }
                    else
                    {
                        cmd += " /" + str.substr(1,1) + "'" + win_path(arg+2) + "'";
                    }
                }
                // -L path
                else if (arg[1] == 'L')
                {
                    if (len == 2)
                    {
                        ++i;
                        if (i < argc)
                        {
                            lnk += " /libpath:'" + win_path(argv[i]) + "'";
                        }
                    }
                    else
                    {
                        lnk += " /libpath:'" + win_path(arg+2) + "'";
                    }
                }
                // -llibname
                else if (arg[1] == 'l' && len > 2)
                {
                    if (str == "-lmsvcrt")
                    {
                        cmd += " /MD";
                    }
                    else if (str == "-llibcmt")
                    {
                        cmd += " /MT";
                    }
                    else if (str != "-lc" && str != "-lm" && str != "-lrt" && str != "-lstdc++" && str != "-lgcc_s")
                    {
                        lnk += " '" + STR(arg+2) + ".lib'";
                    }
                }
                // -O0 -O1 -O2 -O3 -Os
                else if (arg[1] == 'O' && len == 3)
                {
                    if (arg[2] == '1' || arg[2] == '2')
                    {
                        cmd += " /O2 /Ot";
                    }
                    else if (arg[2] == '3')
                    {
                        cmd += " /Ox";
                    }
                    else if (arg[2] == 's')
                    {
                        cmd += " /O1 /Os";
                    }
                    else if (arg[2] == '0')
                    {
                        cmd += " /Od";
                    }
                }
                // -Wl,--whole-archive
                // -Wl,--out-implib,libname
                // -Wl,-output-def,defname
                // -Wall -Wextra -Werror
                // -Wcl,arg -Wlink,arg    parse options directly to cl.exe/link.exe (wrapper)
                else if (arg[1] == 'W' && len > 2)
                {
                    if (arg[2] == 'l')
                    {
                        std::string lopt = "";
                        if (len == 3)
                        {
                            ++i;
                            if (i < argc)
                            {
                                lopt = STR(argv[i]);
                            }
                        }
                        else
                        {
                            lopt = STR(arg+4);
                        }

                        if (lopt == "--whole-archive")
                        {
                            lnk += " /wholearchive";
                        }
                        else if (STRNEQ(lopt.c_str(), "--out-implib,", 13))
                        {
                            lnk += " /implib:'" + STR(arg+17) + "'";
                        }
                        else if (str == "-Wl,--out-implib")
                        {
                            ++i;
                            if (i < argc && STRNEQ(argv[i], "-Wl,", 4))
                            {
                                lnk += " /implib:'" + STR(argv[i]+4) + "'";
                            }
                        }
                        else if (STRNEQ(lopt.c_str(), "-output-def,", 12))
                        {
                            lnk += " /def:'" + STR(arg+16) + "'";
                        }
                        else if (str == "-Wl,-output-def")
                        {
                            ++i;
                            if (i < argc && STRNEQ(argv[i], "-Wl,", 4))
                            {
                                lnk += " /def:'" + STR(argv[i]+4) + "'";
                            }
                        }
                        else if (str == "-Wlink")
                        {
                            ++i;
                            if (i < argc)
                            {
                                lnk += " " + STR(argv[i]);
                            }
                        }
                        else if (STRNEQ(argv[i], "-Wlink,", 7))
                        {
                            lnk += " " + STR(arg+7);
                        }
                    }
                    else if (str == "-Wcl")
                    {
                        ++i;
                        if (i < argc)
                        {
                            cmd += " " + STR(argv[i]);
                        }
                    }
                    else if (STRNEQ(argv[i], "-Wcl,", 5))
                    {
                        cmd += " " + STR(arg+5);
                    }
                    else if (str == "-Wall")
                    {
                        cmd += " /W3";
                    }
                    else if (str == "-Wextra")
                    {
                        cmd += " /Wall";
                    }
                    else if (str == "-Werror")
                    {
                        cmd += " /WX";
                    }
                }
                // -mdll -msse -msse2 -mavx -mavx2
                else if (arg[1] == 'm' && len > 2)
                {
                    if (str == "-mdll")
                    {
                        cmd += " /LD";
                    }
                    else if (str == "-msse")
                    {
                        cmd += " /arch:SSE";
                    }
                    else if (str == "-msse2")
                    {
                        cmd += " /arch:SSE2";
                    }
                    else if (str == "-mavx")
                    {
                        cmd += " /arch:AVX";
                    }
                    else if (str == "-mavx2")
                    {
                        cmd += " /arch:AVX2";
                    }
                }
                // -fno-rtti -fno-threadsafe-statics -fno-inline -fomit-frame-pointer
                // -fpermissive -finline-functions -fopenmp -fstack-protector -fstack-check
                // -funsigned-char -fsized-deallocation -fconstexpr-depth=num
                // -ffp-contract=fast|off -fwhole-program
                else if (arg[1] == 'f' && len > 2)
                {
                    if (STRNEQ(arg, "-fno-", 5))
                    {
                        if (str == "-fno-rtti")
                        {
                            rtti = false;
                        }
                        else if (str == "-fno-threadsafe-statics")
                        {
                            threadsafe_statics = false;
                        }
                        else if (str == "-fno-inline")
                        {
                            cmd += " /Ob0";
                        }
                    }
                    else
                    {
                        if (str == "-fomit-frame-pointer")
                        {
                            cmd += " /Oy";
                        }
                        else if (str == "-fpermissive")
                        {
                            cmd += " /permissive";
                        }
                        else if (str == "-finline-functions")
                        {
                            cmd += " /Ob2";
                        }
                        else if (str == "-fopenmp")
                        {
                            cmd += " /openmp";
                        }
                        else if (str == "-fstack-protector" || str == "-fstack-check")
                        {
                            cmd += " /GS";
                        }
                        else if (str == "-funsigned-char")
                        {
                            cmd += " /J";
                        }
                        else if (str == "-fsized-deallocation")
                        {
                            cmd += " /Zc:sizedDealloc";
                        }
                        else if (STRNEQ(arg, "-fconstexpr-depth=", 18))
                        {
                            cmd += " /constexpr:depth" + STR(arg+18);
                        }
                        else if (STRNEQ(arg, "-ffp-contract=", 14))
                        {
                            if (STR(arg+14) == "fast")
                            {
                                cmd += " /fp:fast";
                            }
                            else if (STR(arg+14) == "off")
                            {
                                cmd += " /fp:strict";
                            }
                        }
                        else if (str == "-fwhole-program")
                        {
                            cmd += " /GL";
                        }
                    }
                }
                // -nostdinc -nostdinc++ -nostdlib -nodefaultlibs
                else if (arg[1] == 'n' && len > 8)
                {
                    if (str == "-nostdinc" || str == "-nostdinc++")
                    {
                        default_include_paths = false;
                    }
                    else if (str == "-nostdlib")
                    {
                        default_lib_paths = false;
                    }
                    else if (str == "-nodefaultlibs")
                    {
                        lnk += " /nodefaultlib";
                        default_lib_paths = false;
                    }
                }
                // -shared -std=c<..>|gnu<..>
                else if (arg[1] == 's' && len > 5)
                {
                    if (str == "-shared")
                    {
                        cmd += " /LD";
                    }
                    else if (STRNEQ(arg, "-std=", 5))
                    {
                        if (STRNEQ(arg, "-std=gnu", 8))
                        {
                            cmd += " /std:c" + STR(arg+8);
                        }
                        else
                        {
                            cmd += " /std:" + STR(arg+5);
                        }
                    }
                }
                // -include file
                else if (str == "-include")
                {
                    ++i;
                    if (i < argc)
                    {
                        cmd += " /FI'" + STR(argv[i]) + "'";
                    }
                }
                // -trigraphs
                else if (str == "-trigraphs")
                {
                    cmd += " /Zc:trigraphs";
                }
                // -print-search-dirs
                else if (str == "-print-search-dirs")
                {
                    std::cout << "cl.exe: " DEFAULT_CL_CMD "\n"
                        << "includes: " DEFAULT_INCLUDES "\n"
                        << "libraries: " DEFAULT_LIBPATHS << std::endl;
                    return 0;
                }
            }
        }
        else
        {
            cmd += " '" + str + "'";
        }
    }

    if (rtti)
    {
        cmd = " /GR" + cmd;
    }
    if (threadsafe_statics)
    {
        cmd = " /Zc:threadSafeInit" + cmd;
    }
    if (default_include_paths)
    {
        cmd += " " DEFAULT_INCLUDES;
    }

    if (do_link)
    {
        if (!have_outname)
        {
            lnk += " /out:'a.exe'";
        }
        if (default_lib_paths)
        {
            lnk += " " DEFAULT_LIBPATHS;
        }
        cmd += " /link" + lnk;
    }

    cmd = "'" + unix_path(cl_cmd) + "'" + cmd;

    if (verbose)
    {
        std::cout << cmd << std::endl;
    }
    if (print_only)
    {
        return 0;
    }

    return system(cmd.c_str());
}

