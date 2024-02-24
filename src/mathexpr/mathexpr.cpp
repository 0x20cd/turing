#include <sstream>
#include <cassert>
#include "mathexpr.hpp"


Number::Number(int64_t value)
	: m_value(value)
{}


int64_t Number::eval(const IEvaluable::vars_t&)
{
	return this->m_value;
}


Variable::Variable(const std::string &name, bool neg = false)
	: m_name(name)
	, m_neg(neg)
{}


int64_t Variable::eval(const IEvaluable::vars_t &vars)
{
	auto it = var.find(this->m_name);

	if (it == var.end())
		throw ExprError();

	int64_t res = std::get<1>(*it);
	if (this->m_neg) res = -res;

	return res;
}


/*Expr::Expr()
	: m_right_assoc(false)
	, m_neg(false)
{}*/


bool Expression::tokenize(std::string_view expr, std::vector<Token> &seq)
{
	Token token;

	static const std::map<char, Token::Type> TOKEN_DICT = {
		{ '(', Token::ParL  },
		{ ')', Token::ParR  },
		{ '^', Token::Pow   },
		{ '%', Token::Mod   },
		{ '/', Token::Div   },
		{ '*', Token::Mul   },
		{ '-', Token::Minus },
		{ '+', Token::Plus  }
	};

	seq.clear();

	for (auto it = expr.begin(); it != expr.end();)
	{
		if (std::isspace(*it)) {
			++it;
			continue;
		}

		auto t = TOKEN_DICT.find(*it);
		if (t != TOKEN_DICT.end()) {
			token.type = std::get<1>(*t);
			seq.push_back(token);
			++it;
			continue;
		}

		token.type = Token::None;
		auto begin = it;

		for (; it != expr.end(); ++it) {
			if (std::isalpha(*it)) {
				if (token.type == Token::Num)
					return false;
				token.type = Token::Var;
			} else if (std::isdigit(*it)) {
				if (token.type == Token::None)
					token.type = Token::Num;
			} else {
				if (token.type == Token::None)
					return false;
				break;
			}
		}

		assert(token.type != Token::None);
		std::string val(begin, it);

		switch (token.type) {
		case Token::Var:
			token.value = val;
			break;
		case Token::Num:
			token.value = 0;
			std::istringstream ss(val);
			ss >> std::get<int64_t>(token.value);
			if (ss.fail())
				return false;
			break;
		default:
			break;
		}

		seq.push_back(std::move(token));
	}

	return true;
}


bool Expression::next_val(std::vector<Token>::iterator &it, std::shared_ptr<IEvaluable> &out)
{
	return false; // TODO
}


bool Expression::next_op(std::vector<Token>::iterator &it, Operator &out)
{
	return false; // TODO
}


Expression::Expression(std::string_view expr)
{
	std::vector<Token> seq = Expression::tokenize(expr);

	new(this) Expression(seq.begin(), seq.end());
}

// TODO
Expression::Expression(std::vector<Token>::iterator begin, std::vector<Token>::iterator end)
{
	std::shared_ptr<IEvaluable> val;
	Operator op;

	

	for (auto it = begin; it != end, ++it) {
		if (!next_val(it, val) || !next_op(it, op))
			return false;
	}
}


bool Expression::eval(const IEvaluable::vars_t &vars, int64_t &out)
{
	if (m_values.empty())
		return false;

	assert(m_values.size() == m_ops.size() + 1);

	auto it_val    = this->m_right_assoc ? m_values.rbegin() : m_values.begin();
	auto ops_begin = this->m_right_assoc ?    m_ops.rbegin() :    m_ops.begin();
	auto ops_end   = this->m_right_assoc ?    m_ops.rend()   :    m_ops.end();
	int64_t res, t;

	if (!(*it_val++)->eval(vars, res))
		return false;

	for (auto it_op = ops_begin; it_op != ops_end; ++it_op) {
		if (!(*it_val)->eval(vars, t))
			return false;

		switch (*it_op) {
		case PLUS:
			res += t;
			break;

		case MINUS:
			res -= t;
			break;

		case MUL:
			res *= t;
			break;

		case DIV:
			if (t == 0)
				return false;
			res /= t;
			break;

		case MOD:
			if (t == 0)
				return false;
			res %= t;
			break;

		case POW:
			if (!intpow(res, t, res))
				return false;

		default:
			assert(false);
		}
	}

	if (this->m_neg)
		res = -res;

	out = res;
	return true;
}

bool intpow(int64_t a, int64_t b, int64_t &out)
{
	static int64_t MAX_BASE[63] = {
		  0,  0, 3037000499, 2097151, 55108, 6208, 1448, 511, 234,
		127, 78,         52,      38,    28,   22,   18,  15,  13,
		 11,  9,          8,       7,     7,    6,    6,   5,   5,
		  5,  4,          4,       4,     4,    3,    3,   3,   3,
		  3,  3,          3,       3,     2,    2,    2,   2,   2,
		  2,  2,          2,       2,     2,    2,    2,   2,   2,
		  2,  2,          2,       2,     2,    2,    2,   2,   2
	};

	if (a < 0 || b < 0)
		return false;

	if (a == 1 || b == 0) {
		out = 1;
		return true;
	}

	if (a == 0) {
		out = 0;
		return true;
	}

	if (b >= std::size(MAX_BASE))
		return false;

	if (a > MAX_BASE[b])
		return false;

	int64_t res = 1, part = a;

	for (unsigned bits = b; bits; bits >>= 1, part *= part)
		if (bits & 1)
			res *= part;

	out = res;
	return true;
}
