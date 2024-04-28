#ifndef IDSPACE_HPP
#define IDSPACE_HPP
#include <QtTypes>
#include <QString>
#include <QList>
#include <QHash>
#include <variant>
#include <vector>
#include <map>
#include "tur/common.hpp"
#include "tur/mathexpr.hpp"

namespace tur::id
{
    struct IdInitError {
        SourceRef srcRef;
    };

    struct IdAccessError {
        SourceRef srcRef;
    };


    template <typename T>
    class IArray
    {
    public:
        virtual uint64_t size() const = 0;
        virtual T operator[](uint64_t i) const = 0;
        virtual ~IArray() = default;
    };

    template <typename T>
    class Range : public IArray<T> {
    public:
        Range(T first, T last);

        bool next(T &value) const;
        bool contains(T value) const;
        uint64_t pos(T value) const;
        virtual uint64_t size() const override;
        virtual T operator[](uint64_t i) const override;
        T first() const;
        T last() const;

    private:
        T m_first, m_last;
        uint64_t m_size;
        bool is_inv;
    };


    using id_t = quint32;
    using name_t = QString;
    using index_t = number_t;

    using idx_t = QList<index_t>;
    using idxrange_t = Range<index_t>;
    using idxranges_t = QList<idxrange_t>;
    using shape_t = idxranges_t;

    using sym_t = quint32;
    using istring_t = IArray<sym_t>;
    using symrange_t = Range<sym_t>;

    class IdxRangeCat {
    public:
        class const_iterator {
            friend class IdxRangeCat;
            friend class IndexIter;
        public:
            const_iterator() = default;
            const_iterator(const const_iterator&) = default;
            index_t operator*() const;
            const_iterator& operator++();
            friend bool operator!=(const const_iterator &lhs, const const_iterator &rhs);
            friend bool operator==(const const_iterator &lhs, const const_iterator &rhs);
        private:
            idxranges_t::const_iterator ranges_it, ranges_end;
            index_t value;
        };

        IdxRangeCat() = default;
        IdxRangeCat(IdxRangeCat&&) = default;
        IdxRangeCat& operator=(IdxRangeCat&&) = default;

        const_iterator begin() const;
        const_iterator end() const;
        bool contains(index_t value) const;

        void append(const idxrange_t &range);
    private:
        idxranges_t m_ranges;
    };

    bool operator!=(const IdxRangeCat::const_iterator &lhs, const IdxRangeCat::const_iterator &rhs);
    bool operator==(const IdxRangeCat::const_iterator &lhs, const IdxRangeCat::const_iterator &rhs);


    struct IdRef {
        name_t name;
        idx_t idx;
    };


    struct IdDesc {
        name_t name;
        shape_t shape;
    };



    using indexeval_t = std::unique_ptr<tur::math::IEvaluable>;
    using idxeval_t = std::vector<indexeval_t>;

    class IdRefEval {
    public:
        IdRefEval() = default;
        IdRefEval(name_t name, idxeval_t &&idxeval);

        void setName(name_t name);
        void setIdxEval(idxeval_t &&idxeval);
        void parse(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);
        IdRef eval(const ctx::context_t *vars = nullptr) const;
    private:
        name_t name;
        idxeval_t idxeval;
    };


    class IdxRangeEval {
    public:
        IdxRangeEval() = default;
        IdxRangeEval(indexeval_t &&first, indexeval_t &&last);

        void setFirst(indexeval_t &&first);
        void setLast(indexeval_t &&last);
        idxrange_t eval(const ctx::context_t *vars = nullptr) const;
    private:
        indexeval_t first, last;
    };

    class IdxRangeCatEval {
    public:
        IdxRangeCatEval() = default;

        void append(IdxRangeEval &&range);
        void parse(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);

        bool isEmpty() const;
        IdxRangeCat eval(const ctx::context_t *vars = nullptr) const;
    private:
        std::vector<IdxRangeEval> ranges;
    };


    class IndexIter {
    public:
        IndexIter(ctx::context_t &ctx, name_t name, IdxRangeCat &&rangecat);
        IndexIter(const IndexIter&) = delete;
        IndexIter(IndexIter &&other);

        bool next();
        index_t value() const;
        void reset();
    private:
        IdxRangeCat rangecat;
        IdxRangeCat::const_iterator it;
        tur::ctx::Variable var;
    };

    class IndexIterEval {
    public:
        IndexIterEval() = default;
        IndexIterEval(name_t name, IdxRangeCatEval &&rangecateval);

        void setName(name_t name);
        void setRangeCatEval(IdxRangeCatEval &&rangecateval);
        void parse(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);
        IndexIter eval(ctx::context_t &ctx, const idxrange_t &size) const;
    private:
        name_t name;
        IdxRangeCatEval rangecateval;
    };


    class IdRefIter {
    public:
        using _index_t = std::variant<index_t, IndexIter>;
        using _idx_t = std::vector<_index_t>;

        IdRefIter(name_t name, _idx_t &&idx);
        bool value(idx_t &idx_out) const;
        bool next();
    private:
        name_t name;
        _idx_t idx;
        bool is_end;
    };

    class IdRefIterEval {
    public:
        using _index_t = std::variant<indexeval_t, IndexIterEval>;
        using _idx_t = std::vector<_index_t>;

        IdRefIterEval() = default;
        IdRefIterEval(name_t name, _idx_t &&idx);

        void setName(name_t name);
        void setIdx(_idx_t &&idx);
        void parse(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);
        IdRefIter eval(ctx::context_t &ctx, const shape_t &shape) const;
    private:
        name_t name;
        _idx_t idx;
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



    class StringValue : public istring_t
    {
    public:
        StringValue(QString str);

        virtual uint64_t size() const override;
        virtual sym_t operator[](uint64_t i) const override;
    private:
        QList<sym_t> value;
    };


    class StringCat : public istring_t
    {
    public:
        StringCat();
        void append(std::unique_ptr<istring_t> str);
        void parse(QList<Token>::const_iterator begin, QList<Token>::const_iterator end);

        virtual uint64_t size() const override;
        virtual sym_t operator[](uint64_t i) const override;
    private:
        quint64 m_size;
        std::vector<std::unique_ptr<istring_t>> strings;
    };


    struct SymDesc : public IdDesc {
        StringCat value;
    };


    class SymSpace
    {
    public:
        SymSpace() = default;
        void insert(SymDesc &&desc);
        sym_t getSym(const IdRef &ref) const;
        bool contains(name_t name) const;
    private:
        std::unordered_map<name_t, SymDesc> m_nameToDesc;
    };
}

#endif // IDSPACE_HPP
