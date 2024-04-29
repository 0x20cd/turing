#ifndef TUR_LOADER_HPP
#define TUR_LOADER_HPP
#include <QString>
#include "tur/emulator.hpp"

namespace tur
{
    class Loader
    {
    private:
        Emulator &m_emu;
    public:
        Loader(Emulator &emu);
        bool loadTable(QString source, bool preserveTape = true);
        void loadTape(QString input, int carPos = 0);
        QString readTape(bool trim = true) const;
    };
}

#endif // TUR_LOADER_HPP
