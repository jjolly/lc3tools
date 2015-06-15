#include <iostream>
#include <cstdarg>

#include "printer.h"

const char *Printer::RED    = "\033[31m";
const char *Printer::YELLOW = "\033[33m";
const char *Printer::GREEN  = "\033[32m";
const char *Printer::RESET  = "\033[0m";
const char *Printer::BOLD   = "\033[1m";

Printer& Printer::getInstance()
{
    // only one copy and guaranteed to be destroyed
    static Printer instance;

    return instance;
}

void Printer::printError(const char *format, ...)
{

    va_list args;
    va_start(args, format);
    printErrorVL(format, args);
    va_end(args);
}

void Printer::printWarning(const char *format, ...)
{

    va_list args;
    va_start(args, format);
    printWarningVL(format, args);
    va_end(args);
}

void Printer::printErrorVL(const char *format, va_list args)
{
    std::cout << BOLD << RED << "error: " << RESET << BOLD;
    std::vprintf(format, args);
    std::cout << RESET << std::endl;
}

void Printer::printWarningVL(const char *format, va_list args)
{
    std::cout << BOLD << YELLOW << "warning: " << RESET << BOLD;
    std::vprintf(format, args);
    std::cout << RESET << std::endl;
}

void Printer::printNChars(std::ostream &stream, char c, int count)
{
    for(int i = 0; i < count; i++) {
        stream << c;
    }
}