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
Range<T>::Range(T first, T last)
    : m_first(first)
    , m_last(last)
    , is_inv(first > last)
{
    m_size = 1 + (
        this->is_inv
        ? (uint64_t)(m_first) - (uint64_t)(m_last)
        : (uint64_t)(m_last) - (uint64_t)(m_first)
    );

    if (m_size == 0) // (uint64_t)INT64_MAX - (uint64_t)INT64_MIN == 0
        throw IdInitError{};
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
        throw ParseError();

    name = it->value.toString();
    ++it;

    while (it != end) {
        if (it->type != Token::BRACKET_L)
            throw ParseError();
        ++it;

        auto lbound = it, rbound = nextToken(it, end, Token::BRACKET_R);
        if (rbound == end)
            throw ParseError();
        it = rbound;
        ++it;

        idxeval.push_back(tur::math::Expression::parse(lbound, rbound));
    }

    this->name = name;
    this->idxeval = std::move(idxeval);
}

IdRef IdRefEval::eval(const ctx::context_t *vars) const
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

idxrange_t IdxRangeEval::eval(const ctx::context_t *vars) const
{
    idxrange_t range(
        this->first->eval(vars),
        this->last->eval(vars)
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
        if (it == end)
            throw ParseError();

        auto rbound = nextToken(it, end, Token::RANGE);
        if (rbound == end)
            throw ParseError();
        rangeeval.setFirst(tur::math::Expression::parse(it, rbound));

        it = rbound;
        ++it;

        rbound = nextToken(it, end, Token::CAT);
        rangeeval.setLast(tur::math::Expression::parse(it, rbound));

        ranges.push_back(std::move(rangeeval));

        if (rbound == end)
            break;

        it = rbound;
        ++it;
    }

    this->ranges = std::move(ranges);
}

bool IdxRangeCatEval::isEmpty() const
{
    return this->ranges.empty();
}

IdxRangeCat IdxRangeCatEval::eval(const ctx::context_t *vars) const
{
    IdxRangeCat rangecat;

    for (const auto &range_eval : this->ranges)
        rangecat.append(range_eval.eval(vars));

    return rangecat;
}


//////////////////////////////////////////////////


IndexIter::IndexIter(ctx::context_t &ctx, name_t name, IdxRangeCat &&rangecat)
    : rangecat(std::move(rangecat))
    , it(this->rangecat.begin())
    , var(ctx, name, *this->it)
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
    auto it = begin;

    if (it == end || it->type != Token::ID)
        throw ParseError();

    this->name = it->value.toString();
    ++it;

    if (it == end)
        return;

    if (it->type != Token::ITER)
        throw ParseError();
    ++it;

    this->rangecateval.parse(it, end);
}

IndexIter IndexIterEval::eval(ctx::context_t &ctx, const idxrange_t &size) const
{
    IdxRangeCat rangecat;

    if (this->rangecateval.isEmpty())
        rangecat.append(size);
    else
        rangecat = this->rangecateval.eval(&ctx);

    return IndexIter(ctx, this->name, std::move(rangecat));
}


//////////////////////////////////////////////////


IdRefIter::IdRefIter(name_t name, _idx_t &&idx)
    : name(name)
    , idx(std::move(idx))
    , is_end(false)
{}

bool IdRefIter::value(idx_t &idx_out) const
{
    if (this->is_end)
        return false;

    idx_t idx;
    for (auto it = this->idx.begin(); it != this->idx.end(); ++it) {
        index_t index;
        if (std::holds_alternative<index_t>(*it)) {
            index = std::get<index_t>(*it);
        } else {
            index = std::get<IndexIter>(*it).value();
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

    auto it_rev = this->idx.rbegin();
    for (; it_rev != this->idx.rend(); ++it_rev) {
        if (std::holds_alternative<index_t>(*it_rev))
            continue;

        auto &iter = std::get<IndexIter>(*it_rev);
        if (iter.next())
            break;

        iter.reset();
    }

    if (it_rev == this->idx.rend()) {
        this->is_end = true;
        return false;
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
    static quint64 anon_counter = 0;

    name_t name;
    IdRefIterEval::_idx_t idxeval;

    auto it = begin;

    if (it == end)
        throw ParseError();

    switch (it->type) {
    case Token::ID:
        name = it->value.toString();
        break;
    case Token::ANON:
        name = QString("_%1").arg(++anon_counter);
        break;
    default:
        throw ParseError();
    }

    name = it->value.toString();
    ++it;

    while (it != end) {
        if (it->type == Token::BRACKET_L) {
            ++it;
            auto lbound = it, rbound = nextToken(it, end, Token::BRACKET_R);
            if (rbound == end)
                throw ParseError();
            it = rbound;
            ++it;

            idxeval.push_back(tur::math::Expression::parse(lbound, rbound));
        }
        else if (it->type == Token::BRACE_L) {
            ++it;
            auto lbound = it, rbound = nextToken(it, end, Token::BRACE_R);
            if (rbound == end)
                throw ParseError();
            it = rbound;
            ++it;

            IndexIterEval iditereval;
            iditereval.parse(lbound, rbound);
            idxeval.push_back(std::move(iditereval));
        }
        else throw ParseError();
    }

    this->name = name;
    this->idx = std::move(idxeval);
}

IdRefIter IdRefIterEval::eval(ctx::context_t &ctx, const shape_t &shape) const
{
    IdRefIter::_idx_t idx;

    for (qsizetype i = 0; i < this->idx.size(); ++i) {
        const auto &curr = this->idx.at(i);

        if (std::holds_alternative<indexeval_t>(curr)) {
            idx.push_back(std::get<indexeval_t>(curr)->eval(&ctx));
        } else {
            idx.push_back(std::get<IndexIterEval>(curr).eval(ctx, shape[i]));
        }
    }

    return IdRefIter(this->name, std::move(idx));
}


//////////////////////////////////////////////////


IdSpace::IdSpace(id_t start)
    : m_start(start)
    , m_stop(start)
{}

void IdSpace::push(const IdDesc &desc)
{
    if (this->m_nameToId.contains(desc.name))
        throw IdInitError{};

    id_t id = this->m_stop;
    safe<id_t> stop = id;
    safe<quint64> len = 1;

    try {
        for (const idxrange_t &range : desc.shape)
            len *= range.size();
        stop += len;
    } catch (const std::exception&) {
        throw IdInitError{};
    }

    this->m_nameToId.insert(desc.name, id);
    this->m_idToDesc[id] = desc;
    this->m_stop = stop;
}

void IdSpace::setAltId(const IdRef &ref, id_t altId)
{
    if (!this->isAlt(altId))
        throw std::logic_error("Alternative id must not intersect the id space");

    id_t id;
    if (!this->get_id_raw(ref, id))
        throw IdInitError{};

    if (this->m_altId.contains(id))
        throw IdInitError{};

    this->m_altId.insert(id, altId);
}

id_t IdSpace::getId(const IdRef &ref) const
{
    id_t id;
    if (!this->get_id_raw(ref, id))
        throw IdAccessError{};

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
                throw ParseError();

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
                    throw ParseError();

                this->append(std::unique_ptr<istring_t>(
                    new symrange_t{lvalue.front(), rvalue.front()}
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
            } else throw ParseError();

            continue;

        case Token::RANGE:
            if (expected != ANY_OP)
                throw ParseError();

            expected = RBOUND;

            continue;

        default:
            throw ParseError();
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


void SymSpace::insert(SymDesc &&desc)
{
    if (this->m_nameToDesc.contains(desc.name))
        throw IdInitError{};

    safe<quint64> len = 1;

    try {
        for (const idxrange_t &range : desc.shape)
            len *= range.size();
    } catch (const std::exception&) {
        throw IdInitError{};
    }

    if (desc.value.size() != len)
        throw IdInitError{};

    this->m_nameToDesc[desc.name] = std::move(desc);
}

sym_t SymSpace::getSym(const IdRef &ref) const
{
    if (!this->m_nameToDesc.contains(ref.name))
        throw IdAccessError{};

    const SymDesc &desc = this->m_nameToDesc.at(ref.name);

    uint64_t pos;
    if (!idxToPos(ref.idx, desc.shape, pos))
        throw IdAccessError{};

    return desc.value[pos];

}

bool SymSpace::contains(name_t name) const
{
    return this->m_nameToDesc.contains(name);
}
