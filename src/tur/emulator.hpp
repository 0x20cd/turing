#ifndef TUR_EMULATOR_HPP
#define TUR_EMULATOR_HPP
#include <QtTypes>
#include <QHash>
#include <list>
#include <set>

namespace tur {
    class Loader;

    enum Direction {None = 'N', Left = 'L', Right = 'R'};

    struct Condition {
        quint32 state, symbol;
    };

    static_assert(sizeof(Condition) == sizeof(quint64));

    struct Transition {
        quint32 state, symbol;
        Direction direction;
    };

    constexpr quint32 STATE_START = 1;
    constexpr quint32 STATE_END = 0;

    class Emulator
    {
        friend class tur::Loader;
    private:
        QHash<quint64, Transition> m_table;
        std::list<quint32> m_tape;
        std::set<quint32> m_symbols;
        std::set<quint32> m_states;
        decltype(m_tape.begin()) m_car;
        quint32 m_state;
        quint32 m_symnull;

    public:
        explicit Emulator(quint32 symnull = 0);

        void reset();
        bool addRule(const Condition &cond, const Transition &tr);
        void step();

        void moveCarriage(Direction move);

        quint32 state() const;
        const decltype(m_tape)& tape() const;
        const decltype(m_table)& table() const;
        const decltype(m_symbols)& symbols() const;
        const decltype(m_states)& states() const;
        decltype(Emulator::m_tape.cbegin()) carriage() const;
    };
}

#endif // TUR_EMULATOR_HPP
