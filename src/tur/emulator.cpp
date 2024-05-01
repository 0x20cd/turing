#include <iterator>
#include "tur/emulator.hpp"
#include "tur/utils.hpp"
using namespace tur;
using namespace tur::emu;


Tape& Tape::operator=(Tape &&rhs)
{
    auto dist = std::distance(rhs.tape.begin(), rhs.car);

    this->tape = std::move(rhs.tape);
    this->car = this->tape.begin();
    std::advance(this->car, dist);

    return *this;
}


Emulator::Emulator()
{
    this->reset();
}


Emulator::Emulator(std::shared_ptr<tur::parser::Alphabet> alph, std::shared_ptr<tur::parser::States> states)
    : m_alph(alph)
    , m_states(states)
{
    this->init(alph, states);
}


void Emulator::init(std::shared_ptr<tur::parser::Alphabet> alph, std::shared_ptr<tur::parser::States> states)
{
    m_alph = alph;
    m_states = states;
    this->reset();
}


void Emulator::reset()
{
    m_tape.tape.clear();
    m_tape.tape.push_back(this->symnull());
    m_tape.car = m_tape.tape.begin();
    m_state = STATE_START;
}


bool Emulator::addRule(const Condition &cond, const Transition &tr)
{
    if (cond.state == STATE_END)
        throw std::invalid_argument("Final state is not a real state");

    quint64 key = std::bit_cast<quint64>(cond);

    if (m_table.contains(key))
        return false;

    m_table.insert(key, tr);
    return true;
}


void Emulator::moveCarriage(int diff)
{
    if (diff < 0)
        this->moveCarriage(Direction::Left, -diff);
    else
        this->moveCarriage(Direction::Right, diff);
}


void Emulator::moveCarriage(Direction dir, unsigned steps)
{
    switch (dir) {
    case Left:
        while (steps) {
            if (m_tape.car == m_tape.tape.begin())
                m_tape.tape.push_front(m_alph ? m_alph->null_value : 0);
            m_tape.car--;
            steps--;
        }
        break;
    case Right:
        while (steps) {
            if (m_tape.car == --m_tape.tape.end())
                m_tape.tape.push_back(m_alph ? m_alph->null_value : 0);
            m_tape.car++;
            steps--;
        }
        break;
    case None:
        break;
    default:
        std::logic_error("Invalid direction");
        break;
    }
}


void Emulator::step()
{
    if (m_state == STATE_END)
        throw std::logic_error("Final state is already reached");

    Condition cond {.state = m_state, .symbol = *m_tape.car};
    quint64 key = std::bit_cast<quint64>(cond);

    if (!m_table.contains(key)) {
        bool named;
        QString symbol_s = utils::symbolToString(cond.symbol, this->alph(), &named);

        throw NoRuleError{
            .symbol = named ? symbol_s : QString("'%1'").arg(symbol_s),
            .state = utils::stateToString(cond.state, this->states())
        };
    }

    auto &tr = m_table[key];

    *m_tape.car = tr.symbol;
    m_state = tr.state;

    this->moveCarriage(tr.direction);
}


quint32 Emulator::symnull() const
{
    if (m_alph)
        return m_alph->null_value;
    return 0;
}


quint32 Emulator::state() const
{
    return m_state;
}


const decltype(Emulator::m_tape.tape)& Emulator::tape() const
{
    return m_tape.tape;
}


const Transition* Emulator::getRule(const Condition &cond) const
{
    quint64 key = std::bit_cast<quint64>(cond);

    if (!m_table.contains(key))
        return nullptr;

    return &(*m_table.find(key));
}


const std::shared_ptr<tur::parser::Alphabet> Emulator::alph() const
{
    return m_alph;
}


const std::shared_ptr<tur::parser::States> Emulator::states() const
{
    return m_states;
}


decltype(Emulator::m_tape.tape.cbegin()) Emulator::carriage() const
{
    return m_tape.car;
}


int Emulator::carriagePos() const
{
    return std::distance<decltype(m_tape.tape.cbegin())>(m_tape.tape.cbegin(), m_tape.car);
}

