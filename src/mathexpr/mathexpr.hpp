#ifndef MATHEXPR_HPP
#define MATHEXPR_HPP
#include <string>
#include <string_view>
#include <variant>
#include <unordered_map>
#include <cctype>
#include <vector>
#include <cstdint>
#include <memory>
#include <iostream>

struct ExprError {
    const char *what;
    int pos;
};
struct EvalError {};


class IEvaluable
{
public:
    using vars_t = std::unordered_map<std::string, int64_t>;
    virtual int64_t eval(const IEvaluable::vars_t *vars) = 0;
    virtual void inv_sign(bool neg) = 0;
};


class Number : public IEvaluable
{
public:
    Number(int64_t value, bool neg = false);
    int64_t eval(const IEvaluable::vars_t *vars) override;
    virtual void inv_sign(bool neg) override;
private:
	int64_t m_value;
};


class Variable : public IEvaluable
{
public:
	Variable(const std::string &name, bool neg = false);
    int64_t eval(const IEvaluable::vars_t *vars) override;
    virtual void inv_sign(bool neg) override;
private:
	std::string m_name;
	bool m_neg;
};


class Expression : public IEvaluable
{
public:
	Expression(std::string_view expr);
    int64_t eval(const IEvaluable::vars_t *vars) override;
    virtual void inv_sign(bool neg) override;

private:
	struct Token {
		enum Type { None, Var, Num, Plus, Minus, Mul, Div, Mod, Pow, ParL, ParR } type;
		std::variant<std::string, int64_t> value;
	};

	struct Operator {
		Token::Type type;
        int group;
    };

    static const int NB_GROUPS;

    static std::shared_ptr<IEvaluable> next_val(std::vector<Token>::iterator &it, std::vector<Token>::iterator end, bool &neg_out);
    static Operator next_op(std::vector<Token>::iterator &it, std::vector<Token>::iterator end);
    static std::vector<Token> tokenize(std::string_view expr);
    static int64_t eval_op(int64_t lhs, Operator op, int64_t rhs);

    Expression();
    Expression(std::vector<Token>::iterator begin, std::vector<Token>::iterator end, bool neg = false);

	std::vector<std::shared_ptr<IEvaluable>> m_values;
	std::vector<Operator> m_ops;
	bool m_neg, m_right_assoc;
};


int64_t intpow(int64_t a, int64_t b);


#endif//MATHEXPR_HPP
