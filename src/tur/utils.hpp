#ifndef UTILS_HPP
#define UTILS_HPP
#include "tur/parser.hpp"

namespace tur::utils
{
    QString symbolToString(quint32 sym, std::shared_ptr<tur::parser::Alphabet> alph, bool *is_named = nullptr);
    QString stateToString(quint32 state, std::shared_ptr<tur::parser::States> states);
}

#endif // UTILS_HPP
