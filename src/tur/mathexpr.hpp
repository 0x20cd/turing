#ifndef MATHEXPR_HPP
#define MATHEXPR_HPP
#include <string>
#include <string_view>
#include <variant>
#include <unordered_map>
#include <cctype>
#include <cstdint>
#include <memory>
#include <iostream>
#include <QList>
#include <QString>
#include <boost/safe_numerics/safe_integer.hpp>
#include "tur/common.hpp"
#include "tur/context.hpp"

namespace tur::math
{
    struct EvalError : CommonError {};


    class IEvaluable
    {
    public:
        virtual number_t eval(const ctx::Context *vars = nullptr) = 0;
        virtual void inv_sign() = 0;
        virtual ~IEvaluable() = default;
    };


    class Number : public IEvaluable
    {
    public:
        Number(number_t value);
        number_t eval(const ctx::Context* = nullptr) override;
        void inv_sign() override;
    private:
        number_t value;
    };


    class Variable : public IEvaluable
    {
    public:
        Variable(const QString &name, bool is_neg = false);
        number_t eval(const ctx::Context *vars = nullptr) override;
        void inv_sign() override;
    private:
        QString name;
        bool is_neg;
    };


    class Expression : public IEvaluable
    {
        typedef number_t (*operator_t)(boost::safe_numerics::safe<number_t> a, boost::safe_numerics::safe<number_t> b);
    public:
        Expression(std::unique_ptr<IEvaluable> &&lhs, std::unique_ptr<IEvaluable> &&rhs, operator_t op, bool is_neg = false);
        number_t eval(const ctx::Context *vars = nullptr) override;
        void inv_sign() override;

        static std::unique_ptr<IEvaluable> parse(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);

        static number_t pow(boost::safe_numerics::safe<number_t> a, boost::safe_numerics::safe<number_t> b);
        static number_t mul(boost::safe_numerics::safe<number_t> a, boost::safe_numerics::safe<number_t> b);
        static number_t div(boost::safe_numerics::safe<number_t> a, boost::safe_numerics::safe<number_t> b);
        static number_t mod(boost::safe_numerics::safe<number_t> a, boost::safe_numerics::safe<number_t> b);
        static number_t add(boost::safe_numerics::safe<number_t> a, boost::safe_numerics::safe<number_t> b);
        static number_t sub(boost::safe_numerics::safe<number_t> a, boost::safe_numerics::safe<number_t> b);

    private:
        std::unique_ptr<IEvaluable> lhs, rhs;
        operator_t op_fn;
        bool is_neg;

        static std::unique_ptr<IEvaluable> next_val_add(QList<Token>::const_iterator &it, QList<Token>::const_iterator end);
        static std::unique_ptr<IEvaluable> next_val_mul(QList<Token>::const_iterator &it, QList<Token>::const_iterator end);
        static std::unique_ptr<IEvaluable> next_val(QList<Token>::const_iterator &it, QList<Token>::const_iterator end);
        static bool next_unary(QList<Token>::const_iterator &it, QList<Token>::const_iterator end);
    };


}

#endif//MATHEXPR_HPP
