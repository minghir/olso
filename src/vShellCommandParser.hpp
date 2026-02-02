#ifndef vShellEngineCOMMANDPARSER_HPP
#define vShellEngineCOMMANDPARSER_HPP

#include <string>
#include <vector>
#include <sstream>

#include "vKeyWords.hpp"
#include "ConsoleManager.hpp"

struct ShellCommand {
    std::wstring name;               // Ex: L"/imp"
    std::vector<std::wstring> args;   // [L"dbf", L"C:\My Data\file.dbf", L"public.table"]
    bool isValid = false;
};

class vShellEngineCommandParser {
public:
    static ShellCommand parse(const std::wstring& line) {
        ShellCommand cmd;

        

        if (line.empty() || line[0] != L'/') return cmd;

        if (cmd.isValid) {
            if (!vKeyWords::isShellCommand(cmd.name)) {
                LOG_ERROR(L"Unknown command: " + cmd.name);
                cmd.isValid = false;
            }
        }
       

        std::wstring currentToken;
        bool inQuotes = false;

        for (size_t i = 0; i < line.length(); ++i) {
            wchar_t c = line[i];

            if (c == L'\"') {
                inQuotes = !inQuotes; // Toggle starea de ghilimele
                continue;
            }

            if (c == L' ' && !inQuotes) {
                if (!currentToken.empty()) {
                    if (cmd.name.empty()) cmd.name = currentToken;
                    else cmd.args.push_back(currentToken);
                    currentToken.clear();
                }
            }
            else {
                currentToken += c;
            }
        }

        // Adăugăm ultimul token dacă există
        if (!currentToken.empty()) {
            if (cmd.name.empty()) cmd.name = currentToken;
            else cmd.args.push_back(currentToken);
        }

        cmd.isValid = !cmd.name.empty();
        return cmd;
    }
};

#endif