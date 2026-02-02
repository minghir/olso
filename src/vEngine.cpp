#include "vShell.hpp"
#include "vShellEngine.hpp"
#include "vApp.hpp"

#include<iostream>
#include <sstream>



class myApp : public vApp {
public:
    myApp(HINSTANCE hInstance) :vApp(hInstance) { setRunMode(RunMode::CONSOLE); };
    ~myApp() {};

    bool initConsole() override {
        

        
        vShellEngine shEngine;
        vShell shell(shEngine);
        shell.run(); // Această metodă blochează până la 'exit'
        return true;
    }
};
  


int main(int argc, char* argv[]) {
    // Obținem HINSTANCE pentru vApp (main nu îl primește ca argument)
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Inițializăm aplicația în modul Consola
    myApp app(hInstance);

    // ConsoleManager::initialize() va detecta că Subsystem e deja Console
    app.startConsole();

    // Rulăm logica (nCmdShow e de obicei SW_SHOW la consolă)
    return app.run(SW_SHOW);
}