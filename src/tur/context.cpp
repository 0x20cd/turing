#include <cassert>
#include "context.hpp"
using namespace tur;
using namespace tur::ctx;

Variable::Variable(context_t &context, QString name, number_t value)
    : context(context)
{
    if (this->context.contains(name))
        throw VariableExistsError();

    this->context.insert(name, value);
    this->name = name;
}

Variable::Variable(Variable &&other)
    : context(other.context)
    , name(std::move(other.name))
{
    other.name.clear();
}

void Variable::operator=(const number_t &rhs)
{
    this->context[this->name] = rhs;
}

number_t Variable::value() const
{
    return this->context.value(this->name);
}

Variable::~Variable()
{
    if (!this->name.isNull())
        this->context.remove(this->name);
}
