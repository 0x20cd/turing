#ifndef CONTEXT_HPP
#define CONTEXT_HPP
#include <QString>
#include "tur/common.hpp"

namespace tur::ctx
{
    using context_t = QHash<QString, number_t>;

    struct VariableExistsError {};

    class Variable {
    public:
        Variable(context_t &context, QString name, number_t value = 0);
        void operator=(const number_t &rhs);
        ~Variable();

    private:
        context_t &context;
        QString name;
    };
}

#endif // CONTEXT_HPP
