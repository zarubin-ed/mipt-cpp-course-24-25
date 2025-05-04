#include <functional>

template <typename T>
class WeakPtr;

template <typename T>
class EnableSharedFromThis;

namespace detail {
struct BaseControlBlock {
    size_t shared_count;
    size_t weak_count;

    BaseControlBlock()
        : shared_count(1),
          weak_count(0) {
    }

    virtual ~BaseControlBlock() = default;

    virtual void delete_object() = 0;

    virtual void delete_block() = 0;

    virtual void* get_object() const = 0;
};
}  // namespace detail

template <typename T>
class SharedPtr {
    template <typename U>
    friend class SharedPtr;

    template <typename U>
    friend class WeakPtr;

    template <typename U, typename Alloc, typename... Args>
    friend SharedPtr<U> allocateShared(const Alloc& allocator, Args&&... args);

private:
    detail::BaseControlBlock* control_block_p_ = nullptr;
    T* object_p_ = nullptr;

    template <typename U, typename Deleter, typename Allocator>
    struct StandartControlBlock : detail::BaseControlBlock {
        using StandartBlockAllocator =
            typename std::allocator_traits<Allocator>::template rebind_alloc<StandartControlBlock>;
        using StandartBlockAllocatorTraits = std::allocator_traits<StandartBlockAllocator>;

        U* object_p;
        [[no_unique_address]] Deleter deleter;
        [[no_unique_address]] StandartBlockAllocator allocator;

        StandartControlBlock(U* object_p, Deleter deleter, Allocator allocator)
            : object_p(object_p),
              deleter(deleter),
              allocator(allocator) {
        }

        void delete_object() override {
            deleter(object_p);
        }

        void delete_block() override {
            StandartBlockAllocator temp_allocator(allocator);

            this->~StandartControlBlock();

            StandartBlockAllocatorTraits::deallocate(temp_allocator, this, 1);
        }

        void* get_object() const override {
            return object_p;
        }

        ~StandartControlBlock() = default;
    };

    template <typename U, typename Allocator>
    struct MakeSharedControlBlock : detail::BaseControlBlock {
        using AllocatorTraits = typename std::allocator_traits<Allocator>;
        using MakeSharedBlockAllocator =
            typename AllocatorTraits::template rebind_alloc<MakeSharedControlBlock>;
        using MakeSharedBlockAllocatorTraits = std::allocator_traits<MakeSharedBlockAllocator>;
        alignas(alignof(U)) std::byte object_p[sizeof(U)];
        [[no_unique_address]] Allocator allocator;

        template <typename... Args>
        MakeSharedControlBlock(Allocator allocator, Args&&... args)
            : allocator(allocator) {
            AllocatorTraits::construct(allocator, reinterpret_cast<U*>(object_p),
                                       std::forward<Args>(args)...);
        }

        void delete_object() override {
            AllocatorTraits::destroy(allocator, reinterpret_cast<U*>(object_p));
        }

        void delete_block() override {
            MakeSharedBlockAllocator temp_allocator(allocator);

            this->~MakeSharedControlBlock();

            MakeSharedBlockAllocatorTraits::deallocate(temp_allocator, this, 1);
        }

        void* get_object() const override {
            return const_cast<U*>(reinterpret_cast<const U*>(object_p));
        }

        ~MakeSharedControlBlock() = default;
    };

    size_t& shared_count() {
        return control_block_p_->shared_count;
    }

    size_t& weak_count() {
        return control_block_p_->weak_count;
    }

    T* get_object() const {
        return object_p_          ? object_p_
               : control_block_p_ ? reinterpret_cast<T*>(control_block_p_->get_object())
                                  : nullptr;
    }

    void support_enable_shared_from_this() {
        if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
            if (get()->this_weak_ptr_.expired()) {
                get()->this_weak_ptr_ = *this;
            }
        }
    }

    void add_shared_pointer() {
        if (control_block_p_) {
            ++shared_count();
            support_enable_shared_from_this();
        }
    }

public:
    using element_type = std::remove_extent_t<T>;

    SharedPtr() noexcept = default;

    template <typename U>
    SharedPtr(const WeakPtr<U>& other)
        : control_block_p_(other.control_block_p_),
          object_p_(other.object_p_) {
        add_shared_pointer();
    }

    template <typename U, typename Deleter = std::default_delete<U>,
              typename Allocator = std::allocator<U> >
    SharedPtr(U* object_p, Deleter deleter = Deleter(), Allocator allocator = Allocator())
        : control_block_p_([object_p(object_p), allocator(allocator), deleter(deleter)] {
              using SCB = StandartControlBlock<U, Deleter, Allocator>;
              typename SCB::StandartBlockAllocator control_block_alloc(allocator);
              auto cbp = SCB::StandartBlockAllocatorTraits::allocate(control_block_alloc, 1);
              try {
                  new (cbp) SCB(object_p, deleter, allocator);
                  return cbp;
              } catch (...) {
                  SCB::StandartBlockAllocatorTraits::deallocate(control_block_alloc, cbp, 1);
                  throw;
              }
          }()),
          object_p_(object_p) {
        support_enable_shared_from_this();
    }

    SharedPtr(const SharedPtr& other)
        : control_block_p_(other.control_block_p_),
          object_p_(other.object_p_) {
        add_shared_pointer();
    }

    template <typename U>
    SharedPtr(const SharedPtr<U>& other)
        : control_block_p_(other.control_block_p_),
          object_p_(other.object_p_) {
        add_shared_pointer();
    }

    SharedPtr(SharedPtr&& other)
        : control_block_p_(other.control_block_p_),
          object_p_(other.object_p_) {
        add_shared_pointer();
        other.reset();
    }

    template <typename U>
    SharedPtr(SharedPtr<U>&& other)
        : control_block_p_(other.control_block_p_),
          object_p_(other.object_p_) {
        add_shared_pointer();
        other.reset();
    }

    SharedPtr& operator=(const SharedPtr& other) {
        if (&other == this) {
            return *this;
        }
        SharedPtr tmp(other);
        swap(tmp);
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        if (&other == this) {
            return *this;
        }
        reset();
        SharedPtr tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    template <typename U>
    SharedPtr(SharedPtr<U>& other, T* ptr) noexcept
        : control_block_p_(other.control_block_p_),
          object_p_(ptr) {
        add_shared_pointer();
    }

    ~SharedPtr() {
        reset();
    }

    size_t use_count() const noexcept {
        return control_block_p_->shared_count;
    }

    bool unique() const noexcept {
        return use_count() == 1;
    }

    element_type* get() const noexcept {
        return get_object();
    }

    void reset() noexcept {
        if (control_block_p_ == nullptr) {
            return;
        }
        bool block_has_been_deleted = false;
        if (shared_count() == 1) {
            control_block_p_->delete_object();
            if (weak_count() == 0) {
                block_has_been_deleted = true;
                control_block_p_->delete_block();
            }
        }
        if (!block_has_been_deleted) {
            --shared_count();  // for case then there is weak or shared pointers in object we are
                               // deleting
        }
        control_block_p_ = nullptr;
        object_p_ = nullptr;
    }

    template <typename U, typename Deleter = std::default_delete<U>,
              typename Allocator = std::allocator<U> >
    void reset(U* ptr, Deleter d = Deleter(), Allocator alloc = Allocator()) noexcept {
        *this = std::move(SharedPtr(ptr, d, alloc));
    }

    explicit operator bool() const noexcept {
        return control_block_p_ != nullptr;
    }

    void swap(SharedPtr& other) noexcept {
        std::swap(control_block_p_, other.control_block_p_);
        std::swap(object_p_, other.object_p_);
    }

    T& operator*() const noexcept {
        return *get();
    }

    T* operator->() const noexcept {
        return get();
    }

private:
    template <typename... Args, typename Allocator = std::allocator<T> >
    SharedPtr(Allocator allocator, Args&&... args) {
        using MSCB = MakeSharedControlBlock<T, Allocator>;
        typename MSCB::MakeSharedBlockAllocator control_block_alloc(allocator);
        auto cbp = MSCB::MakeSharedBlockAllocatorTraits::allocate(control_block_alloc, 1);
        try {
            new (cbp) MSCB(allocator, std::forward<Args>(args)...);
        } catch (...) {
            MSCB::MakeSharedBlockAllocatorTraits::deallocate(control_block_alloc, cbp, 1);
            throw;
        }
        control_block_p_ = cbp;
        object_p_ = get();
        support_enable_shared_from_this();
    }
};

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& allocator, Args&&... args) {
    return SharedPtr<T>(allocator, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
    return allocateShared<T>(std::allocator<T>(), std::forward<Args>(args)...);
}

template <typename T>
class WeakPtr {
    template <typename U>
    friend class WeakPtr;

    template <typename U>
    friend class SharedPtr;

private:
    detail::BaseControlBlock* control_block_p_ = nullptr;
    T* object_p_ = nullptr;

    size_t& weak_count() {
        return control_block_p_->weak_count;
    }

    size_t& shared_count() {
        return control_block_p_->shared_count;
    }

    void add_weak_pointer() {
        if (control_block_p_) {
            ++weak_count();
        }
    }

public:
    WeakPtr() noexcept = default;

    template <typename U>
    WeakPtr(const SharedPtr<U>& other)
        : control_block_p_(other.control_block_p_),
          object_p_(other.object_p_) {
        add_weak_pointer();
    }

    template <typename U>
    WeakPtr(SharedPtr<U>&& other)
        : control_block_p_(other.control_block_p_),
          object_p_(other.object_p_) {
        other.reset();
        add_weak_pointer();
    }

    WeakPtr(const WeakPtr& other)
        : control_block_p_(other.control_block_p_),
          object_p_(other.object_p_) {
        add_weak_pointer();
    }

    WeakPtr(WeakPtr&& other)
        : control_block_p_(other.control_block_p_),
          object_p_(other.object_p_) {
        other.reset();
        add_weak_pointer();
    }

    template <typename U>
    WeakPtr(const WeakPtr<U>& other)
        : control_block_p_(other.control_block_p_),
          object_p_(other.object_p_) {
        add_weak_pointer();
    }

    template <typename U>
    WeakPtr(WeakPtr<U>&& other)
        : control_block_p_(other.control_block_p_),
          object_p_(other.object_p_) {
        other.reset();
        add_weak_pointer();
    }

    WeakPtr& operator=(const WeakPtr& other) {
        if (this == &other) {
            return *this;
        }
        WeakPtr tmp(other);
        swap(tmp);
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) {
        if (this == &other) {
            return *this;
        }
        WeakPtr tmp(std::move(other));
        reset();
        swap(tmp);
        return *this;
    }

    size_t use_count() const noexcept {
        return control_block_p_ ? control_block_p_->shared_count : 0;
    }

    bool expired() const noexcept {
        return use_count() == 0;
    }

    SharedPtr<T> lock() const noexcept {
        return expired() ? SharedPtr<T>() : SharedPtr<T>(*this);
    }

    void reset() noexcept {
        if (control_block_p_ == nullptr) {
            return;
        }
        --weak_count();
        if (use_count() == 0 && weak_count() == 0) {
            control_block_p_->delete_block();
        }
        control_block_p_ = nullptr;
        object_p_ = nullptr;
    }

    void swap(WeakPtr& other) noexcept {
        std::swap(control_block_p_, other.control_block_p_);
        std::swap(object_p_, other.object_p_);
    }

    ~WeakPtr() {
        reset();
    }
};

template <typename T>
class EnableSharedFromThis {
    template <typename U>
    friend class SharedPtr;

private:
    WeakPtr<T> this_weak_ptr_;

public:
    SharedPtr<T> shared_from_this() {
        if (this_weak_ptr_.expired()) {
            throw std::bad_weak_ptr();
        }
        return this_weak_ptr_.lock();
    }
};
