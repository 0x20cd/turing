#ifndef TUR_EMULATOR_HPP
#define TUR_EMULATOR_HPP
#include <QtTypes>
#include <QHash>
#include <list>
#include <set>
#include <bit>
#include <memory>
#include "tur/common.hpp"
#include "tur/parser.hpp"

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
        std::shared_ptr<tur::parser::Alphabet> m_alph;
        std::shared_ptr<tur::parser::States> m_states;
        QHash<quint64, emu::Transition> m_table;
        emu::Tape m_tape;
        quint32 m_state;

    public:
        Emulator();
        explicit Emulator(std::shared_ptr<tur::parser::Alphabet> alph, std::shared_ptr<tur::parser::States> states);

        void init(std::shared_ptr<tur::parser::Alphabet> alph, std::shared_ptr<tur::parser::States> states);
        void step();
        void reset();
        bool addRule(const emu::Condition &cond, const emu::Transition &tr);

        quint32 symnull() const;
        quint32 state() const;
        const decltype(m_tape.tape)& tape() const;
        const emu::Transition* getRule(const emu::Condition &cond) const;
        const std::shared_ptr<tur::parser::Alphabet> alph() const;
        const std::shared_ptr<tur::parser::States> states() const;
        decltype(m_tape.tape.cbegin()) carriage() const;
        int carriagePos() const;
    };

    struct NoRuleError {
        QString symbol, state;
    };
}

#endif // TUR_EMULATOR_HPP
