#ifndef TUR_EMULATOR_HPP
#define TUR_EMULATOR_HPP
#include <QtTypes>
#include <QHash>
#include <list>
#include <set>
#include <bit>
#include "common.hpp"

namespace tur::emu {

    struct Condition {
        quint32 state, symbol;
    };

    static_assert(sizeof(Condition) == sizeof(quint64), "");

    struct Transition {
        quint32 state, symbol;
        Direction direction;
    };

    constexpr quint32 STATE_START = 1;
    constexpr quint32 STATE_END = 0;

    struct Tape {
        Tape() = default;
        Tape& operator=(Tape &&rhs);

        std::list<quint32> tape;
        decltype(tape.begin()) car;
    };
}

namespace tur {
    class Loader;

    class Emulator
    {
        friend class tur::Loader;
    private:
        QHash<quint64, emu::Transition> m_table;
        std::set<quint32> m_symbols;
        std::set<quint32> m_states;
        emu::Tape m_tape;
        quint32 m_state;
        quint32 m_symnull;

    public:
        explicit Emulator(quint32 symnull = 0);

        void step();
        void reset();
        bool addRule(const emu::Condition &cond, const emu::Transition &tr);

        quint32 symnull() const;
        quint32 state() const;
        const decltype(m_tape.tape)& tape() const;
        const emu::Transition* getRule(const emu::Condition &cond) const;
        const decltype(m_symbols)& symbols() const;
        const decltype(m_states)& states() const;
        decltype(m_tape.tape.cbegin()) carriage() const;
        int carriagePos() const;
    };

    class NoRuleError : std::runtime_error {
    public:
        NoRuleError();
    };
}

#endif // TUR_EMULATOR_HPP
