#pragma once

#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>

#ifdef DEBUG
#include <iostream>
#endif

class UnorderedMapOutOfRange : std::exception {
public:
    const char* what() const noexcept override {
        return "calling at() with argument that does no contain in the unordered map causes "
               "exception.";
    }
};

template <typename Key, typename T, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, T>>>
class UnorderedMap {
public:
    using NodeType = std::pair<const Key, T>;

private:
    struct Node {
        size_t hash;
        NodeType data;
    };

    struct ByteNode {
        alignas(std::max_align_t) std::byte data[sizeof(Node)];

        Node* get_node_p() {
            return reinterpret_cast<Node*>(data);
        }

        const Node* get_node_p() const {
            return reinterpret_cast<const Node*>(data);
        }

        size_t& get_hash() {
            return get_node_p()->hash;
        }

        NodeType& get_val() {
            return get_node_p()->data;
        }

        const size_t& get_hash() const {
            return get_node_p()->hash;
        }

        const NodeType& get_val() const {
            return get_node_p()->data;
        }
    };
    using ByteNodeAllocator =
        typename std::allocator_traits<Allocator>::template rebind_alloc<ByteNode>;
    using ListType = List<ByteNode, ByteNodeAllocator>;

    template <bool IsConst,
              typename Base = std::conditional_t<IsConst, typename ListType::const_iterator,
                                                 typename ListType::iterator>>
    class Iterator : public Base {
    public:
        using iterator_category = typename Base::iterator_category;
        using value_type = typename std::conditional<IsConst, const NodeType, NodeType>::type;
        using difference_type = typename Base::difference_type;
        using pointer = value_type*;
        using reference = value_type&;

        Iterator& operator++() {
            ++get_base();
            return *this;
        }

        Iterator& operator--() {
            --get_base();
            return *this;
        }

        Iterator operator++(int) {
            auto tmp = *this;
            ++get_base();
            return tmp;
        }

        Iterator operator--(int) {
            auto tmp = *this;
            --get_base();
            return tmp;
        }

        Iterator() = default;

        Iterator<true> operaotr() const {
            return Iterator<true>(get_base());
        }

        Iterator(const Base& it)
            : Base(it) {
        }

        template <typename OtherBase>
        Iterator(const OtherBase& it)
            : Base(it) {
        }

        reference operator*() const {
            return Base::operator->()->get_val();
        }

        pointer operator->() const {
            return &operator*();
        }

    private:
        Base& get_base() {
            return *static_cast<Base*>(this);
        }
        size_t get_hash() const {
            return Base::operator->()->get_hash();
        }
        friend class UnorderedMap;
    };

public:
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
    using reference = typename iterator::reference;
    using value_type = typename iterator::value_type;
    using const_reference = const reference;
    using size_type = size_t;
    using allocator_type =
        typename std::allocator_traits<Allocator>::template rebind_alloc<value_type>;

private:
    using AllocTraits = std::allocator_traits<Allocator>;
    using NodeAllocator = typename AllocTraits::template rebind_alloc<Node>;
    using NodeAllocTraits = std::allocator_traits<NodeAllocator>;
    using VectorAllocator = typename AllocTraits::template rebind_alloc<iterator>;

    ListType list_;
    std::vector<iterator, VectorAllocator> bucket_begins_;
    float max_load_factor_;
    [[no_unique_address]] Hash hash_;
    [[no_unique_address]] KeyEqual key_equal_;
    [[no_unique_address]] Allocator alloc_;

    void swap_without_alloc(UnorderedMap& other) {
        list_.swap(other.list_);
        bucket_begins_.swap(other.bucket_begins_);
        std::swap(max_load_factor_, other.max_load_factor_);
        std::swap(hash_, other.hash_);
        std::swap(key_equal_, other.key_equal_);
        std::swap(alloc_, other.alloc_);
    }

    template <typename Iterator>
    bool is_same_bucket(Iterator it, size_t hash) const {
        if (it == iterator() || it == end()) {
            return false;
        }
        return it.get_hash() % bucket_count() == hash % bucket_count();
    }

    iterator internal_find(const Key& key, size_t hash) const {
        iterator it = bucket_begins_[hash % bucket_count()];
        if (it == iterator()) {
            return iterator();
        }
        while (is_same_bucket(it, hash)) {
            if (key_equal_(key, it->first)) {
                return it;
            }
            ++it;
        }
        return it;
    }

    size_t required_bucket_count() const {
        return static_cast<size_t>(size() / max_load_factor());
    }

    bool do_need_rehash() const {
        return max_load_factor() * bucket_count() < size();
    }

public:
    Allocator get_allocator() const {
        return alloc_;
    }

    UnorderedMap()
        : list_(),
          bucket_begins_(1, iterator()),
          max_load_factor_(1.0),
          hash_(),
          key_equal_(),
          alloc_() {
    }

    explicit UnorderedMap(size_type bucket_count, const Hash& hash = Hash(),
                          const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator())
        : list_(),
          bucket_begins_(bucket_count, iterator(), alloc),
          max_load_factor_(1.0),
          hash_(hash),
          key_equal_(equal),
          alloc_(alloc) {
    }

    UnorderedMap(size_type bucket_count, const Allocator& alloc)
        : UnorderedMap(bucket_count, Hash(), KeyEqual(), alloc) {
    }

    UnorderedMap(size_type bucket_count, const Hash& hash, const Allocator& alloc)
        : UnorderedMap(bucket_count, hash, KeyEqual(), alloc) {
    }

    explicit UnorderedMap(const Allocator& alloc)
        : UnorderedMap(1, Hash(), KeyEqual(), alloc) {
    }

    UnorderedMap(const UnorderedMap& other, const Allocator& alloc)
        : UnorderedMap(other.bucket_count(), alloc) {
        max_load_factor_ = other.max_load_factor_;
        for (auto it = other.begin(); it != other.end(); ++it) {
            emplace(*it);
        }
    }

    UnorderedMap(const UnorderedMap& other)
        : UnorderedMap(other, AllocTraits::select_on_container_copy_construction(other.alloc_)) {
    }

    UnorderedMap(UnorderedMap&& other, const Allocator& alloc)
        : list_(alloc == other.alloc_ ? std::move(other.list_) : other.list_),
          bucket_begins_(std::move(other.bucket_begins_)),
          max_load_factor_(std::move(other.max_load_factor_)),
          hash_(std::move(other.hash_)),
          key_equal_(std::move(other.key_equal_)),
          alloc_(alloc) {
        other.max_load_factor_ = 1.0;
    }

    UnorderedMap(UnorderedMap&& other)
        : UnorderedMap(std::move(other), std::move(other.alloc_)) {
    }

    UnorderedMap& operator=(const UnorderedMap& other) {
        if (this == &other) {
            return *this;
        }
        UnorderedMap tmp(other, alloc_);
        swap_without_alloc(tmp);
        if constexpr (NodeAllocTraits::propagate_on_container_copy_assignment::value) {
            alloc_ = other.alloc_;
        }
        return *this;
    }

    UnorderedMap& operator=(UnorderedMap&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        bool is_fast_move = NodeAllocTraits::propagate_on_container_move_assignment::value ||
                            other.alloc_ == alloc_;
        if (is_fast_move) {
            UnorderedMap tmp(std::move(other));
            swap_without_alloc(tmp);
            if constexpr (NodeAllocTraits::propagate_on_container_move_assignment::value) {
                alloc_ = std::move(other.alloc_);
            }
        } else {
            UnorderedMap temp(std::move(other), alloc_);
            swap_without_alloc(temp);
        }
        return *this;
    }

    T& operator[](const Key& key) {
        return try_emplace(key).first->second;
    }

    T& operator[](Key&& key) {
        return try_emplace(std::move(key)).first->second;
    }

    const T& at(const Key& key) const {
        auto it = find(key);
        if (it == end()) {
            throw UnorderedMapOutOfRange();
        }
        return it->second;
    }

    T& at(const Key& key) {
        auto it = find(key);
        if (it == end()) {
            throw UnorderedMapOutOfRange();
        }
        return it->second;
    }

    size_type size() const noexcept {
        return list_.size();
    }

    size_type bucket_count() const noexcept {
        return bucket_begins_.size();
    }

    bool empty() const noexcept {
        return list_.empty();
    }

    iterator end() {
        return list_.end();
    }

    iterator begin() {
        return ++end();
    }

    const_iterator cend() const {
        return list_.cend();
    }

    const_iterator cbegin() const {
        return ++cend();
    }

    const_iterator begin() const {
        return cbegin();
    }

    const_iterator end() const {
        return cend();
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        ListType temp_list;
        temp_list.emplace(temp_list.begin());

        AllocTraits::construct(alloc_, &temp_list.begin()->get_val(), std::forward<Args>(args)...);
        size_t cur_hash = temp_list.begin()->get_hash() = hash_(temp_list.begin()->get_val().first);

        auto it = internal_find(temp_list.begin()->get_val().first, cur_hash);
        it = it == iterator() ? end() : it;

        if (is_same_bucket(it, cur_hash)) {
            return std::make_pair(it, false);
        }

        list_.splice(it, temp_list, temp_list.begin());
        --it;
        bucket_begins_[cur_hash % bucket_count()] = it;

        if (do_need_rehash()) {
            reserve(size() * 2);
        }
        return std::make_pair(it, true);
    }

    template <typename K, typename... Args>
    std::pair<iterator, bool> try_emplace(K&& key, Args&&... args) {
        auto it = find(key);
        if (it != end()) {
            return std::make_pair(it, false);
        }
        return emplace(std::piecewise_construct, std::forward_as_tuple(std::forward<K>(key)),
                       std::forward_as_tuple(std::forward<Args>(args)...));
    }

    std::pair<iterator, bool> insert(const NodeType& pair) {
        return emplace(pair);
    }

    std::pair<iterator, bool> insert(NodeType&& pair) {
        return emplace(std::forward<NodeType>(pair));
    }

    template <class P>
    std::pair<iterator, bool> insert(P&& pair) {
        return emplace(std::forward<P>(pair));
    }

    template <typename InputIterator>
    void insert(InputIterator begin, InputIterator end) {
        for (auto it = begin; it != end; ++it) {
            insert(std::forward<decltype(*it)>(*it));
        }
    }

    iterator erase(const_iterator pos) {
        size_t cur_hash = pos.get_hash();
        bool cond = bucket_begins_[cur_hash % bucket_count()] == pos;
        AllocTraits::destroy(alloc_, &*pos);
        iterator iterator_to_return = list_.erase(pos);
        if (cond) {
            bucket_begins_[cur_hash % bucket_count()] =
                is_same_bucket(iterator_to_return, cur_hash) ? iterator_to_return : iterator();
        }
        return iterator_to_return;
    }

    iterator erase(const_iterator bg, const_iterator en) {
        iterator result;
        while (bg != en) {
            bg = result = erase(bg);
        }
        return result;
    }

    const_iterator find(const Key& key) const {
        size_t hash = hash_(key);
        auto result = internal_find(key, hash);
        if (is_same_bucket(result, hash)) {
            return result;
        }
        return end();
    }

    iterator find(const Key& key) {
        size_t hash = hash_(key);
        auto result = internal_find(key, hash);
        if (is_same_bucket(result, hash)) {
            return result;
        }
        return end();
    }

    void rehash(size_t count) {
        size_t new_bucket_count = std::max<size_t>(count, required_bucket_count());
        iterator bg = begin();
        iterator ed = end();

        ListType temp_list;
        bucket_begins_ = std::vector<iterator, VectorAllocator>(new_bucket_count, iterator());

        while (bg != ed) {
            iterator next = std::next(bg);
            if (bucket_begins_[bg.get_hash() % bucket_count()] == iterator()) {
                temp_list.splice(temp_list.end(), list_, bg);
            } else {
                temp_list.splice(bucket_begins_[bg.get_hash() % bucket_count()], list_, bg);
            }
            bucket_begins_[bg.get_hash() % bucket_count()] = bg;
            bg = next;
        }

        temp_list.swap(list_);
    }

    void reserve(size_t count) {
        rehash(std::ceil(count / max_load_factor()));
    }

    float load_factor() const noexcept {
        return static_cast<float>(size()) / static_cast<float>(bucket_count());
    }

    void max_load_factor(float new_load_factor) {
        max_load_factor_ = new_load_factor;
    }

    float max_load_factor() const noexcept {
        return max_load_factor_;
    }

    void swap(UnorderedMap& other) noexcept(NodeAllocTraits::propagate_on_container_swap::value) {
        bool is_fast_swap =
            NodeAllocTraits::propagate_on_container_swap::value || other.alloc_ == alloc_;
        if (is_fast_swap) {
            list_.swap(other.list_);
            bucket_begins_.swap(other.bucket_begins_);
            std::swap(max_load_factor_, other.max_load_factor_);
            std::swap(hash_, other.hash_);
            std::swap(key_equal_, other.key_equal_);
            if constexpr (NodeAllocTraits::propagate_on_container_swap::value) {
                std::swap(alloc_, other.alloc_);
            }
        } else {
            UnorderedMap temp_this(other, alloc_);
            UnorderedMap temp_other(other, alloc_);

            swap_without_alloc(temp_this);
            other.swap_without_alloc(temp_other);
        }
    }

    void clear() {
        erase(begin(), end());
    }

    ~UnorderedMap() {
        clear();
    }
};
