#include <sstream>
#include "mathexpr.hpp"


const int Expression::NB_GROUPS = 3;


Number::Number(int64_t value, bool neg)
	: m_value(value)
{
    this->inv_sign(neg);
}


int64_t Number::eval(const IEvaluable::vars_t*)
{
	return this->m_value;
}

void Number::inv_sign(bool neg)
{
    if (neg) this->m_value *= -1;
}


Variable::Variable(const std::string &name, bool neg)
	: m_name(name)
	, m_neg(neg)
{}


int64_t Variable::eval(const IEvaluable::vars_t *vars)
{
    if (!vars)
        throw EvalError();

    auto it = vars->find(this->m_name);

    if (it == vars->end())
        throw EvalError();

	int64_t res = std::get<1>(*it);
    if (this->m_neg) res *= -1;

	return res;
}

void Variable::inv_sign(bool neg)
{
    this->m_neg = (this->m_neg != neg);
}


std::vector<Expression::Token> Expression::tokenize(std::string_view expr)
{
	Token token;

    static const std::unordered_map<char, Token::Type> TOKEN_DICT = {
		{ '(', Token::ParL  },
		{ ')', Token::ParR  },
		{ '^', Token::Pow   },
		{ '%', Token::Mod   },
		{ '/', Token::Div   },
		{ '*', Token::Mul   },
		{ '-', Token::Minus },
		{ '+', Token::Plus  }
	};

    std::vector<Token> seq;

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
                    throw ExprError{
                        .what = "Unexpected symbol",
                        .pos = std::distance(expr.begin(), it)
                    };
				token.type = Token::Var;
			} else if (std::isdigit(*it)) {
				if (token.type == Token::None)
					token.type = Token::Num;
			} else {
                if (token.type == Token::None)
                    throw ExprError{
                        .what = "Unexpected symbol",
                        .pos = std::distance(expr.begin(), it)
                    };
				break;
			}
		}

        std::string val(begin, it);
        std::istringstream ss;

		switch (token.type) {
		case Token::Var:
			token.value = val;
			break;
		case Token::Num:
			token.value = 0;
            ss.str(val);
			ss >> std::get<int64_t>(token.value);
            if (ss.fail())
                throw ExprError{
                    .what = "Failed to parse number",
                    .pos = std::distance(expr.begin(), it)
                };
			break;
		default:
			break;
		}

		seq.push_back(std::move(token));
	}

    return seq;
}


std::shared_ptr<IEvaluable> Expression::next_val(std::vector<Token>::iterator &it_ref, std::vector<Token>::iterator end, bool &neg_out)
{
    bool neg = false;
    auto it = it_ref;

    while (it != end) {
        switch (it->type) {
        case Token::Minus:
            neg = !neg;
        case Token::Plus:
            ++it;
            continue;
        default:
            break;
        }

        break;
    }

    if (it == end)
        throw ExprError{
            .what = "Unexpected end of expression"
        };

    std::shared_ptr<IEvaluable> ret;

    decltype(it) expr_begin;
    int lvl;

    switch (it->type) {
    case Token::ParL:
        expr_begin = it + 1;
        lvl = 1;

        while (lvl > 0 && ++it != end) {
                 if (it->type == Token::ParL) ++lvl;
            else if (it->type == Token::ParR) --lvl;
        }

        if (lvl > 0)
            throw ExprError{
                .what = "Unexpected end of expression"
            };

        ret = std::shared_ptr<Expression>(new Expression(expr_begin, it++, neg));
        neg = false;
        break;

    case Token::Num:
        ret = std::make_shared<Number>(std::get<int64_t>((it++)->value));
        break;

    case Token::Var:
        ret = std::make_shared<Variable>(std::get<std::string>((it++)->value));
        break;

    default:
        throw ExprError{
            .what = "Unexpected token"
        };
    }

    it_ref = it;
    neg_out = neg;
    return ret;
}


Expression::Operator Expression::next_op(std::vector<Token>::iterator &it_ref, std::vector<Token>::iterator end)
{
    if (it_ref == end)
        return Operator{ .type = Token::None, .group = -1 };

    Operator op;
    op.type = it_ref->type;

    switch(it_ref->type) {
    case Token::Plus:
    case Token::Minus:
        op.group = 0;
        break;
    case Token::Mul:
    case Token::Div:
    case Token::Mod:
        op.group = 1;
        break;
    case Token::Pow:
        op.group = 2;
        break;
    default:
        throw ExprError{
            .what = "Unexpected token",
            .pos = - std::distance(it_ref, end)
        };
    }

    ++it_ref;
    return op;
}


int64_t Expression::eval_op(int64_t lhs, Operator op, int64_t rhs)
{
    switch (op.type) {
    case Token::Plus:
        return lhs + rhs;

    case Token::Minus:
        return lhs - rhs;

    case Token::Mul:
        return lhs * rhs;

    case Token::Div:
        if (rhs == 0)
            throw EvalError();
        return lhs / rhs;

    case Token::Mod:
        if (rhs == 0)
            throw EvalError();
        return lhs % rhs;

    case Token::Pow:
        return intpow(lhs, rhs);

    default:
        throw std::logic_error("Invalid operator type");
    }
}


Expression::Expression()
    : m_right_assoc(false)
    , m_neg(false)
{}


Expression::Expression(std::string_view expr)
{
	std::vector<Token> seq = Expression::tokenize(expr);

	new(this) Expression(seq.begin(), seq.end());
}

// TODO
Expression::Expression(std::vector<Token>::iterator begin, std::vector<Token>::iterator end, bool neg)
{
    static const bool IS_RIGHT_ASSOC[NB_GROUPS] = {false, false, true};

    int g0 = -1, g, gt;
    std::shared_ptr<Expression> groups[NB_GROUPS];
    std::vector<int> stack;

    std::shared_ptr<IEvaluable> val;
    Operator op;
    bool neg_unary;

    for (int i = 0; i < NB_GROUPS; ++i) {
        groups[i] = std::shared_ptr<Expression>(new Expression());
        groups[i]->m_right_assoc = IS_RIGHT_ASSOC[i];
    }

    for (auto it = begin; it != end; g0 = g) {
        val = next_val(it, end, neg_unary);
        op = next_op(it, end);

        g = op.group;

        if (g0 < g) {
            stack.push_back(g0);
            groups[g] = std::shared_ptr<Expression>(new Expression());
            groups[g]->m_right_assoc = IS_RIGHT_ASSOC[g];

            if (g == 2) {
                groups[g]->inv_sign(neg_unary);
            } else {
                val->inv_sign(neg_unary);
            }
        }
        else if (g0 > g) {
            val->inv_sign(neg_unary);

            groups[g0]->m_values.push_back(val);
            val = groups[g0];
            while(!stack.empty() && (gt = stack.back()) > g) {
                stack.pop_back();
                groups[gt]->m_values.push_back(val);
                val = groups[gt];
            }

            if (stack.back() == g)
                stack.pop_back();
        }

        if (g >= 0) {
            groups[g]->m_values.push_back(val);
            groups[g]->m_ops.push_back(op);
        }
    }

    int i;
    for (i = 0; i < NB_GROUPS; ++i) {
        if (!groups[i]->m_ops.empty())
            break;
    }

    if (i < NB_GROUPS) {
        *this = std::move(*groups[i]);
    } else {
        this->m_values.push_back(val);
    }

    this->inv_sign(neg);
}


int64_t Expression::eval(const IEvaluable::vars_t *vars)
{
    if (m_values.size() != m_ops.size() + 1) {
        throw std::logic_error("Invalid Expression");
    }

	int64_t res, t;

    if (this->m_right_assoc) {
        auto it_val = m_values.rbegin();

        res = (*it_val++)->eval(vars);

        for (auto it_op = m_ops.rbegin(); it_op != m_ops.rend(); ++it_op, ++it_val)
            res = eval_op((*it_val)->eval(vars), *it_op, res);
    }
    else {
        auto it_val = m_values.begin();

        res = (*it_val++)->eval(vars);

        for (auto it_op = m_ops.begin(); it_op != m_ops.end(); ++it_op, ++it_val)
            res = eval_op(res, *it_op, (*it_val)->eval(vars));
    }

	if (this->m_neg)
        res *= -1;

    return res;
}


void Expression::inv_sign(bool neg)
{
    this->m_neg = (this->m_neg != neg);
}


int64_t intpow(int64_t a, int64_t b)
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

    if (b < 0)
        throw EvalError();

    if (a == 1 || b == 0)
        return 1;

    if (a == 0)
        return 0;

    if (b >= std::size(MAX_BASE))
        throw EvalError();

    if (a > MAX_BASE[b] || a < -MAX_BASE[b])
        throw EvalError();

	int64_t res = 1, part = a;

	for (unsigned bits = b; bits; bits >>= 1, part *= part)
		if (bits & 1)
			res *= part;

    return res;
}
