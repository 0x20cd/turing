#ifndef TUR_LOADER_HPP
#define TUR_LOADER_HPP
#include <QRegularExpression>
#include "tur/emulator.hpp"

namespace tur
{
    class Loader
    {
    private:
        Emulator &m_emu;
        QRegularExpression m_regex;
    public:
        Loader(Emulator &emu);
        bool loadTable(QString desc);
        void loadTape(QString input, int carPos = 0);
        QString readTape(bool trim = true) const;
    };
}

#endif // TUR_LOADER_HPP
