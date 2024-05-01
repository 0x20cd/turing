#include "tur/utils.hpp"
#include "tur/emulator.hpp"

namespace tur::utils
{
    QString symbolToString(quint32 sym, std::shared_ptr<tur::parser::Alphabet> alph, bool *is_named)
    {
        QString sym_str = "???";
        bool named = true;

        if (!(sym & 0x80000000)) {
            if (std::iswprint(sym)) {
                sym_str = QString::fromUcs4((char32_t*)&sym, 1);
                named = false;
            } else {
                sym_str = "U+" + QString::number(sym, 16);
            }
        }
        else if (alph) {
            auto ref = alph->alph.getRef(sym);

            sym_str = ref.name;
            for (auto index : ref.idx)
                sym_str += QString("[%1]").arg(index);
        }

        if (is_named)
            *is_named = named;

        return sym_str;
    }

    QString stateToString(quint32 state, std::shared_ptr<tur::parser::States> states)
    {
        QString state_str;

        switch (state) {
        case tur::emu::STATE_START:
            state_str = "start";
            break;
        case tur::emu::STATE_END:
            state_str = "end";
            break;
        default: {
            if (!states)
                return "???";

            auto ref = states->states.getRef(state);
            state_str = ref.name;
            for (auto index : ref.idx)
                state_str += QString("[%1]").arg(index);
        }}

        return state_str;
    }
}
