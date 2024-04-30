#include <cassert>
#include "context.hpp"
using namespace tur;
using namespace tur::ctx;

bool Context::has(QString name) const
{
    return this->vars.contains(name) || this->other_names.contains(name);
}

Variable::Variable(Context &context, SourceRef srcRef, QString name, number_t value)
    : context(context)
    , srcRef(srcRef)
{
    if (this->context.has(name)) {
        throw NameOccupiedError{ CommonError{
            .srcRef = this->srcRef,
            .msg = QObject::tr("Name '%1' is already occupied").arg(name)
        }};
    }

    this->context.vars.insert(name, value);
    this->name = name;
}

Variable::Variable(Variable &&other)
    : context(other.context)
    , srcRef(srcRef)
    , name(std::move(other.name))
{
    other.name.clear();
}

void Variable::operator=(const number_t &rhs)
{
    this->context.vars[this->name] = rhs;
}

number_t Variable::value() const
{
    return this->context.vars.value(this->name);
}

Variable::~Variable()
{
    if (!this->name.isNull())
        this->context.vars.remove(this->name);
}
