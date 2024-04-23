#ifndef TUR_PARSER_HPP
#define TUR_PARSER_HPP
#include <optional>
#include "tur/common.hpp"
#include "tur/idspace.hpp"

namespace tur::parser
{
    enum IdxOrShape_e {NONE, IDX, SHAPE};
    IdxOrShape_e getIdxOrShape(
        QList<Token>::const_iterator &it, QList<Token>::const_iterator end,
        tur::id::idx_t &idx, tur::id::shape_t *shape
    );

    class Rule {
    public:
        Rule(QList<Token>::const_iterator begin, QList<Token>::const_iterator end, const QSet<tur::id::name_t> &allnames);
    private:
        tur::id::IdRefIterEval ref;
        std::optional<tur::id::IdRefEval> symbol, state;
        tur::Direction dir;
    };

    class StateBlock {
    public:
        StateBlock(QList<Token>::const_iterator begin, QList<Token>::const_iterator end, const QSet<tur::id::name_t> &allnames);
    private:
        tur::id::IdRefIterEval ref;
        std::vector<Rule> rules;
    };

    class Alphabet {
    public:
        Alphabet(QList<Token>::const_iterator begin, QList<Token>::const_iterator end, QSet<tur::id::name_t> &allnames);
    private:
        bool addNextDeclaration(QList<Token>::const_iterator &it, QList<Token>::const_iterator end);

        tur::id::IdSpace alph;
        tur::id::SymSpace alph_sym;
        QSet<tur::id::name_t> &allnames;
        tur::id::id_t null_value;
        bool is_null_declared, is_null_requested;
    };

    class States {
    public:
        States(QList<Token>::const_iterator begin, QList<Token>::const_iterator end, QSet<tur::id::name_t> &allnames);
    private:
        bool addNextDeclaration(QList<Token>::const_iterator &it, QList<Token>::const_iterator end);

        tur::id::IdSpace states;
        QSet<tur::id::name_t> &allnames;
    };

    class Parser {
    public:
        Parser(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);
    private:
        QSet<tur::id::name_t> allnames;
        std::unique_ptr<Alphabet> alph;
        std::unique_ptr<States> states;
        std::vector<StateBlock> blocks;
    };
}

#endif // TUR_PARSER_HPP
