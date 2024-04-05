#ifndef IDSPACE_HPP
#define IDSPACE_HPP
#include <QtTypes>
#include <QString>
#include <QList>
#include <QHash>
#include <variant>
#include <map>
#include "tur/common.hpp"

namespace tur::id
{
    struct IdInitError {
        SourceRef srcRef;
    };

    struct IdAccessError {
        SourceRef srcRef;
    };

    template <typename T>
    class Range {
    public:
        Range(T first, T last);

        bool next(T &value) const;
        bool contains(T value) const;
        uint64_t pos(T value) const;
        uint64_t size() const;
        T first() const;
        T last() const;

    private:
        T m_first, m_last;
        uint64_t m_size;
        bool is_inv;
    };


    using name_t = QString;

    using index_t = number_t;
    using idx_t = QList<index_t>;
    using idxrange_t = Range<index_t>;
    using idxranges_t = QList<idxrange_t>;
    using shape_t = idxranges_t;

    using id_t = quint32;
    using sym_t = quint32;
    using strval_t = QList<sym_t>;
    using symrange_t = Range<sym_t>;

    class IndexRangeCat {
    public:
        class const_iterator {
            friend class IndexRangeCat;
        public:
            index_t operator*() const;
            const_iterator& operator++();
            friend bool operator!=(const const_iterator &lhs, const const_iterator &rhs);
        private:
            idxranges_t::const_iterator ranges_it, ranges_end;
            index_t value;
        };

        IndexRangeCat() = default;

        const_iterator begin() const;
        const_iterator end() const;
        bool contains(index_t value) const;

        void append(const idxrange_t &range);
    private:
        idxranges_t m_ranges;
    };

    bool operator!=(const IndexRangeCat::const_iterator &lhs, const IndexRangeCat::const_iterator &rhs);


    struct IdRef {
        name_t name;
        idx_t idx;
    };


    struct IdDesc {
        name_t name;
        shape_t shape;
    };


    class IdSpace {
    public:
        IdSpace(id_t start);

        void push(const IdDesc &desc);
        void setAltId(const IdRef &ref, id_t altId);

        id_t getId(const IdRef &ref) const;
        IdRef getRef(id_t id) const;

        bool contains(name_t name) const;
        bool isAlt(id_t id) const;

        id_t start() const;
        id_t stop() const;

    private:
        bool get_id_raw(const IdRef &ref, id_t &id_r) const;
        static bool apply_idx(id_t &id, idx_t idx, shape_t shape);

        id_t m_start, m_stop;
        std::map<id_t, IdDesc> m_idToDesc;
        QHash<name_t, id_t> m_nameToId;
        QHash<id_t, id_t> m_altId;
    };


    class StringCat
    {
    public:
        StringCat();
        sym_t operator[](size_t i) const;
    private:
        QList<std::variant<strval_t, symrange_t>> strings;
    };


    struct SymDesc {
        name_t name;
        shape_t shape;
        StringCat value;
    };


    class SymSpace
    {
    public:
        SymSpace();
        void insert(const SymDesc &desc);
        sym_t getSym(const IdRef &ref);
        bool contains(name_t name);
    private:
        QHash<name_t, SymDesc> m_nameToDesc;
    };
}

#endif // IDSPACE_HPP
