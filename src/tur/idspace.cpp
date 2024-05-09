#include <boost/safe_numerics/safe_integer.hpp>
#include "tur/idspace.hpp"
using namespace tur;
using namespace tur::id;
using boost::safe_numerics::safe;


template class Range<index_t>;
template class Range<sym_t>;


static bool idxToPos(idx_t idx, shape_t shape, uint64_t &pos_r)
{
    if (idx.size() != shape.size())
        return false;

    uint64_t pos = 0;

    for (qsizetype i = 0; i < idx.size(); ++i)
    {
        if (!shape[i].contains(idx[i]))
            return false;

        pos *= shape[i].size();
        pos += shape[i].pos(idx[i]);
    }

    pos_r = pos;
    return true;
}


//////////////////////////////////////////////////


template <typename T>
Range<T>::Range(T first, T last, SourceRef srcRef)
    : m_first(first)
    , m_last(last)
    , is_inv(first > last)
{
    m_size = 1 + (
        this->is_inv
        ? (uint64_t)(m_first) - (uint64_t)(m_last)
        : (uint64_t)(m_last) - (uint64_t)(m_first)
    );

    if (m_size == 0) { // (uint64_t)INT64_MAX - (uint64_t)INT64_MIN + 1 == 0
        throw IdInitError{ CommonError{
            .srcRef = srcRef,
            .msg = QObject::tr("Range is too large")
        }};
    }
}

template <typename T>
bool Range<T>::next(T &value) const
{
    if (value == this->m_last)
        return false;

    if (this->is_inv) --value; else ++value;
    return true;
}

template <typename T>
bool Range<T>::contains(T value) const
{
    return (
        this->is_inv
        ? m_first >= value && value >= m_last
        : m_first <= value && value <= m_last
    );
}

template <typename T>
uint64_t Range<T>::pos(T value) const
{
    return (
        this->is_inv
        ? m_first - value
        : value - m_first
    );
}

template <typename T>
uint64_t Range<T>::size() const
{
    return m_size;
}

template <typename T>
T Range<T>::operator[](uint64_t i) const
{
    return (
        this->is_inv
        ? m_first - i
        : m_first + i
    );
}

template <typename T>
T Range<T>::first() const
{
    return m_first;
}

template <typename T>
T Range<T>::last() const
{
    return m_last;
}


//////////////////////////////////////////////////


index_t IdxRangeCat::const_iterator::operator*() const
{
    return this->value;
}

IdxRangeCat::const_iterator& IdxRangeCat::const_iterator::operator++()
{
    if (!this->ranges_it->next(this->value)) {
        this->value = 0;
        if (++this->ranges_it != this->ranges_end)
            this->value = this->ranges_it->first();
    }

    return *this;
}

bool tur::id::operator!=(const IdxRangeCat::const_iterator &lhs, const IdxRangeCat::const_iterator &rhs)
{
    return lhs.ranges_it != rhs.ranges_it || lhs.value != rhs.value;
}

bool tur::id::operator==(const IdxRangeCat::const_iterator &lhs, const IdxRangeCat::const_iterator &rhs)
{
    return lhs.ranges_it == rhs.ranges_it && lhs.value == rhs.value;
}


//////////////////////////////////////////////////

IdxRangeCat::const_iterator IdxRangeCat::begin() const
{
    IdxRangeCat::const_iterator it;

    it.ranges_it = this->m_ranges.cbegin();
    it.ranges_end = this->m_ranges.cend();
    it.value = it.ranges_it->first();

    return it;
}

IdxRangeCat::const_iterator IdxRangeCat::end() const
{
    IdxRangeCat::const_iterator it;

    it.ranges_it = this->m_ranges.cend();
    it.ranges_end = this->m_ranges.cend();
    it.value = 0;

    return it;
}

bool IdxRangeCat::contains(index_t value) const
{
    return std::any_of(this->m_ranges.cbegin(), this->m_ranges.cend(), [value](const idxrange_t &range){
        return range.contains(value);
    });
}

void IdxRangeCat::append(const idxrange_t &range)
{
    this->m_ranges.push_back(range);
}


//////////////////////////////////////////////////


IdRefEval::IdRefEval(name_t name, idxeval_t &&idxeval)
    : name(name)
    , idxeval(std::move(idxeval))
{}

void IdRefEval::setName(name_t name)
{
    this->name = name;
}

void IdRefEval::setIdxEval(idxeval_t &&idxeval)
{
    this->idxeval = std::move(idxeval);
}

void IdRefEval::parse(QList<Token>::const_iterator begin, QList<Token>::const_iterator end)
{
    name_t name;
    idxeval_t idxeval;

    auto it = begin;

    if (it == end || it->type != Token::ID)
        throw ParseError{ CommonError{
            .srcRef = it->srcRef,
            .msg = QObject::tr("Expected identifier")
        }};

    name = it->value.toString();
    ++it;

    while (it != end) {
        if (it->type != Token::BRACKET_L)
            throw ParseError{ CommonError{
                .srcRef = it->srcRef,
                .msg = QObject::tr("Expected '['")
            }};
        ++it;

        auto lbound = it, rbound = nextToken(it, end, Token::BRACKET_R);
        if (rbound == end)
            throw ParseError{ CommonError {
                .srcRef = it->srcRef,
                .msg = QObject::tr("Bracket has not been closed with ']'")
            }};
        it = rbound;
        ++it;

        idxeval.push_back(tur::math::Expression::parse(lbound, rbound));
    }

    this->name = name;
    this->srcRef = begin->srcRef;
    this->idxeval = std::move(idxeval);
}

SourceRef IdRefEval::getSrcRef() const
{
    return this->srcRef;
}

IdRef IdRefEval::eval(const ctx::Context *vars) const
{
    IdRef ref;

    ref.name = this->name;
    for (const auto &expr : this->idxeval)
        ref.idx.push_back(expr->eval(vars));

    return ref;
}


//////////////////////////////////////////////////


IdxRangeEval::IdxRangeEval(std::unique_ptr<tur::math::IEvaluable> &&first, std::unique_ptr<tur::math::IEvaluable> &&last)
    : first(std::move(first))
    , last(std::move(last))
{}

void IdxRangeEval::setFirst(indexeval_t &&first)
{
    this->first = std::move(first);
}

void IdxRangeEval::setLast(indexeval_t &&last)
{
    this->last = std::move(last);
}

void IdxRangeEval::setSrcRef(SourceRef srcRef)
{
    this->srcRef = srcRef;
}

idxrange_t IdxRangeEval::eval(const ctx::Context *vars) const
{
    idxrange_t range(
        this->first->eval(vars),
        this->last ? this->last->eval(vars) : this->first->eval(vars), // last == nullptr means 1-length range (first..first)
        this->srcRef
    );

    return range;
}


//////////////////////////////////////////////////


void IdxRangeCatEval::append(IdxRangeEval &&range)
{
    this->ranges.push_back(std::move(range));
}

void IdxRangeCatEval::parse(QList<Token>::const_iterator begin, QList<Token>::const_iterator end)
{
    decltype(this->ranges) ranges;
    IdxRangeEval rangeeval;

    auto it = begin;

    while (true) {
        if (it == end) {
            throw ParseError{ CommonError{
                .srcRef = it->srcRef,
                .msg = QObject::tr("Expected index range")
            }};
        }

        auto rbound_range = nextToken(it, end, Token::RANGE);
        auto rbound_cat = nextToken(it, end, Token::CAT);
        auto rbound = rbound_range < rbound_cat ? rbound_range : rbound_cat;

        rangeeval.setSrcRef(it->srcRef);
        rangeeval.setFirst(tur::math::Expression::parse(it, rbound));
        it = rbound;

        if (rbound != end && rbound->type == Token::RANGE) {
            ++it;

            rbound = nextToken(it, end, Token::CAT);
            rangeeval.setLast(tur::math::Expression::parse(it, rbound));
            it = rbound;
        }

        ranges.push_back(std::move(rangeeval));

        if (it == end)
            break;

        ++it;
    }

    this->ranges = std::move(ranges);
}

bool IdxRangeCatEval::isEmpty() const
{
    return this->ranges.empty();
}

IdxRangeCat IdxRangeCatEval::eval(const ctx::Context *vars) const
{
    IdxRangeCat rangecat;

    for (const auto &range_eval : this->ranges)
        rangecat.append(range_eval.eval(vars));

    return rangecat;
}


//////////////////////////////////////////////////


IndexIter::IndexIter(ctx::Context &ctx, SourceRef srcRef, name_t name, IdxRangeCat &&rangecat)
    : rangecat(std::move(rangecat))
    , it(this->rangecat.begin())
    , var(ctx, srcRef, name, *this->it)
{}

IndexIter::IndexIter(IndexIter &&other)
    : var(std::move(other.var))
{
    auto dist = other.it.ranges_end - other.it.ranges_it;

    this->rangecat = std::move(other.rangecat);
    this->it = this->rangecat.end();

    this->it.value = other.it.value;
    this->it.ranges_it -= dist;
}

bool IndexIter::next()
{
    if (++this->it == this->rangecat.end())
        return false;

    this->var = *this->it;
    return true;
}

index_t IndexIter::value() const
{
    return this->var.value();
}

void IndexIter::reset()
{
    this->it = this->rangecat.begin();
    this->var = *it;
}


//////////////////////////////////////////////////


IndexIterEval::IndexIterEval(name_t name, IdxRangeCatEval &&rangecateval)
    : name(name)
    , rangecateval(std::move(rangecateval))
{}

void IndexIterEval::setName(name_t name)
{
    this->name = name;
}

void IndexIterEval::setRangeCatEval(IdxRangeCatEval &&rangecateval)
{
    this->rangecateval = std::move(rangecateval);
}

void IndexIterEval::parse(QList<Token>::const_iterator begin, QList<Token>::const_iterator end)
{
    static quint64 anon_counter = 0;

    auto it = begin;
    this->srcRef = begin->srcRef;

    switch (it->type) {
    case Token::ID:
        this->name = it->value.toString();
        break;
    case Token::ANON:
        this->name = QString("_%1").arg(++anon_counter);
        break;
    default:
        throw ParseError{ CommonError{
            .srcRef = it->srcRef,
            .msg = QObject::tr("Expected identifier or '_'")
        }};
    }

    ++it;

    if (it == end)
        return;

    if (it->type != Token::ITER) {
        throw ParseError{ CommonError{
            .srcRef = it->srcRef,
            .msg = QObject::tr("Expected '|'")
        }};
    }
    ++it;

    this->rangecateval.parse(it, end);
}

IndexIter IndexIterEval::eval(ctx::Context &ctx, const idxrange_t &size) const
{
    IdxRangeCat rangecat;

    if (this->rangecateval.isEmpty())
        rangecat.append(size);
    else
        rangecat = this->rangecateval.eval(&ctx);

    return IndexIter(ctx, this->srcRef, this->name, std::move(rangecat));
}


//////////////////////////////////////////////////


IdRefIter::IdRefIter()
    : is_end(true)
{}

IdRefIter::IdRefIter(name_t name, ctx::Context *ctx, const IdRefIterEval *parent, const shape_t &shape)
    : name(name)
    , ctx(ctx)
    , parent(parent)
    , shape(shape)
    , is_end(false)
{
    for (qsizetype i = 0; i < this->parent->idx.size(); ++i) {
        const auto &r = this->parent->idx[i];

        if (std::holds_alternative<indexeval_t>(r)) {
            this->idx.push_back(_index_t{});
            continue;
        }

        this->idx.push_back(std::get<IndexIterEval>(r).eval(*ctx, this->shape[i]));
    }
}

bool IdRefIter::value(idx_t &idx_out) const
{
    if (this->is_end)
        return false;

    idx_t idx;
    for (qsizetype i = 0; i < this->parent->idx.size(); ++i) {
        index_t index;

        auto &curr = this->idx[i];

        if (curr) {
            index = curr->value();
        } else {
            index = std::get<indexeval_t>(this->parent->idx[i])->eval(this->ctx);
        }

        idx.push_back(index);
    }

    idx_out = std::move(idx);
    return true;
}

bool IdRefIter::next()
{
    if (this->is_end)
        return false;

    qsizetype i = this->idx.size();
    while (--i >= 0) {
        auto &curr = this->idx[i];
        if (!curr)
            continue;

        if (curr->next())
            break;
    }

    if (i < 0) {
        this->is_end = true;
        return false;
    }

    while (++i < this->idx.size()) {
        auto &curr = this->idx[i];
        if (!curr)
            continue;

        curr.reset();
        curr.emplace(std::get<IndexIterEval>(this->parent->idx[i]).eval(*ctx, this->shape[i]));
    }

    return true;
}


//////////////////////////////////////////////////


IdRefIterEval::IdRefIterEval(name_t name, _idx_t &&idx)
    : name(name)
    , idx(std::move(idx))
{}

void IdRefIterEval::setName(name_t name)
{
    this->name = name;
}

void IdRefIterEval::setIdx(_idx_t &&idx)
{
    this->idx = std::move(idx);
}

void IdRefIterEval::parse(QList<Token>::const_iterator begin, QList<Token>::const_iterator end)
{
    name_t name;
    IdRefIterEval::_idx_t idxeval;

    auto it = begin;

    if (it == end || it->type != Token::ID) {
        throw ParseError{ CommonError{
            .srcRef = it->srcRef,
            .msg = QObject::tr("Expected identifier")
        }};
    }

    name = it->value.toString();
    ++it;

    while (it != end) {
        if (it->type == Token::BRACKET_L) {
            ++it;
            auto lbound = it, rbound = nextToken(it, end, Token::BRACKET_R);
            if (rbound == end) {
                throw ParseError{ CommonError{
                    .srcRef = it->srcRef,
                    .msg = QObject::tr("Bracket has not been closed with ']'")
                }};
            }
            it = rbound;
            ++it;

            idxeval.push_back(tur::math::Expression::parse(lbound, rbound));
        }
        else if (it->type == Token::BRACE_L) {
            ++it;
            auto lbound = it, rbound = nextToken(it, end, Token::BRACE_R);
            if (rbound == end)
                throw ParseError{ CommonError{
                    .srcRef = it->srcRef,
                    .msg = QObject::tr("Brace has not been closed with '}'")
                }};
            it = rbound;
            ++it;

            IndexIterEval iditereval;
            iditereval.parse(lbound, rbound);
            idxeval.push_back(std::move(iditereval));
        }
        else {
            throw ParseError{ CommonError{
                .srcRef = it->srcRef,
                .msg = QObject::tr("Expected '[' or '{'")
            }};
        }
    }

    this->name = name;
    this->srcRef = begin->srcRef;
    this->idx = std::move(idxeval);
}

SourceRef IdRefIterEval::getSrcRef() const
{
    return this->srcRef;
}

name_t IdRefIterEval::getName() const
{
    return this->name;
}

IdRefIter IdRefIterEval::eval(ctx::Context &ctx, const shape_t &shape) const
{
    return IdRefIter(this->name, &ctx, this, shape);
}


//////////////////////////////////////////////////


IdSpace::IdSpace(id_t start)
    : m_start(start)
    , m_stop(start)
{}

void IdSpace::push(const IdDesc &desc, SourceRef srcRef)
{
    if (this->m_nameToId.contains(desc.name))
        throw std::logic_error("name uniqueness must be checked beforehand");

    id_t id = this->m_stop;
    safe<id_t> stop = id;
    safe<quint64> len = 1;

    try {
        for (const idxrange_t &range : desc.shape)
            len *= range.size();
        stop += len;
    } catch (const std::exception&) {
        throw IdInitError{CommonError{
            .srcRef = srcRef,
            .msg = QObject::tr("Size of '%1' is too big").arg(desc.name)
        }};
    }

    this->m_nameToId.insert(desc.name, id);
    this->m_idToDesc[id] = desc;
    this->m_stop = stop;
}

void IdSpace::setAltId(const IdRef &ref, id_t altId, SourceRef srcRef)
{
    if (!this->isAlt(altId))
        throw std::logic_error("Alternative id must not intersect the id space");

    id_t id;
    if (!this->get_id_raw(ref, id))
        throw IdAccessError{CommonError{
            .srcRef = srcRef,
            .msg = QObject::tr("Invalid reference")
        }};

    if (this->m_altId.contains(id)) {
        throw IdInitError{CommonError{
            .srcRef = srcRef,
            .msg = QObject::tr("Cannot re-assign value")
        }};
    }

    this->m_altId.insert(id, altId);
}

id_t IdSpace::getId(const IdRef &ref, SourceRef srcRef) const
{
    id_t id;
    if (!this->get_id_raw(ref, id)) {
        throw IdAccessError{CommonError{
            .srcRef = srcRef,
            .msg = QObject::tr("Invalid reference")
        }};
    }

    if (!this->m_altId.contains(id))
        return id;

    return this->m_altId.value(id);
}

IdRef IdSpace::getRef(id_t id) const
{
    if (id < this->m_start || id >= this->m_stop)
        throw std::logic_error("Id is not in the id space");

    auto it = --this->m_idToDesc.upper_bound(id);
    id_t base_id = it->first;
    const IdDesc &desc = it->second;

    IdRef ref;
    ref.name = desc.name;

    quint64 pos = id - base_id;
    for (auto it = desc.shape.crbegin(); it != desc.shape.crend(); ++it) {
        ref.idx.push_front((*it)[pos % it->size()]);
        pos /= it->size();
    }

    return ref;
}

IdDesc IdSpace::getDesc(name_t name, SourceRef srcRef) const
{
    if (!this->m_nameToId.contains(name))
        throw IdAccessError{CommonError{
            .srcRef = srcRef,
            .msg = QObject::tr("Identifier '%1' does not exist in this context").arg(name)
        }};

    auto id = this->m_nameToId.value(name);
    return this->m_idToDesc.at(id);
}

bool IdSpace::contains(name_t name) const
{
    return this->m_nameToId.contains(name);
}

bool IdSpace::isAlt(id_t id) const
{
    return id < this->m_start;
}

id_t IdSpace::start() const
{
    return this->m_start;
}

id_t IdSpace::stop() const
{
    return this->m_stop;
}


bool IdSpace::get_id_raw(const IdRef &ref, id_t &id_r) const
{
    if (!this->m_nameToId.contains(ref.name))
        return false;

    id_t id = this->m_nameToId.value(ref.name);
    const IdDesc &desc = this->m_idToDesc.at(id);

    if (!IdSpace::apply_idx(id, ref.idx, desc.shape))
        return false;

    id_r = id;
    return true;
}

bool IdSpace::apply_idx(id_t &id, idx_t idx, shape_t shape)
{
    uint64_t pos;
    if (!idxToPos(idx, shape, pos))
        return false;

    id += pos;
    return true;
}


//////////////////////////////////////////////////


StringValue::StringValue(QString str)
    : value(str.toUcs4())
{}

uint64_t StringValue::size() const
{
    return this->value.size();
}

sym_t StringValue::operator[](uint64_t i) const
{
    return this->value[i];
}


//////////////////////////////////////////////////


StringCat::StringCat()
    : m_size(0)
{}

void StringCat::append(std::unique_ptr<istring_t> str)
{
    this->m_size += str->size();
    this->strings.push_back(std::move(str));
}

void StringCat::parse(QList<Token>::const_iterator begin, QList<Token>::const_iterator end)
{
    enum {NEXT_VALUE, RBOUND, CAT_OR_END, ANY_OP} expected = NEXT_VALUE;

    auto it_val_1 = end, it_val_2 = end;

    for (auto it = begin; true; ++it) {
        if (it == end || it->type == Token::CAT) {
            if (expected != CAT_OR_END && expected != ANY_OP)
                throw ParseError{ CommonError{
                    .srcRef = it->srcRef,
                    .msg = QObject::tr("Expected string literal")
                }};

            if (it_val_2 == end) {
                auto value = it_val_1->value.toString();
                if (!value.isEmpty()) {
                    this->append(std::unique_ptr<istring_t>(
                        new StringValue{value}
                    ));
                }
            } else {
                auto lvalue = it_val_1->value.toString().toUcs4();
                auto rvalue = it_val_2->value.toString().toUcs4();

                if (lvalue.size() != 1 || rvalue.size() != 1)
                    throw ParseError{ CommonError{
                        .srcRef = it_val_1->srcRef,
                        .msg = QObject::tr("String range operands must be of length 1")
                    }};

                this->append(std::unique_ptr<istring_t>(
                    new symrange_t{lvalue.front(), rvalue.front(), it_val_1->srcRef}
                ));
            }

            if (it == end) break;

            it_val_1 = it_val_2 = end;
            expected = NEXT_VALUE;
            continue;
        }

        switch (it->type) {
        case Token::STRING:
            if (expected == NEXT_VALUE) {
                it_val_1 = it;
                expected = ANY_OP;
            } else if (expected == RBOUND) {
                it_val_2 = it;
                expected = CAT_OR_END;
            } else {
                throw ParseError{ CommonError{
                    .srcRef = it->srcRef,
                    .msg = QObject::tr("Unexpected string literal")
                }};
            }

            continue;

        case Token::RANGE:
            if (expected != ANY_OP) {
                throw ParseError{ CommonError{
                    .srcRef = it->srcRef,
                    .msg = QObject::tr("Unexpected '..'")
                }};
            }

            expected = RBOUND;

            continue;

        default:
            throw ParseError{ CommonError{
                .srcRef = it->srcRef,
                .msg = QObject::tr("Unexpected token")
            }};
        }
    }
}

uint64_t StringCat::size() const
{
    return this->m_size;
}

sym_t StringCat::operator[](uint64_t i) const
{
    if (i >= this->m_size)
        throw std::logic_error("out of bound");

    auto it = this->strings.cbegin();
    while (i >= (*it)->size()) {
        i -= (*it)->size();
        ++it;
    }

    return (**it)[i];
}


//////////////////////////////////////////////////


void SymSpace::insert(SymDesc &&desc, SourceRef srcRef)
{
    if (this->m_nameToDesc.contains(desc.name))
        throw std::logic_error("name uniqueness must be checked beforehand");

    safe<quint64> len = 1;

    try {
        for (const idxrange_t &range : desc.shape)
            len *= range.size();
    } catch (const std::exception&) {
        throw IdInitError{CommonError{
            .srcRef = srcRef,
            .msg = QObject::tr("Size of '%1' is too big").arg(desc.name)
        }};
    }

    if (desc.value.size() != len)
        throw IdInitError{CommonError{
            .srcRef = srcRef,
            .msg = QObject::tr("Value size doesn't match size of '%1'").arg(desc.name)
        }};

    this->m_nameToDesc[desc.name] = std::move(desc);
}

sym_t SymSpace::getSym(const IdRef &ref, SourceRef srcRef) const
{
    if (!this->m_nameToDesc.contains(ref.name)) {
        throw IdAccessError{CommonError{
            .srcRef = srcRef,
            .msg = QObject::tr("Identifier '%1' does not exist in this context").arg(ref.name)
        }};
    }

    const SymDesc &desc = this->m_nameToDesc.at(ref.name);

    uint64_t pos;
    if (!idxToPos(ref.idx, desc.shape, pos)) {
        throw IdAccessError{CommonError{
            .srcRef = srcRef,
            .msg = QObject::tr("Invalid reference")
        }};
    }

    return desc.value[pos];
}

const SymDesc& SymSpace::getDesc(name_t name, SourceRef srcRef) const
{
    if (!this->m_nameToDesc.contains(name)) {
        throw IdAccessError{CommonError{
            .srcRef = srcRef,
            .msg = QObject::tr("Identifier '%1' does not exist in this context").arg(name)
        }};
    }

    return this->m_nameToDesc.at(name);
}

bool SymSpace::contains(name_t name) const
{
    return this->m_nameToDesc.contains(name);
}
