#pragma once

//declare the global variable (defined in ShellUtilities.cpp)
extern bool IS_BASH_BUILTIN_ERROR;

void report_shell_error(const char* command, const char* operand, int err_code);