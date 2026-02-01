#ifndef VSQLSHELL_HPP
#define VSQLSHELL_HPP
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




#include "vShellCommandParser.hpp"

using vValue = std::variant<std::monostate, std::wstring, int, double, long long, bool>;

class vShell {
private:
//    IQueryProvider& m_provider;
    bool m_running;
    std::vector<std::wstring> m_history;
    std::map<std::wstring, std::wstring> m_variables;
    //QueryResult res;

    using ShellHandler = std::function<void(const ShellCommand&)>;
    std::map<std::wstring, ShellHandler> m_commandHandlers;
    
public:
    

    //vShell(IQueryProvider& provider);
    vShell();

    std::wstring vValueToString(const vValue& val);
   // void logQueryResult(const QueryResult& res);

    void executeShellCommand(const std::wstring& line);

    void initializeHandlers();

    

 
    
    void run();

   void handleInput(const std::wstring& line, std::wstring& accumulator);
    
   void addToHistory(const std::wstring& query);


   std::wstring substituteVariables(std::wstring query, const std::map<std::wstring, std::wstring>& vars);

   void processQuery(const std::wstring& query);

   void executeScript(const std::wstring& filePath);
  

   

   void showHelp();
   
   std::wstring normalizeVarName(std::wstring name);
   void assignFromQuery(const std::wstring& varName, std::wstring subQuery);
   void assignFromLastResult(const std::wstring& varName, const std::wstring& colName);

   void displayVariables();
   void displaySingleVariable(const std::wstring& varName);

   void handleSetCommand(const ShellCommand& sc);
   void handleRequeryCommand(const ShellCommand& sc);
   void handleEchoCommand(const ShellCommand& sc);
   void handleImportCommand(const ShellCommand& sc);
   void handleSaveCommand(const ShellCommand& sc);
   void handleInfoCommand(const ShellCommand& sc);
   void handleUnsetCommand(const ShellCommand& sc);
   void handleClearCommand(const ShellCommand& sc);
   void handleHistoryCommand(const ShellCommand& sc);
   void handleRunCommand(const ShellCommand& sc);
   void handlePauseCommand(const ShellCommand& sc);
   void handleLoadCommand(const ShellCommand& sc);
   void handleCsvCommand(const ShellCommand& sc);
   void handleForeachCommand(const ShellCommand& sc);
   void handleDescribeCommand(const ShellCommand& sc);
   void handleIfCommand(const ShellCommand& sc);
   void handleSysCommand(const ShellCommand& sc);

   void handleEvalCommand(const ShellCommand& sc);

   
};
#endif