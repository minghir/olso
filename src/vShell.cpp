#include "stringUtils.hpp"
#include "vShell.hpp"
#include "ConsoleManager.hpp"

#include <cwctype>




    vShell::vShell(IShellEngine& engine) : m_engine(engine), m_running(true) {}

    /*
    void vShell::run() {
        LOG_SUCCESS(L"--- Shell Ready ---");
        std::wstring currentQuery;

        while (m_running) {
            std::wcout << (currentQuery.empty() ? L"\n# " : L"  -> ");

            std::wstring line;
            if (!std::getline(std::wcin, line)) break;

            // --- ADAUGĂ ASTA: Curățăm spațiile de la începutul liniei ---
            size_t first = line.find_first_not_of(L" \t\r\n");
            if (first == std::wstring::npos) continue; // Linie goală
            std::wstring trimmedLine = line.substr(first);

            // 1. Ieșire rapidă (folosim trimmedLine)
            if (trimmedLine == L"/exit" || trimmedLine == L"/quit" || trimmedLine == L"/q") {
                m_running = false;
                continue;
            }

            // 2. Comenzi administrative (Slash commands)
            // Verificăm pe trimmedLine pentru a ignora spațiile accidentale de la început
            if (trimmedLine[0] == L'/') {
                //m_engine.executeShellCommand(trimmedLine);
                m_engine.execute(trimmedLine);
                continue;
            }

            // 3. Acumulare Query SQL (aici folosim linia originală pentru a păstra formatarea dacă e cazul)
            currentQuery += line + L" ";

            if (line.find(L';') != std::wstring::npos) {
                m_engine.processQuery(currentQuery);
                currentQuery.clear();
            }
        }
        */

    void vShell::run() {
        LOG_SUCCESS(L"--- Shell Interface Started ---");

        while (!m_engine.shouldExit()) {
            std::wcout << m_engine.getPrompt();

            std::wstring line;
            if (!std::getline(std::wcin, line)) break;
            //line = normalizeSpaces(line);
            LOG_INFO(L"vShell::run("+line+L")");
            m_engine.execute(line);
        }
    }
    



