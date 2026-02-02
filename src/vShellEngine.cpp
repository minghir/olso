#include <variant>

#include <filesystem>
#include <fstream>


#include "vShellEngine.hpp"
#include "vmath.hpp"
#include "ConsoleManager.hpp"
#include "stringUtils.hpp"

/*
void vShellEngine::execute(const std::wstring& line) {
    ShellCommand sc = vShellEngineCommandParser::parse(line);
    if (!sc.isValid) return;

    // REZOLVARE DINAMICĂ: Transformăm FACT(3) în 6 înainte de a ajunge la Handler
    for (auto& arg : sc.args) {
        arg = processArgument(arg);
    }

    auto it = m_commandHandlers.find(sc.name);
    if (it != m_commandHandlers.end()) {
        it->second(sc); // Acum /echo va primi direct "6" în args[0]
    }
    else {
        LOG_ERROR(L"Unknown command: " + sc.name);
    }
}
*/
void vShellEngine::execute(const std::wstring& line) {
    // Curățăm linia respectând regulile tale
    std::wstring cleanLine = normalizeSpaces(line);

    if (cleanLine.empty()) return;

    // Acum lucrăm doar cu cleanLine
    if (cleanLine[0] == L'/') {
        executeShellCommand(cleanLine);
    }
    else {
        // Pentru SQL, adunăm linia curățată
        m_accumulator += cleanLine + L" ";
        if (cleanLine.find(L';') != std::wstring::npos) {
            processQuery(m_accumulator);
            m_accumulator.clear();
        }
    }
}



std::wstring vShellEngine::vValueToString(const vValue& val) {
    return std::visit([](auto&& arg) -> std::wstring {
        using T = std::decay_t<decltype(arg)>;

        // 1. Gestionare NULL (monostate)
        // Returnăm un string gol sau "NULL" pentru a nu strica query-urile viitoare
        if constexpr (std::is_same_v<T, std::monostate>) {
            return L"";
        }
        // 2. String-uri (wstring)
        else if constexpr (std::is_same_v<T, std::wstring>) {
            return arg;
        }
        // 3. Numere întregi (int, long long, etc.)
        else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            return std::to_wstring(arg);
        }
        // 4. Numere cu virgulă (double, float)
        else if constexpr (std::is_floating_point_v<T>) {
            std::wstringstream ss;
            // setprecision(2) e bun pentru afișaj, dar dacă vrei precizie totală 
            // poți scoate std::fixed
            ss << std::fixed << std::setprecision(2) << arg;
            std::wstring s = ss.str();

            // Eliminăm zerourile inutile de la final (opțional)
            if (s.find(L'.') != std::wstring::npos) {
                s.erase(s.find_last_not_of(L'0') + 1, std::wstring::npos);
                if (s.back() == L'.') s.pop_back();
            }
            return s;
        }
        // 5. Boolean
        else if constexpr (std::is_same_v<T, bool>) {
            return arg ? L"1" : L"0"; // Returnăm 1/0 pentru compatibilitate SQL mai bună
        }
        // 6. Fallback
        else {
            return L"";
        }
        }, val);
}
/*
void vShellEngine::logQueryResult(const QueryResult& res) {
    if (!res.success) {
        LOG_ERROR(L"SQL Error: " + res.errorMessage);
        return;
    }

    if (res.data.empty()) {
        LOG_INFO(L"Query executed successfully. Empty set (0 rows).");
        return;
    }

    // 1. Calculăm lățimea necesară
    std::vector<size_t> colWidths;
    //for (const auto& col : res.columns) {
    for (const auto& col : res.aliases) {
        //colWidths.push_back(col.name.length());
        colWidths.push_back(col.length());
        
    }

    for (const auto& row : res.data) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i < colWidths.size()) {
                std::wstring valStr = vValueToString(row[i]);
                colWidths[i] = (std::max)(colWidths[i], valStr.length());
            }
        }
    }

    auto& console = ConsoleManager::getInstance();
    console.writeRaw(L"");

    // 2. Header
    std::wstring header = L"| ";
    for (size_t i = 0; i < res.columns.size(); ++i) {
        //header += res.columns[i].name;
        header += res.aliases[i];
        //header += std::wstring(colWidths[i] - res.columns[i].name.length(), L' ');
        header += std::wstring(colWidths[i] - res.aliases[i].length(), L' ');
        header += L" | ";
    }

    std::wstring separator(header.length(), L'-');
    console.writeRaw(separator, FOREGROUND_BLUE);
    console.writeRaw(header, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    console.writeRaw(separator, FOREGROUND_BLUE);

    // 3. Datele (cu aliniere numerică)
    for (const auto& row : res.data) {
        std::wstring rowStr = L"| ";
        for (size_t i = 0; i < row.size(); ++i) {
            std::wstring cellStr = vValueToString(row[i]);

            // Verificăm dacă este număr pentru a alinia la dreapta
            bool isNumeric = (res.columns[i].type == vDataType::Integer || res.columns[i].type == vDataType::Double);

            if (isNumeric && !std::holds_alternative<std::monostate>(row[i])) {
                // Aliniere la dreapta pentru numere
                rowStr += std::wstring(colWidths[i] - cellStr.length(), L' ') + cellStr;
            }
            else {
                // Aliniere la stânga pentru text și NULL
                rowStr += cellStr + std::wstring(colWidths[i] - cellStr.length(), L' ');
            }
            rowStr += L" | ";
        }
        console.writeRaw(rowStr);
    }

    console.writeRaw(separator, FOREGROUND_BLUE);

    // Footer cu timp de execuție
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(2) << res.executionTimeMs;
    LOG_INFO(L"Rows: " + std::to_wstring(res.rowCount) + L" (" + ss.str() + L" ms)");
}
*/
/*
vShellEngine::vShellEngine(IQueryProvider& provider) : m_provider(provider), m_running(true) {
        initializeHandlers();
    }
    */

vShellEngine::vShellEngine() : m_running(true) {
    initializeHandlers();
}

void vShellEngine::executeShellCommand(const std::wstring& line) {
    ShellCommand sc = vShellEngineCommandParser::parse(line);
    if (!sc.isValid) return;

    // REZOLVARE DINAMICĂ: Transformăm FACT(3) în 6 înainte de a ajunge la Handler
    for (auto& arg : sc.args) {
        arg = processArgument(arg);
    }

    auto it = m_commandHandlers.find(sc.name);
    if (it != m_commandHandlers.end()) {
        it->second(sc); // Acum /echo va primi direct "6" în args[0]
    }
    else {
        LOG_ERROR(L"Unknown command: " + sc.name);
    }
    }

void vShellEngine::initializeHandlers() {
        /*
        m_commandHandlers[L"/info"] = [this](const auto& sc) {
            if (sc.args.empty()) return LOG_ERROR(L"Usage: /info <path>");
            m_provider.inspectState(sc.args[0]);
            };
            */
        /*
        m_commandHandlers[L"/save"] = [this](const auto& sc) {
            if (sc.args.empty()) return LOG_ERROR(L"Usage: /save <path>");
            if (m_provider.saveState(sc.args[0])) LOG_SUCCESS(L"State saved.");
            else LOG_ERROR(L"Failed to save state.");
            };
        */
    m_commandHandlers[L"/quit"] = [this](const auto& sc) {
        handleQuitCommand(sc);
    };


        m_commandHandlers[L"/csv"] = [this](const auto& sc) {
            handleCsvCommand(sc);
            };

        m_commandHandlers[L"/set"] = [this](const auto& sc) {
            handleSetCommand(sc);
        };

        m_commandHandlers[L"/unset"] = [this](const auto& sc) {
            handleUnsetCommand(sc);
        };

        m_commandHandlers[L"/requery"] = [this](const auto& sc) {//requery
            handleRequeryCommand(sc);
            };

        m_commandHandlers[L"/echo"] = [this](const auto& sc) {//requery
            handleEchoCommand(sc);
            };

        m_commandHandlers[L"/load"] = [this](const auto& sc) {
            handleLoadCommand(sc);
            };

        m_commandHandlers[L"/save"] = [this](const auto& sc) {
            handleSaveCommand(sc);
            };

        m_commandHandlers[L"/info"] = [this](const auto& sc) {
            handleInfoCommand(sc);
            };

        m_commandHandlers[L"/clear"] = [this](const auto& sc) {
            handleClearCommand(sc);
            };

        m_commandHandlers[L"/history"] = [this](const auto& sc) {
            handleHistoryCommand(sc);
            };

        m_commandHandlers[L"/describe"] = [this](const auto& sc) {
            handleDescribeCommand(sc);
            };
        m_commandHandlers[L"/d"] = m_commandHandlers[L"/describe"];

        m_commandHandlers[L"/run"] = [this](const auto& sc) {
            handleRunCommand(sc);
            };

        m_commandHandlers[L"/r"] = m_commandHandlers[L"/run"];


        m_commandHandlers[L"/pause"] = [this](const auto& sc) {
            handlePauseCommand(sc);
            };

        m_commandHandlers[L"/sys"] = [this](const auto& sc) {
            handleSysCommand(sc);
            };

        m_commandHandlers[L"/foreach"] = [this](const auto& sc) {
            handleForeachCommand(sc);
            };

        m_commandHandlers[L"/if"] = [this](const auto& sc) {
            handleIfCommand(sc);
            };

        m_commandHandlers[L"/eval"] = [this](const auto& sc) {
            handleEvalCommand(sc);
            };


        m_commandHandlers[L"/imp"] = [this](const auto& sc) { handleImportCommand(sc); };
        m_commandHandlers[L"/import"] = [this](const auto& sc) { handleImportCommand(sc); };

        // Alias-uri
        m_commandHandlers[L"/h"] = [this](const auto& sc) { showHelp(); };
        m_commandHandlers[L"/help"] = m_commandHandlers[L"/h"];

        m_commandHandlers[L"/t"] = [this](const auto& sc) {
            //this->res = m_provider.getSchemaCatalog();
            //logQueryResult(this->res);
            };

        m_commandHandlers[L"/tables"] = m_commandHandlers[L"/t"];

        // Adaugă restul în același stil...


        m_functionHandlers[L"FACT"] = [this](auto args) -> std::wstring { // Specificăm tipul aici
            if (args.empty()) return L"0";

            try {
                double val = std::stod(args[0]);
                // Am scos (double) cast-ul de la factorial, rezultatul e deja double
                return std::to_wstring(factorial(val));
            }
            catch (...) {
                return L"NaN";
            }
            };

        m_functionHandlers[L"CONCAT"] = [](const std::vector<std::wstring>& args) -> std::wstring {
            std::wstring res;
            for (auto s : args) {
                if (s.size() >= 2 && s.front() == L'\"' && s.back() == L'\"')
                    s = s.substr(1, s.size() - 2);
                res += s;
            }
            return res;
            };

        m_functionHandlers[L"UPPER"] = [](auto args) -> std::wstring {
            if (args.empty()) return L"";
            std::wstring s = args[0];
            std::transform(s.begin(), s.end(), s.begin(), ::towupper);
            return s;
            };

    }

    std::wstring vShellEngine::processArgument(std::wstring arg) {
        // 1. Înlocuim variabilele mai întâi
        arg = substituteVariables(arg, m_variables);

        // 2. Căutăm funcția
        size_t openParen = arg.find(L'(');
        // Căutăm ultima paranteză pentru a închide corect funcția
        size_t closeParen = arg.find_last_of(L')');

        if (openParen != std::wstring::npos && closeParen != std::wstring::npos && openParen < closeParen) {
            std::wstring funcName = arg.substr(0, openParen);
            // Eliminăm eventuale spații de dinainte de paranteză, ex: FACT (3)
            funcName.erase(funcName.find_last_not_of(L" ") + 1);

            if (m_functionHandlers.count(funcName)) {
                std::wstring innerContent = arg.substr(openParen + 1, closeParen - openParen - 1);
                std::vector<std::wstring> parsedArgs;

                // Logică de split simplă după virgulă pentru parametri
                std::wstringstream ss(innerContent);
                std::wstring item;
                while (std::getline(ss, item, L',')) {
                    // Trim spații pentru fiecare argument
                    size_t first = item.find_first_not_of(L" ");
                    if (first != std::wstring::npos) {
                        size_t last = item.find_last_not_of(L" ");
                        item = item.substr(first, (last - first + 1));
                    }

                    // RECURSIVITATE: Permite funcții în interiorul funcțiilor
                    parsedArgs.push_back(processArgument(item));
                }

                return m_functionHandlers[funcName](parsedArgs);
            }
        }

        return arg;
    }


    void vShellEngine::handleCsvCommand(const ShellCommand& sc) {
        /*
        if (sc.args.size() < 2) {
            LOG_ERROR(L"Usage: /csv <table_name|last> <file_path>");
            return;
        }

        std::wstring source = sc.args[0];
        std::wstring filePath = sc.args[1];

        if (source == L"last") {
            if (res.success && !res.data.empty()) {
                if (vDataExporter::toCSV(res, filePath)) LOG_SUCCESS(L"Exported.");
                else LOG_ERROR(L"Export failed.");
            }
            else LOG_ERROR(L"No results to export.");
        }
        else {
            QueryResult fullTable = m_provider.execute(L"SELECT * FROM " + source);
            if (vDataExporter::toCSV(fullTable, filePath)) LOG_SUCCESS(L"Table exported.");
            else LOG_ERROR(L"Export failed.");
        }
        */
    }

    /*
    void vShellEngine::handleIfCommand(const ShellCommand& sc) {
        if (sc.args.size() < 3) {
            LOG_ERROR(L"Usage: /if <val1> <op> <val2> /then_cmd ... [/else /else_cmd ...]");
            return;
        }

        // 1. Extragem condiția (primele 3 argumente: $USER == admin)
        std::wstring val1 = substituteVariables(sc.args[0], m_variables);
        std::wstring op = sc.args[1];
        std::wstring val2 = substituteVariables(sc.args[2], m_variables);

        // 2. Evaluăm condiția
        bool conditionMet = false;
        if (op == L"==") conditionMet = (val1 == val2);
        else if (op == L"!=") conditionMet = (val1 != val2);
        else if (op == L">")  conditionMet = (std::stod(val1) > std::stod(val2)); // Atenție la conversii
        // ... restul operatorilor

        // 3. Identificăm ramurile /then și /else în restul argumentelor
        std::vector<std::wstring> thenBranch;
        std::vector<std::wstring> elseBranch;
        bool buildingElse = false;

        for (size_t i = 3; i < sc.args.size(); ++i) {
            if (sc.args[i] == L"/else") {
                buildingElse = true;
                continue;
            }

            if (buildingElse) elseBranch.push_back(sc.args[i]);
            else thenBranch.push_back(sc.args[i]);
        }

        // 4. Executăm ramura corespunzătoare
        const std::vector<std::wstring>& targetBranch = conditionMet ? thenBranch : elseBranch;

        if (!targetBranch.empty()) {
            // Reconstruim comanda din bucăți
            std::wstring finalCmd;
            for (const auto& s : targetBranch) finalCmd += s + L" ";

            // Trimitem spre execuție (poate fi SQL sau Shell)
            if (finalCmd[0] == L'/') executeShellCommand(finalCmd);
            else processQuery(finalCmd);
        }
    }
    */

    

    

    void vShellEngine::handleIfCommand(const ShellCommand& sc) {
        // Avem nevoie de: val1 (0), op (1), val2 (2) și măcar un token de comandă (3)
        if (sc.args.size() < 4) {
            LOG_ERROR(L"Usage: /if <v1> <op> <v2> <then_cmd...> [/else <else_cmd...>]");
            return;
        }

        // 1. Evaluăm condiția
        std::wstring v1 = substituteVariables(sc.args[0], m_variables);
        std::wstring op = sc.args[1];
        std::wstring v2 = substituteVariables(sc.args[2], m_variables);

        // Curățăm valorile (pentru spațiile din DB)
        auto trim = [](std::wstring s) {
            s.erase(0, s.find_first_not_of(L" "));
            size_t last = s.find_last_not_of(L" ");
            if (last != std::wstring::npos) s.erase(last + 1);
            return s;
            };
        v1 = trim(v1); v2 = trim(v2);

        bool conditionMet = false;
        if (op == L"==")      conditionMet = (v1 == v2);
        else if (op == L"!=") conditionMet = (v1 != v2);

        // 2. Separăm jetoanele (tokens) pentru ramura THEN și ELSE
        std::vector<std::wstring> thenTokens;
        std::vector<std::wstring> elseTokens;
        bool inElse = false;

        for (size_t i = 3; i < sc.args.size(); ++i) {
            if (sc.args[i] == L"/else") {
                inElse = true;
                continue;
            }
            if (inElse) elseTokens.push_back(sc.args[i]);
            else thenTokens.push_back(sc.args[i]);
        }

        // 3. Reconstruim comanda care a câștigat
        const auto& winnerTokens = conditionMet ? thenTokens : elseTokens;
        if (winnerTokens.empty()) return;

        std::wstring finalCmd;
        for (const auto& t : winnerTokens) finalCmd += t + L" ";

        // 4. Executăm rezultatul
        if (finalCmd[0] == L'/') executeShellCommand(finalCmd);
        else processQuery(finalCmd);
    }

    /*
    
        void vShellEngine::run() {
            LOG_SUCCESS(L"--- SQL Shell Ready ---");
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
                if (trimmedLine == L"exit" || trimmedLine == L"quit" || trimmedLine == L"/q") {
                    m_running = false;
                    continue;
                }

                // 2. Comenzi administrative (Slash commands)
                // Verificăm pe trimmedLine pentru a ignora spațiile accidentale de la început
                if (trimmedLine[0] == L'/') {
                    executeShellCommand(trimmedLine);
                    continue;
                }

                // 3. Acumulare Query SQL (aici folosim linia originală pentru a păstra formatarea dacă e cazul)
                currentQuery += line + L" ";

                if (line.find(L';') != std::wstring::npos) {
                    processQuery(currentQuery);
                    currentQuery.clear();
                }
            }
        }

        */
    void vShellEngine::handleInput(const std::wstring& line, std::wstring& accumulator) {
        // Dacă linia începe cu '/', este o comandă administrativă instantanee
        if (line[0] == L'/') {
            executeShellCommand(line);
            return;
        }

        // Altfel, acumulăm pentru SQL
        accumulator += line + L" ";
        if (line.find(L';') != std::wstring::npos) {
            //executeSqlQuery(accumulator);
            processQuery(accumulator);
            accumulator.clear();
        }
    }
    /*
    void executeShellCommand(const std::wstring& line) {
        // 1. Parsăm linia brută într-o structură de comandă
        ShellCommand sc = vShellEngineCommandParser::parse(line);

        // Dacă parserul a detectat erori de sintaxă (ex: ghilimele neînchise)
        if (!sc.isValid) return;

        // 2. Dispatcher de comenzi
        

        

        else if (sc.name == L"/load") {
            if (!sc.args.empty()) {
                if (m_provider.loadState(sc.args[0])) LOG_SUCCESS(L"State loaded.");
                else LOG_ERROR(L"Failed to load state.");
            }
            else LOG_ERROR(L"Usage: /load <path>");
        }

       

        else if (sc.name == L"/csv") {
            if (sc.args.size() >= 2) {
                std::wstring source = sc.args[0]; // Poate fi nume tabel sau "last"
                std::wstring filePath = sc.args[1];

                if (source == L"last") {
                    // Verificăm dacă avem ce să exportăm
                    if (this->res.success && !this->res.data.empty()) {
                        LOG_INFO(L"Exporting last query result to CSV...");
                        if (vDataExporter::toCSV(this->res, filePath)) {
                            LOG_SUCCESS(L"Last result exported successfully!");
                        }
                        else {
                            LOG_ERROR(L"Failed to write CSV file.");
                        }
                    }
                    else {
                        LOG_ERROR(L"No query results available to export. Run a query first.");
                    }
                }
                else {
                    // Logica ta veche: exportă un tabel întreg
                    LOG_INFO(L"Exporting table " + source + L" to CSV...");
                    QueryResult fullTable = m_provider.execute(L"SELECT * FROM " + source);
                    if (vDataExporter::toCSV(fullTable, filePath)) {
                        LOG_SUCCESS(L"Table exported successfully.");
                    }
                    else {
                        LOG_ERROR(L"Export failed.");
                    }
                }
            }
            else {
                LOG_ERROR(L"Usage: /csv <table_name|last> <file_path>");
            }
        }

        else if (sc.name == L"/d" || sc.name == L"/describe") {
            if (!sc.args.empty()) {
                this->res = m_provider.describeTable(sc.args[0]);
                logQueryResult(this->res);
            }
            else {
                LOG_ERROR(L"Usage: /d <table_name>");
            }
        }

        else if (sc.name == L"/tables" || sc.name == L"/t") {
            this->res = m_provider.getSchemaCatalog();
            logQueryResult(this->res);
        }

       

        else if (sc.name == L"/help" || sc.name == L"/h") {
            showHelp();
        }
       
        
        else if (sc.name == L"/run" || sc.name == L"/r") {
            if (!sc.args.empty()) {
                executeScript(sc.args[0]);
            }
            else {
                LOG_ERROR(L"Usage: /run <file.sql>");
            }
        }
        
        

       
        else if (sc.name == L"/echo") {
            if (sc.args.empty()) {
                ConsoleManager::getInstance().writeRaw(L""); // Linie goală pentru spațiere
            }
            else {
                // Unim toate argumentele într-un singur șir de caractere
                std::wstring rawMessage;
                for (size_t i = 0; i < sc.args.size(); ++i) {
                    rawMessage += sc.args[i] + (i < sc.args.size() - 1 ? L" " : L"");
                }

                // Înlocuim variabilele (ex: "Salut $USER" devine "Salut admin")
                std::wstring finalMessage = substituteVariables(rawMessage, this->m_variables);

                // Afișăm cu o culoare distinctă (Cyan deschis)
                ConsoleManager::getInstance().writeRaw(finalMessage, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            }
        }
        else if (sc.name == L"/pause") {
            std::wstring msg = L"Press Enter to continue...";
            if (!sc.args.empty()) {
                // Dacă utilizatorul a pus un mesaj custom: /pause Verifică datele de mai sus!
                msg.clear();
                for (const auto& arg : sc.args) msg += arg + L" ";
            }

            ConsoleManager::getInstance().writeRaw(msg, FOREGROUND_RED | FOREGROUND_INTENSITY);
            std::wstring dummy;
            std::getline(std::wcin, dummy); // Așteaptă input
            }
        else {
            LOG_ERROR(L"Unknown command: " + sc.name);
        }
    }
    */
void vShellEngine::addToHistory(const std::wstring& query) {
        m_history.push_back(query);
        // Salvare opțională în fișier
        std::wofstream historyFile("vSql_history.txt", std::ios::app);
        if (historyFile.is_open()) {
            historyFile << query << std::endl;
        }
    }


std::wstring vShellEngine::substituteVariables(std::wstring query, const std::map<std::wstring, std::wstring>& vars) {
    if (vars.empty()) return query;

    // 1. Înlocuire ${VARIABLE}
    // Sfat: Nu te baza pe name.substr(1). Mai bine iterezi prin vars 
    // și construiești cheia dinamic.
    for (const auto& [name, value] : vars) {
        std::wstring explicitKey = L"${" + (name[0] == L'$' ? name.substr(1) : name) + L"}";
        size_t pos = 0;
        while ((pos = query.find(explicitKey, pos)) != std::wstring::npos) {
            query.replace(pos, explicitKey.length(), value);
            pos += value.length();
        }
    }

    // 2. Înlocuire $VARIABLE (Sortate descrescător după lungime)
    std::vector<std::pair<std::wstring, std::wstring>> sortedVars(vars.begin(), vars.end());
    std::sort(sortedVars.begin(), sortedVars.end(), [](const auto& a, const auto& b) {
        return a.first.length() > b.first.length();
        });

    for (const auto& [name, value] : sortedVars) {
        size_t pos = 0;
        while ((pos = query.find(name, pos)) != std::wstring::npos) {
            size_t nextCharPos = pos + name.length();
            if (nextCharPos < query.length()) {
                wchar_t next = query[nextCharPos];

                // ELIMINĂ next == L'}' de aici! 
                // Dacă urmează '}', variabila s-a terminat, deci TREBUIE înlocuită.
                if (std::iswalnum(next) || next == L'_') {
                    pos = nextCharPos;
                    continue;
                }
            }
            query.replace(pos, name.length(), value);
            pos += value.length();
        }
    }
    return query;
}

    void vShellEngine::processQuery(const std::wstring& query) {
        /*
        if (query.find_first_not_of(L" \t\n\r") == std::wstring::npos) return;

        std::wstring interpolatedQuery = substituteVariables(query, this->m_variables);

        addToHistory(query);
        // Curățăm punctul și virgula pentru parser dacă e cazul
        std::wstring cleanQuery = interpolatedQuery;
        size_t semiPos = cleanQuery.find(L';');
        if (semiPos != std::wstring::npos) cleanQuery.erase(semiPos);

        try {
            this->res = m_provider.execute(cleanQuery);
            logQueryResult(res); // Folosim funcția ta existentă
        }
        catch (const std::exception& e) {
            std::string err(e.what());
            LOG_ERROR(L"Execution Error: " + std::wstring(err.begin(), err.end()));
        }
        catch (...) {
            LOG_ERROR(L"An unknown error occurred during execution.");
        }
        */
    }


    void vShellEngine::executeScript(const std::wstring& filePath) {

        std::wifstream file;
        file.open(std::filesystem::path(filePath));


        if (!file.is_open()) {
            LOG_ERROR(L"Could not open script file: " + filePath);
            return;
        }

        auto& console = ConsoleManager::getInstance();
        LOG_INFO(L"--- Starting Script: " + filePath + L" ---");

        std::wstring line;
        std::wstring batchQuery;    // Pentru SQL
        std::wstring shellAccumulator; // PENTRU COMENZI SHELL MULTI-LINIE
        int commandCount = 0;

        while (std::getline(file, line)) {
            // 1. Curățăm spațiile
            line.erase(0, line.find_first_not_of(L" \t\r\n"));
            line.erase(line.find_last_not_of(L" \t\r\n") + 1);

            // 2 & 3. Comentarii (Rămân neschimbate)
            if (line.empty() || line.find(L"--") == 0) continue;
            size_t commentPos = line.find(L"--");
            if (line[0] != L'/') {
                if (commentPos != std::wstring::npos) {
                    line = line.substr(0, commentPos);
                    line.erase(line.find_last_not_of(L" \t\r\n") + 1);
                    if (line.empty()) continue;
                }
            }

            // --- LOGICĂ NOUĂ PENTRU CONTINUARE LINIE ---
            bool hasContinuation = (!line.empty() && line.back() == L'\\');
            if (hasContinuation) {
                line.pop_back(); // Scoatem '\'
            }

            // 4. Gestionare SHELL (/)
            // Verificăm dacă suntem deja în curs de acumulare SAU dacă linia începe cu /
            if (!shellAccumulator.empty() || line[0] == L'/') {
                shellAccumulator += line + L" ";

                if (!hasContinuation) { // Dacă NU mai are \, înseamnă că s-a terminat comanda
                    commandCount++;
                   // console.writeRaw(L"\n[Script Cmd #" + std::to_wstring(commandCount) + L"]", FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                   // console.writeRaw(L"Shell> " + shellAccumulator, FOREGROUND_GREEN);

                    executeShellCommand(shellAccumulator);
                    shellAccumulator.clear();
                }
                continue;
            }

            // 5. Gestionare SQL
            batchQuery += line + L" ";
            if (!hasContinuation && line.find(L';') != std::wstring::npos) {
                commandCount++;
              //  console.writeRaw(L"\n[Script Cmd #" + std::to_wstring(commandCount) + L"]", FOREGROUND_BLUE | FOREGROUND_INTENSITY);

                // Folosim batchQuery direct în processQuery
                processQuery(batchQuery);
                batchQuery.clear();
            }
        }

        LOG_SUCCESS(L"--- Script Finished (" + std::to_wstring(commandCount) + L" commands) ---");
    }

    



    void vShellEngine::showHelp() {
        auto& console = ConsoleManager::getInstance();

        console.writeRaw(L"");
        console.writeRaw(L"=== vEngine SQL Shell Help ===", FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        console.writeRaw(L"Note: Standard SQL commands must end with ';'", FOREGROUND_RED | FOREGROUND_GREEN);
        console.writeRaw(L"----------------------------------------------------------------------", FOREGROUND_BLUE);

        struct HelpCmd { std::wstring cmd; std::wstring desc; };
        std::vector<HelpCmd> helpCmds = {
            { L"/h, /help",          L"Show this help screen." },
            { L"/t, /tables",        L"List all available schemas and tables." },
            { L"/d <table_name>",    L"Show table structure (Describe)." },
            { L"/requery",           L"Re-print the results of the last successful query." },
            { L"/csv <tbl> <pth>",   L"Export a specific table to CSV." },
            { L"/csv last <pth>",    L"Export the current results from memory to CSV." },
            { L"/imp <ty> <p> <t>",  L"Import data (ex: /imp dbf data.dbf public.airports)." },
            { L"/save <path>",       L"Save current engine state to file." },
            { L"/load <path>",       L"Load engine state from file." },
            { L"/info <path>",       L"Inspect a state file without loading it." },
            { L"/clear",             L"Clear the console screen." },
            { L"/sys <command>",     L"Run sys command." },
            { L"/history",           L"Show recent SQL queries." },
            { L"/r, /run <file>",    L"Execute an SQL script file." },
            { L"/set <$V=val>",      L"Set a variable (ex: /set $ID=10)." },
            { L"/set $V=`SELECT...`",L"Set variable from sub-query (backticks)." },
            { L"/set $V=[col_name]", L"Set variable from last result cell (brackets)." },
            { L"/unset <$V>",        L"Remove a variable." },
            { L"/echo <text|$V>",    L"Print text or variable values to console." },
            { L"/pause [msg]",       L"Wait for Enter key before continuing." },
            { L"/q, exit, quit",     L"Close the SQL shell." },
            { L"SELECT...",          L"Execute SQL (ex: SELECT * FROM aviation.flights;)" }
        };

        for (const auto& hc : helpCmds) {
            std::wstring line = L"  ";
            line += hc.cmd;

            // Am mărit padding-ul la 25 pentru a lăsa loc comenzii /csv cu argumente
            int padding = 25 - static_cast<int>(hc.cmd.length());
            if (padding > 0) line += std::wstring(padding, L' ');

            line += L" - " + hc.desc;
            console.writeRaw(line);
        }

        console.writeRaw(L"----------------------------------------------------------------------", FOREGROUND_BLUE);
        console.writeRaw(L"Examples:", FOREGROUND_GREEN);
        console.writeRaw(L"  SQL> SELECT * FROM aviation.pilots WHERE rank = 'Captain';");
        console.writeRaw(L"  SQL> /csv last C:\\exports\\my_result.csv");
        console.writeRaw(L"");
    }

    void vShellEngine::handleLoadCommand(const ShellCommand& sc) {
        /*
             if (!sc.args.empty()) {
                 if (m_provider.loadState(sc.args[0])) LOG_SUCCESS(L"State loaded.");
                 else LOG_ERROR(L"Failed to load state.");
             }
             else LOG_ERROR(L"Usage: /load <path>");
             */
    }
    /*
    void vShellEngine::handleSetCommand(const ShellCommand& sc) {
        // CAZ 1: /set (fără argumente) -> Afișăm tot
        if (sc.args.empty()) {
            displayVariables();
            return;
        }

        // Reconstruim linia pentru a trata corect spațiile (ex: /set $V = `SELECT 1`)
        std::wstring fullLine;
        for (size_t i = 0; i < sc.args.size(); ++i) {
            fullLine += sc.args[i] + (i < sc.args.size() - 1 ? L" " : L"");
        }

        size_t eqPos = fullLine.find(L'=');

        // CAZ 2: /set $VAR (fără egal) -> Afișăm valoarea curentă
        if (eqPos == std::wstring::npos) {
            displaySingleVariable(fullLine);
            return;
        }

        // CAZ 3: Atribuire
        std::wstring varName = normalizeVarName(fullLine.substr(0, eqPos));
        std::wstring varValue = fullLine.substr(eqPos + 1);

        // Eliminăm spațiile de la începutul valorii
        size_t firstValid = varValue.find_first_not_of(L" ");
        if (firstValid != std::wstring::npos) varValue = varValue.substr(firstValid);

        // Determinăm tipul de atribuire
        if (varValue.size() >= 2 && varValue.front() == L'`' && varValue.back() == L'`') {
           // assignFromQuery(varName, varValue.substr(1, varValue.size() - 2));
        }
        else if (varValue.size() >= 2 && varValue.front() == L'[' && varValue.back() == L']') {
            assignFromLastResult(varName, varValue.substr(1, varValue.size() - 2));
        }
        else {
            m_variables[varName] = varValue;
            LOG_SUCCESS(L"Variable " + varName + L" set to: " + varValue);
        }
    }
    */

    void vShellEngine::handleSetCommand(const ShellCommand& sc) {
        if (sc.args.empty()) {
            displayVariables();
            return;
        }

        // Reconstruim linia completă
        std::wstring fullLine;
        for (size_t i = 0; i < sc.args.size(); ++i) {
            fullLine += sc.args[i] + (i < sc.args.size() - 1 ? L" " : L"");
        }

        size_t eqPos = fullLine.find(L'=');
        if (eqPos == std::wstring::npos) {
            displaySingleVariable(fullLine);
            return;
        }

        std::wstring varName = normalizeVarName(fullLine.substr(0, eqPos));
        std::wstring varValue = fullLine.substr(eqPos + 1);

        // Trim spații
        varValue.erase(0, varValue.find_first_not_of(L" "));

        // Verificăm dacă valoarea este o sub-comandă între backticks
        if (!varValue.empty() && varValue.front() == L'`' && varValue.back() == L'`') {
            m_variables[varName] = resolveExpression(varValue);
            LOG_SUCCESS(L"Variable " + varName + L" set dynamically to: " + m_variables[varName]);
        }
        else if (!varValue.empty() && varValue.front() == L'[' && varValue.back() == L']') {
            assignFromLastResult(varName, varValue.substr(1, varValue.size() - 2));
        }
        else {
            // Atribuire simplă cu interpolare de variabile existente
            m_variables[varName] = substituteVariables(varValue, m_variables);
            LOG_SUCCESS(L"Variable " + varName + L" set to: " + m_variables[varName]);
        }
    }

    std::wstring vShellEngine::normalizeVarName(std::wstring name) {
        name.erase(name.find_last_not_of(L" ") + 1);
        name.erase(0, name.find_first_not_of(L" "));
        if (!name.empty() && name[0] != L'$') name = L"$" + name;
        return name;
    }

    void vShellEngine::assignFromQuery(const std::wstring& varName, std::wstring subQuery) {
        /*
        subQuery = substituteVariables(subQuery, m_variables);
        QueryResult subRes = m_provider.execute(subQuery);

        if (subRes.success && !subRes.data.empty() && !subRes.data[0].empty()) {
            m_variables[varName] = vValueToString(subRes.data[0][0]);
            LOG_SUCCESS(L"Variable " + varName + L" set via query.");
        }
        else {
            LOG_ERROR(L"Sub-query failed or returned no data.");
        }
        */
    }

    void vShellEngine::assignFromLastResult(const std::wstring& varName, const std::wstring& colName) {
        /*
        auto it = std::find(res.aliases.begin(), res.aliases.end(), colName);
        if (it != res.aliases.end() && !res.data.empty()) {
            size_t colIdx = std::distance(res.aliases.begin(), it);
            m_variables[varName] = vValueToString(res.data[0][colIdx]);
            LOG_SUCCESS(L"Variable " + varName + L" extracted from result.");
        }
        else {
            LOG_ERROR(L"Column '" + colName + L"' not found in last result.");
        }
        */
    }

    void vShellEngine::displayVariables() {
        if (m_variables.empty()) {
            LOG_INFO(L"No variables defined.");
        }
        else {
            ConsoleManager::getInstance().writeRaw(L"=== Defined Variables ===", FOREGROUND_GREEN);
            for (const auto& [name, val] : m_variables) {
                ConsoleManager::getInstance().writeRaw(name + L" = " + val);
            }
        }
    }

    void vShellEngine::displaySingleVariable(const std::wstring& varName) {
        std::wstring cleanName = normalizeVarName(varName);
        if (m_variables.count(cleanName)) {
            ConsoleManager::getInstance().writeRaw(cleanName + L" = " + m_variables[cleanName],
                FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        }
        else {
            LOG_ERROR(L"Variable " + cleanName + L" is not defined.");
        }
    }

    void vShellEngine::handleRequeryCommand(const ShellCommand& sc) {
        // CAZ 1: /requery <index> (Re-executare din istoric)
        if (!sc.args.empty()) {
            try {
                // Conversie argument string în index numeric (1-based pentru utilizator)
                int historyIndex = std::stoi(sc.args[0]);

                if (historyIndex > 0 && historyIndex <= (int)m_history.size()) {
                    // Extragem interogarea (ajustăm de la 1-based la 0-based index)
                    std::wstring queryToRepeat = m_history[historyIndex - 1];

                    LOG_INFO(L"Re-executing history item #" + std::to_wstring(historyIndex) + L": " + queryToRepeat);

                    // Trimitem interogarea către procesare (ca și cum ar fi fost scrisă acum)
                    processQuery(queryToRepeat);
                }
                else {
                    LOG_ERROR(L"History index out of range (1-" + std::to_wstring(m_history.size()) + L").");
                }
            }
            catch (const std::exception&) {
                LOG_ERROR(L"Invalid history index: " + sc.args[0]);
            }
            return;
        }

        // CAZ 2: /requery (Simplă reafisare a ultimului rezultat din RAM)
        /*
        if (this->res.success && !this->res.columns.empty()) {
            LOG_INFO(L"Re-printing last query results...");
            logQueryResult(this->res);
        }
        else {
            LOG_ERROR(L"No previous query results to display.");
        }
        */
    }
    

    void vShellEngine::handleEchoCommand(const ShellCommand& sc) {

        if (sc.args.empty()) {
            ConsoleManager::getInstance().writeRaw(L""); // Linie goală pentru spațiere
        }
        else {
            // Unim toate argumentele într-un singur șir de caractere
            std::wstring rawMessage;
            for (size_t i = 0; i < sc.args.size(); ++i) {
                rawMessage += sc.args[i] + (i < sc.args.size() - 1 ? L" " : L"");
            }

            // Înlocuim variabilele (ex: "Salut $USER" devine "Salut admin")
            std::wstring finalMessage = substituteVariables(rawMessage, this->m_variables);

            // Afișăm cu o culoare distinctă (Cyan deschis)
            ConsoleManager::getInstance().writeRaw(finalMessage, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        }
    }

    void vShellEngine::handleImportCommand(const ShellCommand& sc) {

     /*
         if (sc.args.size() >= 2) {
             std::wstring type = sc.args[0];
             std::wstring path = sc.args[1];
             std::wstring target = (sc.args.size() >= 3) ? sc.args[2] : L"";

             LOG_INFO(L"Importing " + type + L" from " + path + L"...");
             if (m_provider.importData(type, path, target)) {
                 LOG_SUCCESS(L"Import successful.");
             }
             else {
                 LOG_ERROR(L"Import failed.");
             }
         }
         else {
             LOG_ERROR(L"Usage: /imp <type> <path> [target]");
         }
         */
     }

    void vShellEngine::handleSaveCommand(const ShellCommand& sc) {
        /*
    
        if (!sc.args.empty()) {
            if (m_provider.saveState(sc.args[0])) LOG_SUCCESS(L"State saved.");
            else LOG_ERROR(L"Failed to save state.");
        }
        else LOG_ERROR(L"Usage: /save <path>");
        */
    }

    void vShellEngine::handleInfoCommand(const ShellCommand& sc) {
        /*
            if (!sc.args.empty()) m_provider.inspectState(sc.args[0]);
        else LOG_ERROR(L"Usage: /info <path>");
        */
    }

    void vShellEngine::handleUnsetCommand(const ShellCommand& sc) {
         
             if (!sc.args.empty()) {
                 std::wstring varName = sc.args[0];

                 // Ne asigurăm că verificăm și varianta cu $ în față
                 if (varName[0] != L'$') varName = L"$" + varName;

                 if (m_variables.erase(varName)) {
                     LOG_SUCCESS(L"Variable " + varName + L" has been removed.");
                 }
                 else {
                     LOG_ERROR(L"Variable " + varName + L" not found.");
                 }
             }
             else {
                 LOG_ERROR(L"Usage: /unset <variable_name>");
             }
         
    }
    
    void vShellEngine::handleClearCommand(const ShellCommand& sc) {
     
        ConsoleManager::getInstance().clear();
    }
    
    
    void vShellEngine::handleHistoryCommand(const ShellCommand& sc) {
    
        if (m_history.empty()) {
            LOG_INFO(L"Command history is empty.");
            return;
        }

        auto& console = ConsoleManager::getInstance();
        console.writeRaw(L"=== Command History ===", FOREGROUND_GREEN);
        for (size_t i = 0; i < m_history.size(); ++i) {
            std::wstring entry = std::to_wstring(i + 1) + L": " + m_history[i];
            console.writeRaw(entry);
        }
    }

    void vShellEngine::handleRunCommand(const ShellCommand& sc) {
    
        if (!sc.args.empty()) {
            executeScript(sc.args[0]);
        }
        else {
            LOG_ERROR(L"Usage: /run <file.sql>");
        }
    }


    void vShellEngine::handlePauseCommand(const ShellCommand& sc){
    
        std::wstring msg = L"Press Enter to continue...";
        if (!sc.args.empty()) {
            // Dacă utilizatorul a pus un mesaj custom: /pause Verifică datele de mai sus!
            msg.clear();
            for (const auto& arg : sc.args) msg += arg + L" ";
        }

        ConsoleManager::getInstance().writeRaw(msg, FOREGROUND_RED | FOREGROUND_INTENSITY);
        std::wstring dummy;
        std::getline(std::wcin, dummy); // Așteaptă input
    }


    /*
    void vShellEngine::handleForeachCommand(const ShellCommand& sc) {
    
        if (this->res.data.empty()) {
            LOG_ERROR(L"No data in last result to iterate over.");
            return;
        }

        // 1. Reconstruim tot ce urmează după /foreach într-un singur string
        std::wstring fullTemplate;
        for (const auto& arg : sc.args) fullTemplate += arg + L" ";

        // 2. Iterăm prin rândurile ultimului SELECT
        for (const auto& row : this->res.data) {
            std::map<std::wstring, std::wstring> rowContext = m_variables;
            for (size_t i = 0; i < this->res.aliases.size(); ++i) {
                std::wstring colVar = L"$" + this->res.aliases[i];
                std::wstring val = vValueToString(row[i]);

                // Trim pentru a scoate spațiile reziduale din DB (ex: "admin  " -> "admin")
                size_t last = val.find_last_not_of(L" ");
                if (last != std::wstring::npos) val.erase(last + 1);
                size_t first = val.find_first_not_of(L" ");
                if (first != std::wstring::npos) val.erase(0, first);

                rowContext[colVar] = val;
            }

            // 3. Aplicăm variabilele rândului peste template-ul complet
            std::wstring interpolatedLine = substituteVariables(fullTemplate, rowContext);

            // 4. SPARGEM comanda în sub-comenzi, dar cu protecție pentru /else
            std::vector<std::wstring> rawSegments;
            size_t pos = 0;
            while ((pos = interpolatedLine.find(L'/', pos)) != std::wstring::npos) {
                size_t nextPos = interpolatedLine.find(L'/', pos + 1);
                std::wstring segment = (nextPos == std::wstring::npos)
                    ? interpolatedLine.substr(pos)
                    : interpolatedLine.substr(pos, nextPos - pos);

                // Trim segmentului
                segment.erase(0, segment.find_first_not_of(L" "));
                segment.erase(segment.find_last_not_of(L" ") + 1);

                if (!segment.empty()) rawSegments.push_back(segment);
                if (nextPos == std::wstring::npos) break;
                pos = nextPos;
            }

            // 5. RE-UNIREA segmentelor dependente (ex: /else)
            std::vector<std::wstring> finalCommands;
            for (const auto& seg : rawSegments) {
                // Dacă segmentul curent este "/else ..." și avem o comandă anterioară, le lipim
                if (seg.find(L"/else") == 0 && !finalCommands.empty()) {
                    finalCommands.back() += L" " + seg;
                }
                else {
                    finalCommands.push_back(seg);
                }
            }

            // 6. EXECUȚIA finală
            for (const auto& cmd : finalCommands) {
                if (cmd[0] == L'/') {
                    executeShellCommand(cmd);
                }
                else {
                    processQuery(cmd);
                }
            }
        }
        }
*/
    void vShellEngine::handleForeachCommand(const ShellCommand& sc) {
        /*
        if (this->res.data.empty()) return;

        std::wstring fullTemplate;
        for (const auto& arg : sc.args) fullTemplate += arg + L" ";

        for (const auto& row : this->res.data) {
            std::map<std::wstring, std::wstring> rowContext = m_variables;
            for (size_t i = 0; i < this->res.aliases.size(); ++i) {
                std::wstring val = vValueToString(row[i]);
                // Trim obligatoriu pentru comparatii corecte
                val.erase(0, val.find_first_not_of(L" "));
                val.erase(val.find_last_not_of(L" ") + 1);
                rowContext[L"$" + this->res.aliases[i]] = val;
            }

            std::wstring interpolatedLine = substituteVariables(fullTemplate, rowContext);

            // --- NOUA LOGICĂ DE SPLIT INTELIGENT ---
            std::vector<std::wstring> finalCommands;
            size_t pos = 0;
            while ((pos = interpolatedLine.find(L'/', pos)) != std::wstring::npos) {
                size_t nextPos = interpolatedLine.find(L'/', pos + 1);

                // Verificăm dacă / urmat de ceva este de fapt un /else
                // Dacă da, "sărim" peste el și căutăm următorul /
                while (nextPos != std::wstring::npos) {
                    std::wstring peek = interpolatedLine.substr(nextPos, 6); // Verificăm dacă e "/else "
                    if (peek.find(L"/else") == 0) {
                        nextPos = interpolatedLine.find(L'/', nextPos + 1);
                    }
                    else {
                        break;
                    }
                }

                std::wstring cmd = (nextPos == std::wstring::npos)
                    ? interpolatedLine.substr(pos)
                    : interpolatedLine.substr(pos, nextPos - pos);

                finalCommands.push_back(cmd);
                if (nextPos == std::wstring::npos) break;
                pos = nextPos;
            }

            for (auto& cmd : finalCommands) {
                cmd.erase(0, cmd.find_first_not_of(L" "));
                cmd.erase(cmd.find_last_not_of(L" ") + 1);
                if (cmd.empty()) continue;

                if (cmd[0] == L'/') executeShellCommand(cmd);
                else processQuery(cmd);
            }
        }
        */
    }

    void vShellEngine::handleDescribeCommand(const ShellCommand& sc) {
     /*
            if (!sc.args.empty()) {
                this->res = m_provider.describeTable(sc.args[0]);
                logQueryResult(this->res);
            }
            else {
                LOG_ERROR(L"Usage: /d <table_name>");
            }
    */
    }

    void vShellEngine::handleSysCommand(const ShellCommand& sc) {
        if (sc.args.empty()) {
            LOG_ERROR(L"Usage: /sys <system_command>");
            return;
        }

        // 1. Reconstruim comanda și interpolăm variabilele shell-ului
        std::wstring fullCommand;
        for (const auto& arg : sc.args) fullCommand += arg + L" ";
        fullCommand = substituteVariables(fullCommand, m_variables);

        // Curățăm ghilimelele dacă utilizatorul a scris /sys "ls -l"
        if (fullCommand.front() == L'"' && fullCommand.back() == L'"') {
            fullCommand = fullCommand.substr(1, fullCommand.size() - 2);
        }

        LOG_INFO(L"Executing system command: " + fullCommand);

        // 2. Executăm și capturăm output-ul
        // Folosim _wpopen pentru a suporta caractere wide (Unicode)
        FILE* pipe = _wpopen(fullCommand.c_str(), L"rt");
        if (!pipe) {
            LOG_ERROR(L"Could not execute system command.");
            return;
        }

        wchar_t buffer[128];
        while (fgetws(buffer, 128, pipe)) {
            std::wcout << buffer;
        }

        int returnCode = _pclose(pipe);

        if (returnCode == 0) LOG_SUCCESS(L"Command finished.");
        else LOG_ERROR(L"Command failed with code: " + std::to_wstring(returnCode));

    }


    void vShellEngine::handleEvalCommand(const ShellCommand& sc) {
        if (sc.args.empty()) {
            LOG_ERROR(L"Usage: /eval <expression> (ex: /eval 2 * (3 + 4))");
            return;
        }

        // 1. Reconstruim expresia din argumente
        std::wstring wideExpr;
        for (const auto& arg : sc.args) wideExpr += arg;

        // 2. Înlocuim variabilele shell-ului (ex: ${ID} -> 1)
        wideExpr = substituteVariables(wideExpr, m_variables);

        // 3. Convertim din wstring (Unicode) în string (pentru funcțiile tale de calcul)
        std::string expr(wideExpr.begin(), wideExpr.end());

        try {
            // 4. Folosim motorul tău de calcul
            double result = evaluate_formula_fp(expr);

            // 5. Afișăm rezultatul
            // Dacă există o redirecționare setată, std::wcout va scrie în fișier automat!
            std::wcout << result << std::endl;

            // Opțional: salvăm rezultatul într-o variabilă specială pentru a fi folosit ulterior
            m_variables[L"$LAST_EVAL"] = std::to_wstring(result);
        }
        catch (const std::exception& e) {
            LOG_ERROR(L"Math Error: Expresie invalidă.");
        }

    }

    std::wstring vShellEngine::resolveExpression(std::wstring input) {
        if (input.size() < 2 || input.front() != L'`' || input.back() != L'`') {
            return substituteVariables(input, m_variables);
        }

        // Extragem comanda din interiorul backticks (ex: "/eval 2+2")
        std::wstring innerCmd = input.substr(1, input.size() - 2);

        // Pentru a captura output-ul comenzilor care în mod normal scriu la consolă (std::wcout),
        // putem folosi un buffer temporar sau stream redirection dacă vrei rezultate complexe.
        // Pentru /eval, e simplu deoarece avem acces la variabila $LAST_EVAL.

        if (innerCmd.find(L"/eval ") == 0) {
            executeShellCommand(innerCmd); // Aceasta va actualiza $LAST_EVAL
            return m_variables[L"$LAST_EVAL"];
        }

        if (innerCmd.find(L"/echo ") == 0) {
            return substituteVariables(innerCmd.substr(6), m_variables);
        }

        return input; // Returnăm neschimbat dacă nu știm să-l executăm
    }

    void vShellEngine::handleQuitCommand(const ShellCommand& sc) {
        stop();
    }