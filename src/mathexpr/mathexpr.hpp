#ifndef MATHEXPR_HPP
#define MATHEXPR_HPP
#include <string>
#include <string_view>
#include <variant>
#include <map>
#include <cctype>
#include <vector>
#include <cstdint>
#include <memory>


struct ExprError {};


class IEvaluable
{
public:
	using vars_t = std::map<std::string, int64_t>;
	virtual int64_t eval(const IEvaluable::vars_t &vars) = 0;
};


class Number : public IEvaluable
{
public:
	Number(int64_t value);
	int64_t eval(const IEvaluable::vars_t &vars) override;
private:
	int64_t m_value;
};


class Variable : public IEvaluable
{
public:
	Variable(const std::string &name, bool neg = false);
	int64_t eval(const IEvaluable::vars_t &vars) override;
private:
	std::string m_name;
	bool m_neg;
};


class Expression : public IEvaluable
{
public:
	Expression(std::string_view expr);
	int64_t eval(const IEvaluable::vars_t &vars) override;

private:
	Expression(std::vector<Token>::iterator begin, std::vector<Token>::iterator end);

	struct Token {
		enum Type { None, Var, Num, Plus, Minus, Mul, Div, Mod, Pow, ParL, ParR } type;
		std::variant<std::string, int64_t> value;
	};

	struct Operator {
		Token::Type type;
		int group;
		bool right_assoc;
	}

	static bool next_val(std::vector<Token>::iterator &it, std::shared_ptr<IEvaluable> &out);
	static bool next_op(std::vector<Token>::iterator &it, Operator &out);
	static std::vector<Token> tokenize(std::string_view expr);

	std::vector<std::shared_ptr<IEvaluable>> m_values;
	std::vector<Operator> m_ops;
	bool m_neg, m_right_assoc;
};


bool intpow(int64_t a, int64_t b, int64_t &out);


#endif//MATHEXPR_HPP
