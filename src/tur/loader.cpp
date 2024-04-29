#include <exception>
#include <iterator>
#include "tur/loader.hpp"
#include "tur/parser.hpp"
#include "tur/tokenizer.hpp"
using namespace tur;


Loader::Loader(Emulator &emu)
    : m_emu(emu)
{}


void Loader::loadTable(QString source, bool preserveTape)
{
    auto chain = Tokenizer::tokenize(source);
    Parser parser(chain.cbegin(), chain.cend());
    Emulator emu(parser.alph, parser.states);

    emu::Condition cond;
    emu::Transition tr;

    for (auto &block : parser.blocks) {
        id::IdRefIter state_iter;
        id::IdRef state_ref;
        id::id_t state_id;

        switch (block.refiter_type) {
        case parser::ITER:
            state_ref.name = block.refiter.getName();

            state_iter = block.refiter.eval(
                parser.context,
                parser.states->states.getDesc(state_ref.name).shape
            );

            state_iter.value(state_ref.idx);

            state_id = parser.states->states.getId(state_ref);
            break;

        case parser::KW_START:
            state_id = emu::STATE_START;
            break;

        default:
            throw std::logic_error("");
        }

        do {
            if (state_id == emu::STATE_END) {
                continue;
            }

            for (auto &rule : block.rules) {
                id::IdRefIter sym_iter;
                id::IdRef sym_ref;
                id::id_t sym_id;

                switch (rule.refiter_type) {
                case parser::ITER:
                    sym_ref.name = rule.refiter.getName();

                    sym_iter = rule.refiter.eval(
                        parser.context,
                        parser.alph->getIdDesc(sym_ref.name).shape
                    );

                    sym_iter.value(sym_ref.idx);

                    sym_id = parser.alph->getId(sym_ref);
                    break;

                case parser::KW_NULL:
                    sym_id = emu.symnull();
                    break;

                default:
                    throw std::logic_error("");
                }

                do {
                    emu::Condition cond {.state = state_id, .symbol = sym_id};

                    if (emu.getRule(cond) != nullptr)
                        continue;

                    tur::emu::Transition tr {.direction = rule.dir};

                    switch (rule.symbol_type) {
                    case parser::REF:
                        tr.symbol = parser.alph->getId(rule.symbol.eval(&parser.context));
                        break;
                    case parser::KW_SAME:
                        tr.symbol = sym_id;
                        break;
                    case parser::KW_NULL:
                        tr.symbol = emu.symnull();
                        break;
                    default:
                        throw std::logic_error("");
                    }

                    switch (rule.state_type) {
                    case parser::REF:
                        tr.state = parser.states->states.getId(rule.state.eval(&parser.context));
                        break;
                    case parser::KW_SAME:
                        tr.state = state_id;
                        break;
                    case parser::KW_START:
                        tr.state = tur::emu::STATE_START;
                        break;
                    case parser::KW_END:
                        tr.state = tur::emu::STATE_END;
                        break;
                    default:
                        throw std::logic_error("");
                    }

                    emu.addRule(cond, tr);
                } while (
                    sym_iter.next()
                    && sym_iter.value(sym_ref.idx)
                    && (sym_id = parser.alph->getId(sym_ref), true)
                );
            }
        } while (
            state_iter.next()
            && state_iter.value(state_ref.idx)
            && (state_id = parser.states->states.getId(state_ref), true)
        );
    }

    if (preserveTape)
        emu.m_tape = std::move(this->m_emu.m_tape);

    this->m_emu = std::move(emu);
}


void Loader::loadTape(QString input, int carPos)
{
    auto input_list = input.toUcs4();
    decltype(m_emu.m_tape.tape) tape(input_list.begin(), input_list.end());

    if (tape.empty())
        tape.push_back(m_emu.symnull());

    m_emu.m_tape.tape = std::move(tape);

    m_emu.m_tape.car = carPos >= 0 ? m_emu.m_tape.tape.begin() : m_emu.m_tape.tape.end();
    std::advance(m_emu.m_tape.car, carPos);
}


QString Loader::readTape(bool trim) const
{
    if (m_emu.m_tape.tape.empty())
        return QString();

    auto begin = m_emu.m_tape.tape.begin(), end = m_emu.m_tape.tape.end();

    if (trim) {
        while (begin != end && *begin == m_emu.symnull()) ++begin;
        if (begin == end)
            return QString();
        while (end != begin && *(--end) == m_emu.symnull());
        ++end;
    }

    QString output;

    while (begin != end)
        output.append(QString::fromUcs4((char32_t*)&(*begin++), 1));

    return output;
}
