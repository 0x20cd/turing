#include <exception>
#include <iterator>
#include "tur/loader.hpp"
using tur::Loader;


Loader::Loader(Emulator &emu)
    : m_emu(emu)
{
    m_regex.setPattern(R"raw(^\s*([0-9]+)\s*,\s*(?:'([^'])'|"([^"])"|[uU]\+([0-9a-fA-F]{1,8}))\s*->\s*([0-9]+)\s*,\s*(?:'([^'])'|"([^"])"|[uU]\+([0-9a-fA-F]{1,8}))\s*,\s*([NLR])\s*$)raw");
    m_regex.optimize();
}


bool Loader::loadTable(QString desc)
{
    tur::Emulator emu;

    Condition cond;
    Transition tr;

    for (const QString& line : desc.split('\n')) {
        if (line.trimmed().isEmpty())
            continue;
        auto match = m_regex.match(line);
        if (!match.hasMatch())
            return false;

        cond.state = match.captured(1).toUInt();

        if (match.hasCaptured(4)) {
            cond.symbol = match.captured(4).toUInt(nullptr, 16);
        } else {
            const auto &sym = match.captured(2) + match.captured(3);
            cond.symbol = sym.toUcs4()[0];
        }

        tr.state = match.captured(5).toUInt();

        if (match.hasCaptured(8)) {
            tr.symbol = match.captured(8).toUInt(nullptr, 16);
        } else {
            const auto &sym = match.captured(6) + match.captured(7);
            tr.symbol = sym.toUcs4()[0];
        }

        tr.direction = static_cast<tur::Direction>(match.captured(9)[0].toLatin1());

        emu.addRule(cond, tr);
    }

    m_emu = std::move(emu);
    return true;
}


void Loader::loadTape(QString input, int carPos)
{
    auto input_list = input.toUcs4();
    decltype(m_emu.m_tape) tape(input_list.begin(), input_list.end());

    m_emu.m_tape = std::move(tape);

    m_emu.m_car = carPos >= 0 ? m_emu.m_tape.begin() : m_emu.m_tape.end();
    std::advance(m_emu.m_car, carPos);
}


QString Loader::readTape(bool trim) const
{
    if (m_emu.m_tape.empty())
        return QString();

    auto begin = m_emu.m_tape.begin(), end = m_emu.m_tape.end();

    if (trim) {
        while (*begin == m_emu.m_symnull) ++begin;
        while (*(--end) == m_emu.m_symnull);
        ++end;
    }

    QString output;

    while (begin != end)
        output.append(QString::fromUcs4((char32_t*)&(*begin++), 1));

    return output;
}
