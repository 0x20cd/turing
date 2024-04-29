#ifndef TUR_PARSER_HPP
#define TUR_PARSER_HPP
#include <optional>
#include "tur/common.hpp"
#include "tur/idspace.hpp"

namespace tur::parser
{
    enum desctype_e {NONE, KW_NULL, KW_START, KW_END, KW_SAME, ITER, REF};

    class Rule {
    public:
        Rule(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);
    private:
        desctype_e refiter_type, symbol_type, state_type;
        tur::id::IdRefIterEval refiter;
        tur::id::IdRefEval symbol, state;
        tur::Direction dir;
    };

    class StateBlock {
    public:
        StateBlock(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);
    private:
        desctype_e refiter_type;
        tur::id::IdRefIterEval refiter;
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
}

namespace tur {
    class Parser {
    public:
        Parser(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);
    private:
        tur::ctx::Context context;
        std::unique_ptr<parser::Alphabet> alph;
        std::unique_ptr<parser::States> states;
        std::vector<parser::StateBlock> blocks;
    };
}

#endif // TUR_PARSER_HPP
