#ifndef COMMON_HPP
#define COMMON_HPP
#include <QVariant>
#include <QChar>
#include <QString>

namespace tur
{
    using number_t = qint64;

    struct SourceRef {
        long row, col;
    };

    struct Token {
        enum Type {
            NONE, KW_A, KW_Q, KW_NULL, KW_START, KW_END, KW_N, KW_L, KW_R, KW_SAME,
            COLON, SEMICOLON, COMMA, PERIOD, ARROW, ASSIGN, RANGE, CAT, ANON, ITER,
            BRACKET_L, BRACKET_R, BRACE_L, BRACE_R, PAR_L, PAR_R,
            PLUS, MINUS, MUL, DIV, MOD, POW, ID, NUMBER, STRING
        } type;

        SourceRef srcRef;
        QVariant value;
    };

    enum Direction {None, Left, Right};

    struct ParseError {};
}

#endif // COMMON_HPP
