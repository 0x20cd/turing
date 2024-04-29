#ifndef TUR_PARSER_HPP
#define TUR_PARSER_HPP
#include <optional>
#include "tur/common.hpp"
#include "tur/idspace.hpp"

namespace tur::parser
{
    enum desctype_e {NONE, KW_NULL, KW_START, KW_END, KW_SAME, ITER, REF};

    struct Rule {
        Rule(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);

        desctype_e refiter_type, symbol_type, state_type;
        tur::id::IdRefIterEval refiter;
        tur::id::IdRefEval symbol, state;
        tur::Direction dir;
    };

    struct StateBlock {
        StateBlock(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);

        desctype_e refiter_type;
        tur::id::IdRefIterEval refiter;
        std::vector<Rule> rules;
    };

    struct Alphabet {
        Alphabet(QList<Token>::const_iterator begin, QList<Token>::const_iterator end, ctx::Context &context);

        tur::id::IdDesc getIdDesc(tur::id::name_t name) const;
        tur::id::id_t getId(const tur::id::IdRef &ref) const;

        tur::id::IdSpace alph;
        tur::id::SymSpace alph_sym;
        tur::id::id_t null_value;

    private:
        bool addNextDeclaration(QList<Token>::const_iterator &it, QList<Token>::const_iterator end);

        ctx::Context &context;
        bool is_null_declared, is_null_requested;
    };

    struct States {
        States(QList<Token>::const_iterator begin, QList<Token>::const_iterator end, ctx::Context &context);

        tur::id::IdSpace states;

    private:
        bool addNextDeclaration(QList<Token>::const_iterator &it, QList<Token>::const_iterator end);

        ctx::Context &context;
    };
}

namespace tur {
    class Parser {
    public:
        Parser(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);

        tur::ctx::Context context;
        const parser::Alphabet* getAlph() const;
        const parser::States* getStates() const;
        const std::vector<parser::StateBlock>& getBlocks() const;

    private:
        std::unique_ptr<parser::Alphabet> alph;
        std::unique_ptr<parser::States> states;
        std::vector<parser::StateBlock> blocks;
    };
}

#endif // TUR_PARSER_HPP
