#ifndef VSHELL_HPP
#define VSHELL_HPP

#include "IShellEngine.hpp"


class vShell {
private:
    IShellEngine& m_engine; // Shell-ul folosește engine-ul
    bool m_running;

public:
    vShell(IShellEngine& engine);
    void run();

};

#endif