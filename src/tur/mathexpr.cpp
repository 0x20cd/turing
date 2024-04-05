#include <sstream>
#include <tuple>
#include "tur/mathexpr.hpp"

using namespace tur;
using namespace tur::math;
using boost::safe_numerics::safe;


Number::Number(number_t value)
    : value(value)
{}


void Number::inv_sign()
{
    this->value = -this->value;
}


number_t Number::eval(const vars_t*)
{
    return this->value;
}


Variable::Variable(const QString &name, bool is_neg)
    : name(name)
    , is_neg(is_neg)
{}


void Variable::inv_sign()
{
    this->is_neg = !is_neg;
}


number_t Variable::eval(const vars_t *vars)
{
    if (!vars || !vars->contains(this->name))
        throw EvalError();

    try {
        safe<number_t> val = vars->at(this->name);
        return is_neg ? (-val) : val;
    } catch (const std::exception&) {
        throw EvalError();
    }
}


Expression::Expression(std::unique_ptr<IEvaluable> &&lhs, std::unique_ptr<IEvaluable> &&rhs, operator_t op, bool is_neg)
    : lhs(std::move(lhs))
    , rhs(std::move(rhs))
    , op_fn(op)
    , is_neg(is_neg)
{}


void Expression::inv_sign()
{
    this->is_neg = !is_neg;
}


number_t Expression::eval(const IEvaluable::vars_t *vars)
{
    try {
        safe<number_t> op_res = this->op_fn(this->lhs->eval(vars), this->rhs->eval(vars));
        return this->is_neg ? (-op_res) : op_res;
    } catch (const std::exception&) {
        throw EvalError();
    }
}


std::unique_ptr<IEvaluable> Expression::parse(QList<Token>::const_iterator begin, QList<Token>::const_iterator end)
{
    std::unique_ptr<IEvaluable> res = nullptr;
    operator_t op;

    auto it = begin;
    res = next_val_add(it, end);

    while (it != end)
    {
        switch (it->type) {
        case Token::PLUS:
            op = Expression::add;
            break;
        case Token::MINUS:
            op = Expression::sub;
            break;
        default:
            throw ExprError();
        }

        ++it;

        res = std::unique_ptr<IEvaluable>(new Expression(std::move(res), next_val_add(it, end), op));
    }

    return res;
}


std::unique_ptr<IEvaluable> Expression::next_val_add(QList<Token>::const_iterator &it, QList<Token>::const_iterator end)
{
    std::unique_ptr<IEvaluable> res = nullptr;
    operator_t op;

    res = next_val_mul(it, end);

    while (it != end)
    {
        switch (it->type) {
        case Token::MUL:
            op = Expression::mul;
            break;
        case Token::DIV:
            op = Expression::div;
            break;
        case Token::MOD:
            op = Expression::mod;
            break;
        default:
            return res;
        }

        ++it;

        res = std::unique_ptr<IEvaluable>(new Expression(std::move(res), next_val_mul(it, end), op));
    }

    return res;
}

std::unique_ptr<IEvaluable> Expression::next_val_mul(QList<Token>::const_iterator &it, QList<Token>::const_iterator end)
{
    std::vector<bool> signs;
    std::vector<std::unique_ptr<IEvaluable>> values;

    bool is_neg = next_unary(it, end);
    std::unique_ptr<IEvaluable> val = next_val(it, end);

    while (it != end)
    {
        if (it->type != Token::POW)
            break;
        ++it;

        signs.push_back(is_neg);
        values.push_back(std::move(val));

        is_neg = next_unary(it, end);
        val = next_val(it, end);
    }

    while (!values.empty()) {
        if (is_neg) val->inv_sign();

        is_neg = signs.back();
        val = std::unique_ptr<IEvaluable>(new Expression(std::move(values.back()), std::move(val), Expression::pow));

        signs.pop_back();
        values.pop_back();
    }

    if (is_neg) val->inv_sign();
    return val;
}

std::unique_ptr<IEvaluable> Expression::next_val(QList<Token>::const_iterator &it, QList<Token>::const_iterator end)
{
    std::unique_ptr<IEvaluable> res = nullptr;
    bool is_neg;

    is_neg = next_unary(it, end);

    QList<Token>::const_iterator subexpr_begin, subexpr_end;

    switch(it->type) {
    case Token::PAR_L:
        subexpr_begin = ++it;

        for (int par_lvl = 1; it != end; ++it) {
            if (it->type == Token::PAR_L) ++par_lvl;
            else if (it->type == Token::PAR_R) --par_lvl;

            if (par_lvl == 0)
                break;
        }

        if (it == end)
            throw ExprError();

        subexpr_end = it;
        res = Expression::parse(subexpr_begin, subexpr_end);
        ++it;
        break;
    case Token::NUMBER:
        res = std::unique_ptr<IEvaluable>(new Number(it->value.toLongLong()));
        ++it;
        break;
    case Token::ID:
        res = std::unique_ptr<IEvaluable>(new Variable(it->value.toString()));
        ++it;
        break;
    default:
        throw ExprError();
    }

    if (is_neg) res->inv_sign();
    return res;
}


bool Expression::next_unary(QList<Token>::const_iterator &it, QList<Token>::const_iterator end)
{
    bool is_neg = false;

    for (; it != end; ++it) {
        if (it->type == Token::PLUS)
            continue;

        if (it->type == Token::MINUS) {
            is_neg = !is_neg;
            continue;
        }

        break;
    }

    return is_neg;
}


number_t Expression::pow(safe<number_t> a, safe<number_t> b)
{
    if (b < 0) throw EvalError();

    if (b == 1) return a;
    if (b == 0 || a == 1) return 1;
    if (a == 0) return 0;
    if (a == -1) return (b % 2) ? -1 : 1;

    safe<number_t> res = 1, part = a;

    uintmax_t bits = b;
    while (true) {
        if (bits & 1)
            res *= part;
        bits >>= 1;

        if (!bits) break;

        part *= part;
    }

    return res;
}

number_t Expression::mul(safe<number_t> a, safe<number_t> b)
{
    return a * b;
}

number_t Expression::div(safe<number_t> a, safe<number_t> b)
{
    return a / b;
}

number_t Expression::mod(safe<number_t> a, safe<number_t> b)
{
    return a % b;
}

number_t Expression::add(safe<number_t> a, safe<number_t> b)
{
    return a + b;
}

number_t Expression::sub(safe<number_t> a, safe<number_t> b)
{
    return a - b;
}

