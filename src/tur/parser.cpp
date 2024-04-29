#include "tur/emulator.hpp"
#include "tur/parser.hpp"
#include "tur/context.hpp"
using namespace tur;
using namespace parser;

enum IdxOrShape_e {NONE, IDX, SHAPE};

static IdxOrShape_e getIdxOrShape(
    const ctx::Context *ctx,
    QList<Token>::const_iterator &it, QList<Token>::const_iterator end,
    id::idx_t &idx, id::shape_t *shape)
{
    id::indexeval_t index_eval;
    id::IdxRangeEval range_eval;

    IdxOrShape_e type = IdxOrShape_e::NONE;

    while (it != end && it->type == Token::BRACKET_L) {
        ++it;
        auto expr_lbound = it, expr_rbound = end, range_it = end;

        while (it != end) {
            if (it->type == Token::BRACKET_R) {
                expr_rbound = it;
                ++it;
                break;
            }

            if (it->type == Token::RANGE) {
                if (shape == nullptr)
                    throw ParseError();
                range_it = it;
            }

            ++it;
        }

        if (expr_rbound == end)
            throw ParseError();

        if (range_it != end) {
            if (type == IDX)
                throw ParseError();
            type = SHAPE;

            index_eval = math::Expression::parse(expr_lbound, range_it);
            range_eval.setFirst(std::move(index_eval));

            ++range_it;
            index_eval = math::Expression::parse(range_it, expr_rbound);
            range_eval.setLast(std::move(index_eval));

            shape->push_back(range_eval.eval(ctx));
        }
        else {
            if (type == SHAPE)
                throw ParseError();
            type = IDX;

            index_eval = math::Expression::parse(expr_lbound, expr_rbound);

            idx.push_back(index_eval->eval(ctx));
        }
    }

    return type;
}


Rule::Rule(QList<Token>::const_iterator begin, QList<Token>::const_iterator end)
    : refiter_type(NONE)
    , symbol_type(NONE)
    , state_type(NONE)
{
    auto it = begin;

    if (it == end)
        throw ParseError();

    if (it->type == Token::KW_NULL) {
        this->refiter_type = KW_NULL;
        ++it;
    } else {
        this->refiter_type = ITER;

        auto lbound = it;
        auto rbound = nextToken(it, end, Token::ARROW);

        this->refiter.parse(lbound, rbound);

        it = rbound;
    }

    if (it == end || it->type != Token::ARROW)
        throw ParseError();
    ++it;

    if (it == end)
        throw ParseError();

    switch (it->type) {
    case Token::KW_NULL:
        this->symbol_type = KW_NULL;
        ++it;
        break;
    case Token::KW_SAME:
        this->symbol_type = KW_SAME;
        ++it;
        break;
    default: {
        this->symbol_type = REF;

        auto lbound = it;
        auto rbound = nextToken(it, end, Token::COMMA);

        this->symbol.parse(lbound, rbound);

        it = rbound;
    }}

    if (it == end || it->type != Token::COMMA)
        throw ParseError();
    ++it;

    if (it == end)
        throw ParseError();

    switch (it->type) {
    case Token::KW_N:
        this->dir = Direction::None;
        break;
    case Token::KW_L:
        this->dir = Direction::Left;
        break;
    case Token::KW_R:
        this->dir = Direction::Right;
        break;
    default:
        throw ParseError();
    }
    ++it;

    if (it == end || it->type != Token::COMMA)
        throw ParseError();
    ++it;

    if (it == end)
        throw ParseError();

    switch (it->type) {
    case Token::KW_START:
        this->state_type = KW_START;
        ++it;
        break;
    case Token::KW_END:
        this->state_type = KW_END;
        ++it;
        break;
    case Token::KW_SAME:
        this->state_type = KW_SAME;
        ++it;
        break;
    default: {
        this->state_type = REF;
        this->state.parse(it, end);
        it = end;
    }}

    if (it != end)
        throw ParseError();
}


StateBlock::StateBlock(QList<Token>::const_iterator begin, QList<Token>::const_iterator end)
{
    auto it = begin;

    if (it == end)
        throw ParseError();

    switch (it->type) {
    case Token::KW_START:
        this->refiter_type = KW_START;
        ++it;
        break;
    case Token::KW_END:
        this->refiter_type = KW_END;
        ++it;
        break;
    default: {
        this->refiter_type = ITER;
        auto rbound = nextToken(it, end, Token::COLON);
        this->refiter.parse(it, rbound);
        it = rbound;
        break;
    }}

    if (it == end || it->type != Token::COLON)
        throw ParseError();

    do {
        ++it;
        auto rbound = nextToken(it, end, Token::SEMICOLON);
        Rule rule(it, rbound);
        this->rules.push_back(std::move(rule));
        it = rbound;
    } while (it != end);
}


Alphabet::Alphabet(QList<Token>::const_iterator begin, QList<Token>::const_iterator end, ctx::Context &context)
    : context(context)
    , alph(0x80'00'00'00)
    , null_value(0)
    , is_null_declared(false)
    , is_null_requested(false)
{
    if (begin == end)
        throw ParseError();

    auto it = begin;
    while (this->addNextDeclaration(it, end));
}


bool Alphabet::addNextDeclaration(QList<Token>::const_iterator &it, QList<Token>::const_iterator end)
{
    if (it == end)
        return false;

    id::IdRef ref;
    id::SymDesc desc;

    enum {NONE, KW_NULL, DESC, REF} lval_type = NONE;

    if (it->type == Token::KW_NULL) {
        ++it;

        if (this->is_null_declared || this->is_null_requested)
            throw ParseError();

        this->is_null_declared = true;
        lval_type = KW_NULL;
    }
    else if (it->type == Token::ID) {
        id::name_t name = it->value.toString();
        ++it;

        auto idx_or_shape = getIdxOrShape(&this->context, it, end, ref.idx, &desc.shape);

        if (idx_or_shape == IdxOrShape_e::IDX) {
            lval_type = REF;
            ref.name = name;
        }
        else {
            if (this->context.has(name))
                throw ctx::NameOccupiedError();
            this->context.other_names.insert(name);

            lval_type = DESC;
            desc.name = name;
        }
    }
    else throw ParseError();

    if (it == end || it->type == Token::COMMA) {
        if (lval_type == REF)
            throw ParseError();

        if (lval_type == DESC)
            this->alph.push(desc);
    }
    else if (it->type == Token::ASSIGN) {
        ++it;

        if (it == end)
            throw ParseError();

        if (lval_type == DESC) {
            auto rval_lbound = it;
            auto rval_rbound = it = nextToken(it, end, Token::COMMA);
            desc.value.parse(rval_lbound, rval_rbound);

            this->alph_sym.insert(std::move(desc));
        } else {
            id_t value;

            if (it->type == Token::STRING) {
                auto value_s = it->value.toString().toUcs4();
                ++it;

                if (value_s.size() != 1)
                    throw ParseError();

                value = value_s.front();
            }
            else if (it->type == Token::KW_NULL && lval_type == REF) {
                ++it;
                value = this->null_value;
                this->is_null_requested = true;
            }
            else throw ParseError();

            if (lval_type == KW_NULL) {
                this->null_value = value;
            } else {
                this->alph.setAltId(ref, value);
            }
        }
    }
    else throw ParseError();

    if (it != end) {
        if (it->type != Token::COMMA)
            throw ParseError();
        ++it;
    }

    return true;
}


States::States(QList<Token>::const_iterator begin, QList<Token>::const_iterator end, ctx::Context &context)
    : context(context)
    , states(2)
{
    if (begin == end)
        throw ParseError();

    auto it = begin;
    while (this->addNextDeclaration(it, end));
}


bool States::addNextDeclaration(QList<Token>::const_iterator &it, QList<Token>::const_iterator end)
{
    if (it == end)
        return false;

    id::IdRef ref;
    id::IdDesc desc;

    enum {NONE, KW, DESC, REF} lval_type = NONE;

    if (it->type == Token::KW_START || it->type == Token::KW_END) {
        ++it;
        lval_type = KW;
    }
    else if (it->type == Token::ID) {
        id::name_t name = it->value.toString();
        ++it;

        auto idx_or_shape = getIdxOrShape(&this->context, it, end, ref.idx, &desc.shape);

        if (idx_or_shape == IdxOrShape_e::IDX) {
            lval_type = REF;
            ref.name = name;
        }
        else {
            if (this->context.has(name))
                throw ctx::NameOccupiedError();
            this->context.other_names.insert(name);

            lval_type = DESC;
            desc.name = name;
        }
    }
    else throw ParseError();

    if (it == end || it->type == Token::COMMA) {
        if (lval_type == REF)
            throw ParseError();

        if (lval_type == DESC)
            this->states.push(desc);
    }
    else if (it->type == Token::ASSIGN) {
        ++it;

        if (it == end)
            throw ParseError();

        if (lval_type != REF)
            throw ParseError();

        switch (it->type) {
        case Token::KW_START:
            this->states.setAltId(ref, emu::STATE_START);
            break;
        case Token::KW_END:
            this->states.setAltId(ref, emu::STATE_END);
            break;
        default:
            throw ParseError();
        }
        ++it;
    }
    else throw ParseError();

    if (it != end) {
        if (it->type != Token::COMMA)
            throw ParseError();
        ++it;
    }

    return true;
}


Parser::Parser(QList<Token>::const_iterator begin, QList<Token>::const_iterator end)
{
    QList<Token>::const_iterator it = begin, it_period;

    while (it != end) {
        it_period = nextToken(it, end, Token::PERIOD);
        if (it_period == end) throw ParseError();

        switch (it->type) {
        case Token::KW_A:
            if (alph) throw ParseError();

            ++it;
            if (it->type != Token::COLON)
                throw ParseError();
            ++it;

            this->alph = std::make_unique<Alphabet>(it, it_period, this->context);

            break;

        case Token::KW_Q:
            if (states) throw ParseError();

            ++it;
            if (it->type != Token::COLON)
                throw ParseError();
            ++it;

            this->states = std::make_unique<States>(it, it_period, this->context);

            break;

        default:
            StateBlock block(it, it_period);
            this->blocks.push_back(std::move(block));
        }

        it = it_period;
        ++it;
    }
}

const parser::Alphabet* Parser::getAlph() const
{
    return this->alph.get();
}

const parser::States* Parser::getStates() const
{
    return this->states.get();
}

const std::vector<parser::StateBlock>& Parser::getBlocks() const
{
    return this->blocks;
}
