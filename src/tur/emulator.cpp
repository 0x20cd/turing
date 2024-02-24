#include <iterator>
#include "tur/emulator.hpp"
using tur::Emulator;
using tur::NoRuleError;

Emulator::Emulator(quint32 symnull)
    : m_symnull(symnull)
{
    m_symbols.insert(symnull);
    m_states.insert(STATE_START);
    this->reset();
}


void Emulator::reset()
{
    m_tape.clear();
    m_tape.push_back(m_symnull);
    m_car = m_tape.begin();
    m_state = STATE_START;
}


bool Emulator::addRule(const Condition &cond, const Transition &tr)
{
    if (cond.state == STATE_END)
        throw std::invalid_argument("Final state is not a real state");

    const quint64 &key = *(quint64*)(&cond);

    if (m_table.contains(key))
        return false;

    m_symbols.insert(cond.symbol);
    m_symbols.insert(tr.symbol);

    m_states.insert(cond.state);
    if (tr.state != STATE_END)
        m_states.insert(tr.state);

    m_table.insert(key, tr);
    return true;
}


void Emulator::step()
{
    if (m_state == STATE_END)
        throw std::logic_error("Final state is already reached");

    Condition cond {.state = m_state, .symbol = *m_car};
    const quint64 &key = *(quint64*)(&cond);

    if (!m_table.contains(key))
        throw NoRuleError();

    auto &tr = m_table[key];

    *m_car = tr.symbol;
    m_state = tr.state;

    switch (tr.direction) {
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
    case None:
        break;
    default:
        std::logic_error("Invalid direction");
        break;
    }
}


quint32 Emulator::symnull() const
{
    return m_symnull;
}


quint32 Emulator::state() const
{
    return m_state;
}


const decltype(Emulator::m_tape)& Emulator::tape() const
{
    return m_tape;
}


const tur::Transition* Emulator::getRule(const Condition &cond) const
{
    const quint64 &key = *(quint64*)(&cond);
    if (!m_table.contains(key))
        return nullptr;

    return &(*m_table.find(key));
}


const decltype(Emulator::m_symbols)& Emulator::symbols() const
{
    return m_symbols;
}


const decltype(Emulator::m_states)& Emulator::states() const
{
    return m_states;
}


decltype(Emulator::m_tape.cbegin()) Emulator::carriage() const
{
    return m_car;
}


int Emulator::carriagePos() const
{
    return std::distance<decltype(m_tape.cbegin())>(m_tape.cbegin(), m_car);
}


NoRuleError::NoRuleError() : std::runtime_error("Behaviour is not defined for current state") {}
