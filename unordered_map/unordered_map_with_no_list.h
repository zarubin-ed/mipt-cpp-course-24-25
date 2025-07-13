#pragma once

#include <cmath>
#include <functional>
#include <iterator>
#include <stdexcept>

#ifdef DEBUG
#include <iostream>
#endif

template <typename Key, typename T, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, T> > >
class UnorderedMap {
public:
    using NodeType = std::pair<const Key, T>;

private:
    struct BaseNode {
        BaseNode* prev = this;
        BaseNode* next = this;

        void swap(BaseNode& other) {
            std::swap(next, other.next);
            std::swap(prev, other.prev);

            if (next == &other) {
                next = this;
                prev = this;
            } else {
                next->prev = this;
                prev->next = this;
            }

            if (other.next == this) {
                other.next = &other;
                other.prev = &other;
            } else {
                other.next->prev = &other;
                other.prev->next = &other;
            }
        }

        void reset() {
            prev = next = this;
        }
    };

    struct Node : BaseNode {
        size_t hash;
        NodeType data;

        Node()
            : BaseNode(),
              data() {
        }

        Node(const NodeType& value)
            : BaseNode(),
              data(value) {
        }

        Node(const Node& other)
            : BaseNode(),
              data(other.data) {
        }
    };

    template <bool IsConst>
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename std::conditional<IsConst, const NodeType, NodeType>::type;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        Iterator() = default;

        Iterator(const BaseNode* node)
            : node_(node) {
        }

        pointer operator->() const {
            return &operator*();
        }

        reference operator*() const {
            return reinterpret_cast<Node*>(const_cast<BaseNode*>(node_))->data;
        }

        Iterator& operator++() {
            node_ = node_->next;
            return *this;
        }

        Iterator& operator--() {
            node_ = node_->prev;
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++*this;
            return tmp;
        }

        Iterator operator--(int) {
            Iterator tmp = *this;
            --*this;
            return tmp;
        }

        operator Iterator<true>() const {
            return Iterator<true>(node_);
        }

        template <bool IsOtherConst>
        bool operator==(const Iterator<IsOtherConst>& other) const {
            return node_ == other.node_;
        }

    private:
        const BaseNode* node_ = nullptr;
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

    BaseNode fake_node_;
    std::vector<iterator, VectorAllocator> bucket_begins_;
    size_t size_;
    float max_load_factor_;
    [[no_unique_address]] Hash hash_;
    [[no_unique_address]] KeyEqual key_equal_;
    [[no_unique_address]] NodeAllocator node_alloc_;

    void swap_without_alloc(UnorderedMap& other) {
        fake_node_.swap(other.fake_node_);
        bucket_begins_.swap(other.bucket_begins_);
        std::swap(size_, other.size_);
        std::swap(max_load_factor_, other.max_load_factor_);
        std::swap(hash_, other.hash_);
        std::swap(key_equal_, other.key_equal_);
        std::swap(node_alloc_, other.node_alloc_);
    }

    template <typename... Args>
    Node* make_node(Args&&... args) {
        Node* new_node = NodeAllocTraits::allocate(node_alloc_, 1);
        try {
            NodeAllocTraits::construct(node_alloc_, std::addressof(new_node->data),
                                       std::forward<Args>(args)...);
            new_node->hash = hash_(new_node->data.first);
        } catch (...) {
            NodeAllocTraits::deallocate(node_alloc_, new_node, 1);
            throw;
        }
        return new_node;
    }

    void destroy_node(Node* node) {
        NodeAllocator node_alloc(node_alloc_);
        NodeAllocTraits::destroy(node_alloc_, node);
        NodeAllocTraits::deallocate(node_alloc, node, 1);
    }

    template <bool IsConst>
    bool is_same_bucket(Iterator<IsConst> it, size_t hash) const {
        if (it == iterator() || it == end()) {
            return false;
        }
        return reinterpret_cast<const Node*>(it.node_)->hash % bucket_count() ==
               hash % bucket_count();
    }

    iterator internal_find(const Key& key, size_t hash) const {
        iterator it = bucket_begins_[hash % bucket_count()];
        if (it == iterator()) {
            return iterator(&fake_node_);
        }
        while (is_same_bucket(it, hash)) {
            if (key_equal_(key, reinterpret_cast<const Node*>(it.node_)->data.first)) {
                return it;
            }
            ++it;
        }
        return it;
    }

    iterator insert_node(iterator pos, Node* new_node) {
        new_node->next = const_cast<BaseNode*>(pos.node_);
        new_node->prev = pos.node_->prev;

        pos.node_->prev->next = new_node;
        const_cast<BaseNode*>(pos.node_)->prev = new_node;

        auto result = iterator(static_cast<const BaseNode*>(new_node));

        if (!is_same_bucket(bucket_begins_[new_node->hash % bucket_count()], new_node->hash)) {
            bucket_begins_[new_node->hash % bucket_count()] = result;
        }

        return result;
    }

    size_t required_bucket_count() const {
        return static_cast<size_t>(size() / max_load_factor());
    }

    bool do_need_rehash() const {
        return max_load_factor() * bucket_count() < size();
    }

public:
    Allocator get_allocator() const {
        return node_alloc_;
    }

    UnorderedMap()
        : fake_node_(),
          bucket_begins_(1, iterator()),
          size_(0),
          max_load_factor_(1.0),
          hash_(),
          key_equal_(),
          node_alloc_() {
    }

    explicit UnorderedMap(size_type bucket_count, const Hash& hash = Hash(),
                          const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator())
        : fake_node_(),
          bucket_begins_(bucket_count, iterator(), alloc),
          size_(0),
          max_load_factor_(1.0),
          hash_(hash),
          key_equal_(equal),
          node_alloc_(alloc) {
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
        : UnorderedMap(other,
                       AllocTraits::select_on_container_copy_construction(other.node_alloc_)) {
    }

    UnorderedMap(UnorderedMap&& other, const Allocator& alloc)
        : fake_node_(),
          bucket_begins_(std::move(other.bucket_begins_)),
          size_(std::move(other.size_)),
          max_load_factor_(std::move(other.max_load_factor_)),
          hash_(std::move(other.hash_)),
          key_equal_(std::move(other.key_equal_)),
          node_alloc_(alloc) {
        fake_node_.swap(other.fake_node_);
        other.size_ = 0;
        other.max_load_factor_ = 1.0;
    }

    UnorderedMap(UnorderedMap&& other)
        : UnorderedMap(std::move(other), std::move(other.node_alloc_)) {
    }

    UnorderedMap& operator=(const UnorderedMap& other) {
        if (this == &other) {
            return *this;
        }
        UnorderedMap tmp(other, node_alloc_);
        swap_without_alloc(tmp);
        if constexpr (NodeAllocTraits::propagate_on_container_copy_assignment::value) {
            node_alloc_ = other.node_alloc_;
        }
        return *this;
    }

    UnorderedMap& operator=(UnorderedMap&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        bool is_fast_move = NodeAllocTraits::propagate_on_container_move_assignment::value ||
                            other.node_alloc_ == node_alloc_;
        if (is_fast_move) {
            UnorderedMap tmp(std::move(other));
            swap_without_alloc(tmp);
            if constexpr (NodeAllocTraits::propagate_on_container_move_assignment::value) {
                node_alloc_ = std::move(other.node_alloc_);
            }
        } else {
            UnorderedMap temp(std::move(other), node_alloc_);
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
            throw std::out_of_range("key not found");
        }
        return it->second;
    }

    T& at(const Key& key) {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("key not found");
        }
        return it->second;
    }

    size_type size() const noexcept {
        return size_;
    }

    size_type bucket_count() const noexcept {
        return bucket_begins_.size();
    }

    bool empty() const noexcept {
        return size_ == 0;
    }

    iterator end() {
        return iterator(&fake_node_);
    }

    iterator begin() {
        return ++end();
    }

    const_iterator cend() const {
        return const_iterator(&fake_node_);
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
        Node* new_node = make_node(std::forward<Args>(args)...);

        auto it = internal_find(new_node->data.first, new_node->hash);

        if (is_same_bucket(it, new_node->hash)) {
            destroy_node(new_node);
            return std::make_pair(it, false);
        }

        ++size_;
        auto result = insert_node(it, new_node);
        if (do_need_rehash()) {
            reserve(size_ * 2);
        }
        return std::make_pair(result, true);
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
        if (pos == cend()) {
            return end();
        }
        iterator iterator_to_return = iterator(pos.node_->next);

        Node* to_destroy = reinterpret_cast<Node*>(const_cast<BaseNode*>(pos.node_));

        if (bucket_begins_[to_destroy->hash % bucket_count()] == pos) {
            bucket_begins_[to_destroy->hash % bucket_count()] =
                is_same_bucket(iterator_to_return, to_destroy->hash) ? iterator_to_return
                                                                     : iterator();
        }

        pos.node_->prev->next = pos.node_->next;
        pos.node_->next->prev = pos.node_->prev;

        destroy_node(to_destroy);

        --size_;
        return iterator_to_return;
    }

    iterator erase(const_iterator lhs, const_iterator rhs) {
        const_iterator current = lhs;
        while (current != rhs) {
            current = erase(current);
        }
        return iterator(current.node_);
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
        iterator beg_it = begin();
        iterator end_it = end();

        fake_node_.reset();
        bucket_begins_ = std::vector<iterator, VectorAllocator>(new_bucket_count, iterator());

        while (beg_it != end_it) {
            iterator next = std::next(beg_it);
            Node* cur_node = reinterpret_cast<Node*>(const_cast<BaseNode*>(beg_it.node_));
            insert_node(internal_find(cur_node->data.first, cur_node->hash), cur_node);
            beg_it = next;
        }
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
            NodeAllocTraits::propagate_on_container_swap::value || other.node_alloc_ == node_alloc_;
        if (is_fast_swap) {
            fake_node_.swap(other.fake_node_);
            bucket_begins_.swap(other.bucket_begins_);
            std::swap(size_, other.size_);
            std::swap(max_load_factor_, other.max_load_factor_);
            std::swap(hash_, other.hash_);
            std::swap(key_equal_, other.key_equal_);
            if constexpr (NodeAllocTraits::propagate_on_container_swap::value) {
                std::swap(node_alloc_, other.node_alloc_);
            }
        } else {
            UnorderedMap temp_this(other, node_alloc_);
            UnorderedMap temp_other(other, node_alloc_);

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
