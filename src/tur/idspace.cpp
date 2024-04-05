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


index_t IndexRangeCat::const_iterator::operator*() const
{
    return this->value;
}

IndexRangeCat::const_iterator& IndexRangeCat::const_iterator::operator++()
{
    if (!this->ranges_it->next(this->value)) {
        this->value = 0;
        if (++this->ranges_it != this->ranges_end)
            this->value = this->ranges_it->first();
    }

    return *this;
}

bool tur::id::operator!=(const IndexRangeCat::const_iterator &lhs, const IndexRangeCat::const_iterator &rhs)
{
    return lhs.ranges_it != rhs.ranges_it || lhs.value != rhs.value;
}


//////////////////////////////////////////////////

IndexRangeCat::const_iterator IndexRangeCat::begin() const
{
    IndexRangeCat::const_iterator it;

    it.ranges_it = this->m_ranges.cbegin();
    it.ranges_end = this->m_ranges.cend();
    it.value = it.ranges_it->first();

    return it;
}

IndexRangeCat::const_iterator IndexRangeCat::end() const
{
    IndexRangeCat::const_iterator it;

    it.ranges_it = this->m_ranges.cend();
    it.ranges_end = this->m_ranges.cend();
    it.value = 0;

    return it;
}

bool IndexRangeCat::contains(index_t value) const
{
    return std::any_of(this->m_ranges.cbegin(), this->m_ranges.cend(), [value](const idxrange_t &range){
        return range.contains(value);
    });
}

void IndexRangeCat::append(const idxrange_t &range)
{
    this->m_ranges.push_back(range);
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

void StringCat::append(const std::shared_ptr<istring_t> &str)
{
    this->m_size += str->size();
    this->strings.push_back(str);
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


void SymSpace::insert(const SymDesc &desc)
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

    this->m_nameToDesc.insert(desc.name, desc);
}

sym_t SymSpace::getSym(const IdRef &ref) const
{
    if (!this->m_nameToDesc.contains(ref.name))
        throw IdAccessError{};

    const auto &nameToDescConst = this->m_nameToDesc;
    const SymDesc &desc = nameToDescConst[ref.name];

    uint64_t pos;
    if (!idxToPos(ref.idx, desc.shape, pos))
        throw IdAccessError{};

    return desc.value[pos];

}

bool SymSpace::contains(name_t name) const
{
    return this->m_nameToDesc.contains(name);
}
