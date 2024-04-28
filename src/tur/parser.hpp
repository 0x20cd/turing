#ifndef TUR_PARSER_HPP
#define TUR_PARSER_HPP
#include <optional>
#include "tur/common.hpp"
#include "tur/idspace.hpp"

namespace tur::parser
{
    class Rule {
    public:
        Rule(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);
    private:
        enum {NONE, KW_NULL, KW_START, KW_END, KW_SAME, ITER, REF}
            refiter_type, symbol_type, state_type;
        tur::id::IdRefIterEval refiter;
        tur::id::IdRefEval symbol, state;
        tur::Direction dir;
    };

    class StateBlock {
    public:
        StateBlock(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);
    private:
        tur::id::IdRefIterEval ref;
        std::vector<Rule> rules;
    };

    class Alphabet {
    public:
        Alphabet(QList<Token>::const_iterator begin, QList<Token>::const_iterator end, ctx::Context &context);
    private:
        bool addNextDeclaration(QList<Token>::const_iterator &it, QList<Token>::const_iterator end);

        ctx::Context &context;
        tur::id::IdSpace alph;
        tur::id::SymSpace alph_sym;
        tur::id::id_t null_value;
        bool is_null_declared, is_null_requested;
    };

    class States {
    public:
        States(QList<Token>::const_iterator begin, QList<Token>::const_iterator end, ctx::Context &context);
    private:
        bool addNextDeclaration(QList<Token>::const_iterator &it, QList<Token>::const_iterator end);

        ctx::Context &context;
        tur::id::IdSpace states;
    };

    class Parser {
    public:
        Parser(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);
    private:
        tur::ctx::Context context;
        std::unique_ptr<Alphabet> alph;
        std::unique_ptr<States> states;
        std::vector<StateBlock> blocks;
    };
}

#endif // TUR_PARSER_HPP
