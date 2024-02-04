#include "tur/emulator.hpp"
using tur::Emulator;

Emulator::Emulator(quint32 symnull)
{
    this->reset(symnull);
}


void Emulator::reset(quint32 symnull)
{
    m_symnull = symnull;
    this->reset();
}


void Emulator::reset()
{
    m_tape.clear();
    m_tape.push_back(m_symnull);
    m_car = m_tape.begin();
    m_state = STATE_START;
}


bool Emulator::addRule(quint32 state, quint32 symbol, const Transition &trans)
{
    if (state == STATE_END)
        throw std::invalid_argument("Final state is not a real state");

    auto key = Emulator::cond(state, symbol);

    if (m_table.contains(key))
        return false;

    m_table.insert(key, trans);
    return true;
}


void Emulator::step()
{
    if (m_state == STATE_END)
        throw std::logic_error("Final state is already reached");

    auto key = Emulator::cond(m_state, *m_car);

    if (!m_table.contains(key))
        throw std::runtime_error("Behaviour is not defined for current state");

    auto &tr = m_table[key];

    *m_car = tr.symbol;
    m_state = tr.state;

    switch (tr.move) {
    case Left:
        if (m_car == m_tape.begin())
            m_tape.push_front(m_symnull);
        m_car--;
        break;
    case Right:
        if (m_car == --m_tape.end())
            m_tape.push_back(m_symnull);
        m_car++;
        break;
    default:
        break;
    }
}


quint32 Emulator::state()
{
    return m_state;
}


const decltype(Emulator::m_tape)& Emulator::tape()
{
    return m_tape;
}


decltype(Emulator::m_car) Emulator::carriage()
{
    return m_car;
}


inline quint64 Emulator::cond(quint32 state, quint32 symbol)
{
    return (quint64(state) << 32) | symbol;
}
