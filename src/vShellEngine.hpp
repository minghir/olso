#ifndef VSQLSHELLENGINE_HPP
#define VSQLSHELLENGINE_HPP
#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <cwctype>
#include <cctype>
#include <map>
#include <functional>
#include <variant>

#pragma once



#include "IShellEngine.hpp"
#include "vShellCommandParser.hpp"


#include <vector>
#include <memory>

struct vData; // Forward declaration

using vDataArray = std::vector<vData>;
//using vDataValue = std::variant<std::monostate, std::wstring, vDataArray>;

using vDataValue = std::variant<
    std::monostate,
    std::wstring,
    long long,
    double,
    bool,
    vDataArray
>;


struct vData {
    vDataValue value;

    // Utilitar pentru a verifica dacă este array sau string
    bool isArray() const { return std::holds_alternative<vDataArray>(value); }
};

using vValue = std::variant<std::monostate, std::wstring, int, double, long long, bool>;




class vShellEngine : public IShellEngine {

private:
//    IQueryProvider& m_provider;
    bool m_running;
    std::wstring m_accumulator;
    std::vector<std::wstring> m_history;
    //std::map<std::wstring, std::wstring> m_variables;
    std::map<std::wstring, vData> m_variables;

    using ShellHandler = std::function<void(const ShellCommand&)>;
    std::map<std::wstring, ShellHandler> m_commandHandlers;
    

    using ShellFunction = std::function<std::wstring(const std::vector<std::wstring>&)>;
    std::map<std::wstring, ShellFunction> m_functionHandlers;

public:
    

    //vShellEngine(IQueryProvider& provider);
    vShellEngine();

    std::wstring getPrompt() const override {
        return m_accumulator.empty() ? L"\noli# " : L"  -> ";
    }

    bool shouldExit() const override { return !m_running; }
    void stop() { m_running = false; }
    std::wstring vValueToString(const vValue& val);

    void executeShellCommand(const std::wstring& line);
    
    void initializeCommandsHandlers();
    void initializeFunctionsHandlers();

    std::vector<std::wstring> splitArguments(const std::wstring& s);
    std::wstring processArgument(std::wstring arg);


    void execute(const std::wstring& line);
    void handleInput(const std::wstring& line, std::wstring& accumulator);
    std::wstring normalizeVarName(std::wstring name);
    vDataValue getVarValue(std::wstring expression);
    void displayVariables();
    void displaySingleVariable(const std::wstring& varName);
    vData resolveExpression(std::wstring input);
    vData parseLiteral(const std::wstring& input, size_t& pos);

    void addToHistory(const std::wstring& query);
   
   std::wstring substituteVariables(std::wstring query, const std::map<std::wstring, vData>& vars);

   void executeScript(const std::wstring& filePath);
     

   std::wstring formatPrintR(const vDataValue& data, int indent);
   std::wstring vDataToPrintable(const vDataValue& data);
   void showHelp();
  
private:
    std::wstring stripQuotes(std::wstring s);
    vDataValue parseLiteralToValue(std::wstring str);

    //command handlers
    void handleSetCommand(const ShellCommand& sc);
    void handleUnsetCommand(const ShellCommand& sc);
    void handleUnsetAllCommand(const ShellCommand& sc);
    void handleEchoCommand(const ShellCommand& sc);
    void handleImportCommand(const ShellCommand& sc);
    void handleClearCommand(const ShellCommand& sc);
    void handleHistoryCommand(const ShellCommand& sc);
    void handleRunCommand(const ShellCommand& sc);
    void handlePauseCommand(const ShellCommand& sc);
    void handleForeachCommand(const ShellCommand& sc);
    void handleIfCommand(const ShellCommand& sc);
    void handleSysCommand(const ShellCommand& sc);
    void handleQuitCommand(const ShellCommand& sc);

    void handleEvalCommand(const ShellCommand& sc);
    void handlePrintRCommand(const ShellCommand& sc);

    // Function Handlers
    std::wstring fn_SUM(const std::vector<std::wstring>& args);
    std::wstring fn_FACT(const std::vector<std::wstring>& args);
    std::wstring fn_CONCAT(const std::vector<std::wstring>& args);
    std::wstring fn_UPPER(const std::vector<std::wstring>& args);
    std::wstring fn_LEN(const std::vector<std::wstring>& args);

    std::wstring fn_GET_VAR_VAL(const std::vector<std::wstring>& args);
    std::wstring fn_SET_VAR_VAL(const std::vector<std::wstring>& args);

    std::wstring fn_PUSH(const std::vector<std::wstring>& args);
    std::wstring fn_POP(const std::vector<std::wstring>& args);

};
#endif