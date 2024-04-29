#include <exception>
#include <iterator>
#include "tur/loader.hpp"
#include "tur/parser.hpp"
#include "tur/tokenizer.hpp"
using namespace tur;


Loader::Loader(Emulator &emu)
    : m_emu(emu)
{}


bool Loader::loadTable(QString source, bool preserveTape)
{
    auto chain = Tokenizer::tokenize(source);
    Parser parser(chain.cbegin(), chain.cend());
    Emulator emu(parser.getAlph()->null_value);

    emu::Condition cond;
    emu::Transition tr;

    // TODO

    if (preserveTape)
        emu.m_tape = std::move(m_emu.m_tape);

    m_emu = std::move(emu);
    return true;
}


void Loader::loadTape(QString input, int carPos)
{
    auto input_list = input.toUcs4();
    decltype(m_emu.m_tape.tape) tape(input_list.begin(), input_list.end());

    if (tape.empty())
        tape.push_back(m_emu.m_symnull);

    m_emu.m_tape.tape = std::move(tape);

    m_emu.m_tape.car = carPos >= 0 ? m_emu.m_tape.tape.begin() : m_emu.m_tape.tape.end();
    std::advance(m_emu.m_tape.car, carPos);
}


QString Loader::readTape(bool trim) const
{
    if (m_emu.m_tape.tape.empty())
        return QString();

    auto begin = m_emu.m_tape.tape.begin(), end = m_emu.m_tape.tape.end();

    if (trim) {
        while (begin != end && *begin == m_emu.m_symnull) ++begin;
        if (begin == end)
            return QString();
        while (end != begin && *(--end) == m_emu.m_symnull);
        ++end;
    }

    QString output;

    while (begin != end)
        output.append(QString::fromUcs4((char32_t*)&(*begin++), 1));

    return output;
}
