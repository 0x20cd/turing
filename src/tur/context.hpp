#ifndef CONTEXT_HPP
#define CONTEXT_HPP
#include <QString>
#include "tur/common.hpp"

namespace tur::ctx
{
    struct Context {
        bool has(QString name) const;
        QHash<QString, number_t> vars;
        QSet<QString> other_names;
    };

    struct NameOccupiedError : CommonError {};

    class Variable {
    public:
        Variable(Context &context, SourceRef srcRef, QString name, number_t value = 0);
        Variable(const Variable&) = delete;
        Variable(Variable &&other);
        void operator=(const number_t &rhs);
        number_t value() const;
        ~Variable();

    private:
        Context &context;
        SourceRef srcRef;
        QString name;
    };
}

#endif // CONTEXT_HPP
