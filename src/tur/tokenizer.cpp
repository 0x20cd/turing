#include "tur/tokenizer.hpp"
using namespace tur;

QList<Token> Tokenizer::tokenize(QString source)
{
    static const QHash<QString, Token::Type> KEYWORDS {
        {"A", Token::KW_A}, {"Q", Token::KW_Q}, {"null", Token::KW_NULL}, {"start", Token::KW_START},
        {"end", Token::KW_END}, {"N", Token::KW_N}, {"L", Token::KW_L}, {"R", Token::KW_R}
    };

    static const QHash<QChar, Token::Type> PUNCTS {
        {':', Token::COLON}, {';', Token::SEMICOLON}, {',', Token::COMMA}, {'.', Token::PERIOD},
        {'=', Token::ASSIGN}, {'&', Token::CAT}, {'_', Token::ANON}, {'|', Token::ITER},
        {'[', Token::BRACKET_L}, {']', Token::BRACKET_R}, {'{', Token::BRACE_L}, {'}', Token::BRACE_R},
        {'(', Token::PAR_L}, {')', Token::PAR_R}, {'+', Token::PLUS}, {'-', Token::MINUS},
        {'*', Token::MUL}, {'/', Token::DIV}, {'%', Token::MOD}, {'^', Token::POW}
    };

    QList<Token> chain;

    auto it = source.cbegin(), row_begin = source.cbegin(), tok_begin = source.cbegin();
    int row = 1;

    while (true)
    {
        while (it != source.cend() && it->isSpace()) {
            if (*(it++) == '\n') {
                row_begin = it;
                ++row;
            }
        }

        if (it == source.cend())
            break;

        tok_begin = it;
        SourceRef srcRef {
            .row = row,
            .col = tok_begin - row_begin
        };
        QStringView tail(&(*it), source.cend() - it);

        Token::Type type = Token::NONE;

        if (tail.startsWith(QLatin1StringView("->"))) {
            type = Token::ARROW;
            it += 2;
        } else if (tail.startsWith(QLatin1StringView(".."))) {
            type = Token::RANGE;
            it += 2;
        } else if (PUNCTS.contains(*it)) {
            type = PUNCTS.value(*it);
            ++it;
        }

        if (type != Token::NONE) {
            chain.push_back(Token {
                .type = type,
                .srcRef = srcRef
            });
            continue;
        }

        QString name = nextName(tail, it);
        if (!name.isNull()) {
            if (KEYWORDS.contains(name)) {
                chain.push_back(Token{
                    .type = KEYWORDS.value(name),
                    .srcRef = srcRef
                });
            } else {
                chain.push_back(Token{
                    .type = Token::ID,
                    .srcRef = srcRef,
                    .value = name,
                });
            }

            continue;
        }

        Token token = nextLiteral(tail, it);
        if (token.type != Token::NONE) {
            token.srcRef = srcRef;
            chain.push_back(token);
            continue;
        }

        throw TokenizerError{.srcRef = srcRef};
    }

    return chain;
}

QString Tokenizer::nextName(QStringView tail, QString::const_iterator &it)
{
    static QRegularExpression re(R"raw(^[a-zA-Z][_0-9a-zA-Z]*)raw");

    auto match = re.matchView(tail);
    if (match.hasMatch()) {
        auto ret = match.captured();
        it += ret.size();
        return ret;
    }

    return QString();
}

Token Tokenizer::nextLiteral(QStringView tail, QString::const_iterator &it)
{
    static QRegularExpression sym_re(R"raw((["'])|([^"'\n\\])|\\(["'\\])|\\u\{([0-9a-fA-F]{1,8})\})raw");
    static QRegularExpression num_re(R"raw(^[0-9]+)raw");

    Token token;
    QStringView view;
    bool ok;

    auto match = num_re.matchView(tail);
    if (match.hasMatch()) {
        view = match.capturedView();

        token.type = Token::NUMBER;
        token.value = view.toLongLong(&ok);

        if (!ok) throw TokenizerError();

        it += view.size();
        return token;
    }

    QString value;
    int skipCounter = 0;

    QChar del = tail.first();
    if (del != '"' && del != '\'')
        return Token{.type = Token::NONE};

    ++skipCounter;

    auto match_it = sym_re.globalMatchView(tail, 1);
    while (match_it.hasNext()) {
        match = match_it.next();

        skipCounter += match.capturedView().size();

        if (match.hasCaptured(1)) {
            view = match.capturedView(1);

            if (view.startsWith(del)) {
                token.type = Token::STRING;
                token.value = value;

                it += skipCounter;
                return token;
            }

            value.append(view);
        }
        else if (match.hasCaptured(2)) {
            view = match.capturedView(2);

            value.append(view);
        }
        else if (match.hasCaptured(3)) {
            view = match.capturedView(3);

            value.append(view);
        }
        else if (match.hasCaptured(4)) {
            view = match.capturedView();
            char32_t sym = view.toUInt(nullptr, 16);

            value.append(QString::fromUcs4(&sym, 1));
        }
        else {
            throw TokenizerError();
        }
    }

    throw TokenizerError(); // string literal was not terminated
}
