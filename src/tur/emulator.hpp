#ifndef TUR_EMULATOR_HPP
#define TUR_EMULATOR_HPP
#include <QtTypes>
#include <QHash>
#include <list>

namespace tur {
    enum Move {None = 'N', Left = 'L', Right = 'R'};

    struct Transition {
        quint32 symbol, state;
        Move move;
    };

    constexpr quint32 STATE_START = 1;
    constexpr quint32 STATE_END = 0;

    class Emulator
    {
    private:
        static inline quint64 cond(quint32 state, quint32 symbol);

        QHash<quint64, Transition> m_table;
        std::list<quint32> m_tape;
        decltype(m_tape.begin()) m_car;
        quint32 m_state;
        quint32 m_symnull;

    public:
        explicit Emulator(quint32 symnull = 0);

        void reset(quint32 symnull);
        void reset();
        bool addRule(quint32 state, quint32 symbol, const Transition &trans);
        void step();

        quint32 state();
        const decltype(m_tape)& tape();
        decltype(m_car) carriage();
    };
}

#endif // TUR_EMULATOR_HPP
