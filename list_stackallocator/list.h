#include <cstddef>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>

template <size_t N>
class StackStorage {
public:
    StackStorage() = default;

    StackStorage(const StackStorage&) = delete;

    std::byte* getMemory(size_t bytes, size_t alignment) {
        void* current_pointer = static_cast<void*>(bytes_ + N - space_);
        void* result = std::align(alignment, bytes, current_pointer, space_);
        if (result == nullptr) {
            throw std::bad_alloc();
        }
        space_ -= bytes;
        return static_cast<std::byte*>(result);
    }

private:
    alignas(std::max_align_t) std::byte bytes_[N];
    size_t space_ = N;
};

template <typename T, size_t N>
class StackAllocator {
public:
    using value_type = T;

    void deallocate(T*, size_t) const noexcept {
    }

    T* allocate(size_t n) const {
        return reinterpret_cast<T*>(storage_->getMemory(n * sizeof(T), alignof(T)));
    }

    StackAllocator(StackStorage<N>& storage)
        : storage_(&storage) {
    }

    const StackStorage<N>* get_storage() const noexcept {
        return storage_;
    }

    template <typename U>
    StackAllocator(const StackAllocator<U, N>& other)
        : storage_(other.storage_) {
    }

    template <typename U>
    struct rebind {
        using other = StackAllocator<U, N>;
    };

private:
    StackStorage<N>* storage_;

    template <typename U, size_t M>
    friend class StackAllocator;
};

template <typename T1, typename T2, size_t N>
bool operator==(const StackAllocator<T1, N>& lhs, const StackAllocator<T2, N>& rhs) noexcept {
    return lhs.get_storage() == rhs.get_storage();
}

template <typename T, typename Allocator = std::allocator<T>>
class List {
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
    };

    struct Node : BaseNode {
        T data;

        Node()
            : BaseNode(),
              data() {
        }

        Node(const T& value)
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
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = typename std::conditional<IsConst, const T, T>::type;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        Iterator(const BaseNode* node)
            : node_(node) {
        }

        Iterator() = default;

        pointer operator->() const {
            return &operator*();
        }

        reference operator*() const {
            return static_cast<Node*>(const_cast<BaseNode*>(node_))->data;
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

        bool operator==(const Iterator& other) const = default;

    protected:
        const BaseNode* node_;
        friend class List;
    };

public:
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using value_type = typename iterator::value_type;
    using reference = typename iterator::reference;
    using const_reference = const reference;
    using size_type = std::size_t;
    using allocator_type =
        typename std::allocator_traits<Allocator>::template rebind_alloc<value_type>;

private:
    BaseNode fake_node_;
    size_t size_ = 0;

    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using AllocTraits = std::allocator_traits<NodeAllocator>;

    [[no_unique_address]] NodeAllocator node_allocator_;

    void swap_without_alloc(List& other) {
        fake_node_.swap(other.fake_node_);
        std::swap(size_, other.size_);
        std::swap(node_allocator_, other.node_allocator_);
    }

    void delete_node(Node* node) {
        AllocTraits::destroy(node_allocator_, node);
        AllocTraits::deallocate(node_allocator_, node, 1);
    }

    iterator insert_pointer(const_iterator pos, BaseNode* node) {
        node->next = const_cast<BaseNode*>(pos.node_);
        node->prev = pos.node_->prev;
        pos.node_->prev->next = node;
        const_cast<BaseNode*>(pos.node_)->prev = node;
        ++size_;
        return iterator(node);
    }

public:
    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        Node* new_node = AllocTraits::allocate(
            node_allocator_, 1);  // ether throws and we have to do nothing or everything is ok
        try {
            AllocTraits::construct(
                node_allocator_, new_node,
                std::forward<Args>(args)...);  // if throws we mustn't call destructor for new_node
        } catch (...) {
            AllocTraits::deallocate(node_allocator_, new_node, 1);
            throw;
        }
        return insert_pointer(pos, new_node);
    }

    void splice(const_iterator pos, List& other, const_iterator it) {
        it.node_->prev->next = it.node_->next;
        it.node_->next->prev = it.node_->prev;
        insert_pointer(pos, const_cast<BaseNode*>(it.node_));

        --other.size_;
    }

    List() = default;

    List(const Allocator& allocator)
        : size_(0),
          node_allocator_(allocator) {
    }

    List(size_t size, const Allocator& allocator = Allocator())
        : List(allocator) {
        for (size_t i = 0; i < size; ++i) {
            emplace(cend());
        }
    }

    List(size_t size, const T& value, const Allocator& allocator = Allocator())
        : List(allocator) {
        for (size_t i = 0; i < size; ++i) {
            emplace(cend(), value);
        }
    }

    List(const List& other)
        : List(other, AllocTraits::select_on_container_copy_construction(other.node_allocator_)) {
    }

    List(List&& other)
        : size_(std::move(other.size_)),
          node_allocator_(other.node_allocator_) {
        fake_node_.swap(other.fake_node_);
        other.size_ = 0;
    }

    List(const List& other, const Allocator& allocator)
        : List(allocator) {
        for (const_iterator it = other.begin(); it != other.end(); ++it) {
            push_back(*it);
        }
    }

    ~List() {
        clear();
    }

    iterator erase(const_iterator pos) {
        iterator iterator_to_return = iterator(pos.node_->next);

        Node* to_destroy = static_cast<Node*>(const_cast<BaseNode*>(pos.node_));

        pos.node_->prev->next = pos.node_->next;
        pos.node_->next->prev = pos.node_->prev;

        delete_node(to_destroy);

        --size_;
        return iterator_to_return;
    }

    iterator erase(const_iterator bg, const_iterator ed) {
        iterator result;
        while (bg != ed) {
            bg = result = erase(bg);
        }
        return result;
    }

    void clear() {
        erase(begin(), end());
    }

    void swap(List& other) noexcept(AllocTraits::propagate_on_container_swap::value) {
        bool is_fast_swap = AllocTraits::propagate_on_container_swap::value ||
                            other.node_allocator_ == node_allocator_;
        if (is_fast_swap) {
            std::swap(size_, other.size_);
            fake_node_.swap(other.fake_node_);
            if constexpr (AllocTraits::propagate_on_container_swap::value) {
                std::swap(node_allocator_, other.node_allocator_);
            }
        } else {
            List temp_this(*this, node_allocator_);
            List temp_other(other, node_allocator_);

            swap_without_alloc(temp_this);
            other.swap_without_alloc(temp_other);
        }
    }

    List& operator=(const List& other) {
        if (this == &other) {
            return *this;
        }
        List tmp(other, node_allocator_);
        swap_without_alloc(tmp);
        if constexpr (AllocTraits::propagate_on_container_copy_assignment::value) {
            node_allocator_ = other.node_allocator_;
        }
        return *this;
    }

    List& operator=(List&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        bool is_fast_move = AllocTraits::propagate_on_container_move_assignment::value ||
                            other.node_allocator_ == node_allocator_;
        if (is_fast_move) {
            List tmp(std::move(other));
            swap_without_alloc(tmp);
            if constexpr (AllocTraits::propagate_on_container_move_assignment::value) {
                node_allocator_ = std::move(other.node_allocator_);
            }
        } else {
            List temp(std::move(other), node_allocator_);
            swap_without_alloc(temp);
        }
        return *this;
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

    const_iterator end() const {
        return cend();
    }

    const_iterator begin() const {
        return cbegin();
    }

    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(cbegin());
    }

    const_reverse_iterator rbegin() const {
        return crbegin();
    }

    const_reverse_iterator rend() const {
        return crend();
    }

    allocator_type get_allocator() const noexcept {
        return node_allocator_;
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    iterator insert(const_iterator pos, const T& value) {
        return emplace(pos, value);
    }

    void push_back(const T& value) {
        insert(cend(), value);
    }

    void pop_back() {
        erase(--cend());
    }

    void push_front(const T& value) {
        insert(cbegin(), value);
    }

    void pop_front() {
        erase(cbegin());
    }
};
