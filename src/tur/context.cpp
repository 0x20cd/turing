#include "context.hpp"
using namespace tur::ctx;

Variable::Variable(context_t &context, QString name, number_t value)
    : context(context)
{
    if (this->context.contains(name))
        throw VariableExistsError();

    this->context.insert(name, value);
    this->name = name;
}

void Variable::operator=(const number_t &rhs)
{
    this->context[this->name] = rhs;
}

Variable::~Variable()
{
    if (!this->name.isNull())
        this->context.remove(this->name);
}
