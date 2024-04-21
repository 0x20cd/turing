#include "tur/parser.hpp"
using namespace tur;
using namespace tur::parser;


static inline QList<Token>::const_iterator nextPeriod(QList<Token>::const_iterator it, QList<Token>::const_iterator end)
{
    while (it != end && it->type != Token::PERIOD) ++it;
    return it;
}


StateBlock::StateBlock(QList<Token>::const_iterator begin, QList<Token>::const_iterator end, const QSet<tur::id::name_t> &allnames)
{}


Alphabet::Alphabet(QList<Token>::const_iterator begin, QList<Token>::const_iterator end, QSet<tur::id::name_t> &allnames)
    : alph(0x80'00'00'00)
    , allnames(allnames)
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

    tur::id::IdRef ref;
    tur::id::SymDesc desc;

    enum {NONE, KW_NULL, DESC, REF} lval_type = NONE;

    if (it->type == Token::KW_NULL) {
        if (this->is_null_declared || this->is_null_requested)
            throw ParseError();

        this->is_null_declared = true;
        lval_type = KW_NULL;
    }
    else if (it->type == Token::ID) {
        tur::id::name_t name = it->value.toString();
        ++it;

        if (this->allnames.contains(name))
            throw ParseError();
        this->allnames.insert(name);

        auto shape_or_idx = getShapeOrIdx(it, end, desc.shape, ref.idx);

        switch (shape_or_idx) {
        case shapeOrIdx_e::NONE:
        case shapeOrIdx_e::SHAPE:
            lval_type = DESC;
            desc.name = name;

            break;

        case shapeOrIdx_e::IDX:
            lval_type = REF;
            ref.name = name;

            break;

        default:
            throw std::logic_error("Unknown shape_or_idx");
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


        switch (lval_type) {
        case DESC:
            {
            auto rval_lbound = it;
            while (it != end && it->type != Token::COMMA) ++it;
            auto rval_rbound = it;
            desc.value.parse(rval_lbound, rval_rbound);
            }

            this->alph_sym.insert(std::move(desc));

            break;

        case KW_NULL:
        case REF:
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

            break;

        default:
            throw std::logic_error("Unknown lval_type");
        }

        if (it != end) {
            if (it->type != Token::COMMA)
                throw ParseError();
            ++it;
        }
    }
    else throw ParseError();

    return true;
}


Alphabet::shapeOrIdx_e Alphabet::getShapeOrIdx(
    QList<Token>::const_iterator &it, QList<Token>::const_iterator end,
    tur::id::shape_t &shape, tur::id::idx_t &idx) const
{
    tur::id::indexeval_t index_eval;
    tur::id::IdxRangeEval range_eval;

    shapeOrIdx_e type = NONE;

    while (it != end && it->type == Token::BRACKET_L) {
        ++it;
        auto expr_lbound = it, expr_rbound = end, range_it = end;

        while (it != end) {
            if (it->type == Token::BRACKET_R) {
                expr_rbound = it;
                ++it;
                break;
            }

            if (it->type == Token::RANGE)
                range_it = it;

            ++it;
        }

        if (expr_rbound == end)
            throw ParseError();

        if (range_it != end) {
            if (type == IDX)
                throw ParseError();
            type = SHAPE;

            index_eval = tur::math::Expression::parse(expr_lbound, range_it);
            range_eval.setFirst(std::move(index_eval));

            ++range_it;
            index_eval = tur::math::Expression::parse(range_it, expr_rbound);
            range_eval.setLast(std::move(index_eval));

            shape.push_back(range_eval.eval(nullptr));
        }
        else {
            if (type == SHAPE)
                throw ParseError();
            type = IDX;

            index_eval = tur::math::Expression::parse(expr_lbound, expr_rbound);

            idx.push_back(index_eval->eval(nullptr));
        }
    }

    return type;
}


/*const Token& Alphabet::getAssignedValue(QList<Token>::const_iterator &it, QList<Token>::const_iterator end, bool allow_null)
{
    if (it == end)
        throw ParseError();

    switch(it->type) {
    case Token::KW_NULL:
        if (!allow_null)
            throw ParseError();
        this->is_null_requested = true;
        break;
    case Token::STRING:
        break;
    default:
        throw ParseError();
    }

    const Token& ret = *it;
    ++it;
    return ret;
}*/


States::States(QList<Token>::const_iterator begin, QList<Token>::const_iterator end, QSet<tur::id::name_t> &allnames)
    : states(2)
{}


Parser::Parser(QList<Token>::const_iterator begin, QList<Token>::const_iterator end)
{
    QList<Token>::const_iterator it = begin, it_period;

    while (it != end) {
        it_period = nextPeriod(it, end);
        if (it_period == end) throw ParseError();

        switch (it->type) {
        case Token::KW_A:
            if (alph) throw ParseError();

            ++it;
            this->alph = std::make_unique<Alphabet>(it, it_period, this->allnames);

            break;

        case Token::KW_Q:
            if (states) throw ParseError();

            ++it;
            this->states = std::make_unique<States>(it, it_period, this->allnames);

            break;

        default:
            this->blocks.emplace_back(it, it_period, this->allnames);
        }

        it = it_period;
        ++it;
    }
}
