#ifndef TUR_TOKENIZER_HPP
#define TUR_TOKENIZER_HPP
#include <QFile>
#include <QList>
#include <QRegularExpression>
#include "tur/common.hpp"

namespace tur {

    class Tokenizer
    {
    public:
        static QList<Token> tokenize(QString source);
    private:
        static QString nextName(QStringView tail, QString::const_iterator &it);
        static Token nextLiteral(QStringView tail, QString::const_iterator &it);
    };

    struct TokenizerError {
        SourceRef srcRef;
    };
}

#endif // TUR_TOKENIZER_HPP
