#ifndef CONSOLE_MANAGER_HPP
#define CONSOLE_MANAGER_HPP

#pragma once

#include <windows.h> // Pentru HWND, WORD, GetStdHandle, SetConsoleTextAttribute, AllocConsole etc.
#include <iostream>  // Pentru std::wcout, std::endl
#include <fstream>   // Nu este direct folosit aici, dar poate fi necesar pentru alte tipuri de logare
#include <string>    // Pentru std::wstring
#include <mutex>     // Pentru std::mutex (opțional, pentru thread-safety)


#define LOG_INFO(msg)         ConsoleManager::getInstance().log((msg),LogLevel::INFO)
#define LOG_SUCCESS(msg)      ConsoleManager::getInstance().log((msg),LogLevel::SUCCESS)
#define LOG_WARNING(msg)      ConsoleManager::getInstance().log((msg),LogLevel::WARNING)
#define LOG_ERROR(msg)        ConsoleManager::getInstance().log((msg),LogLevel::LOG_ERROR)
#define LOG_FATAL(msg)        ConsoleManager::getInstance().log((msg),LogLevel::FATAL_ERROR)
#define LOG_DEBUG(msg)        ConsoleManager::getInstance().log((msg),LogLevel::DEBUG)
#define LOG(msg)    ConsoleManager::getInstance().log((msg), LogLevel::INFO) 

enum class LogLevel {
    INFO,           // Informații generale (default)
        SUCCESS,        // Operație reușită (opțional, dar util)
        WARNING,        // Avertisment (nu e critic, dar necesită atenție)
        LOG_ERROR,      // Eroare (o problemă care nu oprește programul)
        FATAL_ERROR,    // Eroare critică (programul trebuie să se oprească sau este grav afectat)
        DEBUG           // Mesaje pentru depanare (folosite în timpul dezvoltării)
};

class ConsoleManager {
private:
    // Păstrează constructorul privat, dar șterge:
    // ConsoleManager() = delete;
    ConsoleManager() = default; // Acum este necesară o implementare
    ConsoleManager(const ConsoleManager&) = delete;
    ConsoleManager& operator=(const ConsoleManager&) = delete;

public:
    // METODA SINGLETON: Acesta este noul tău punct de acces
    static ConsoleManager& getInstance() {
        // Creează instanța la prima utilizare (thread-safe din C++11 încoace)
        static ConsoleManager instance;
        return instance;
    }

    // Toate metodele publice devin non-statice!
    void initialize();
    void setColor(WORD color);
    void resetColor();
    void log(const std::wstring& message, LogLevel level = LogLevel::INFO);
    void logTest();
    void shutdown();
    void clear();
    void writeRaw(const std::wstring& message, WORD color = 0);


private:
    // Mutex (dacă e necesar, scoate comentariul și pune-l la începutul clasei)
     std::mutex mtxLog; 
};



#endif // CONSOLE_MANAGER_HPP