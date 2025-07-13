#pragma once

#include <functional>
#include <iostream>
#include <type_traits>
#include <typeinfo>

#include "function.h"

namespace detail {
template <typename F>
struct FunctionalSignature {
    using type = FunctionalSignature<decltype(&F::operator())>::type;
};

template <typename ReturnType, typename... Args>
struct FunctionalSignature<ReturnType(Args...)> {
    using type = ReturnType(Args...);
};

template <typename ReturnType, typename... Args>
struct FunctionalSignature<ReturnType (*)(Args...)> {
    using type = ReturnType(Args...);
};

template <typename T, typename ReturnType, typename... Args>
struct FunctionalSignature<ReturnType (T::*)(Args...) const> {
    using type = ReturnType(Args...);
};

template <typename T, typename ReturnType, typename... Args>
struct FunctionalSignature<ReturnType (T::*)(Args...)> {
    using type = ReturnType(Args...);
};

template <typename F, typename Function, typename... Args>
concept is_function_constructible_from = (!std::is_same_v<std::remove_cvref_t<F>, Function>) &&
                                         std::is_invocable_v<std::remove_cvref_t<F>, Args...>;
}  // namespace detail

template <typename F>
using functional_signature_t = typename detail::FunctionalSignature<std::remove_cvref_t<F>>::type;

namespace function_impl {
enum class FunctionMethod { Destroy, Copy, Move, TypeInfo };

template <bool>
struct ControlBlock;

template <bool, typename>
class FunctionImpl;

template <bool IsMoveOnly, typename ReturnType, typename... Args>
class FunctionImpl<IsMoveOnly, ReturnType(Args...)> {
private:
    using CallMethodType = const std::type_info& (*)(void*, void*, const void*, bool,
                                                     FunctionMethod);
    template <typename F>
    struct FunctionWrapper {

        static const std::type_info& call_method(void* fptr, void* other, const void* c_other,
                                                 bool do_delete, FunctionMethod flag) {
            switch (flag) {
                case FunctionMethod::Destroy:
                    if (do_delete) {
                        delete static_cast<F*>(fptr);
                    } else {
                        static_cast<F*>(fptr)->~F();
                    }
                    break;
                case FunctionMethod::Copy:
                    if constexpr (!IsMoveOnly) {
                        new (static_cast<F*>(fptr)) F(*static_cast<const F*>(c_other));
                    }
                    break;
                case FunctionMethod::Move:
                    new (static_cast<F*>(fptr)) F(std::move(*static_cast<F*>(other)));
                    break;
                default:
                    break;
            }
            return typeid(F);
        }

        static ReturnType invoker(void* fptr, Args... args) {
            return std::invoke(*static_cast<F*>(fptr), std::forward<Args>(args)...);
        }

        inline static CallMethodType call_method_p = &call_method;
    };

    static constexpr size_t kBufferSize = 16;

    alignas(std::max_align_t) std::byte buffer_[kBufferSize];
    CallMethodType cmp_ = nullptr;
    void* fptr_ = nullptr;
    ReturnType (*invoker_)(void*, Args...) = nullptr;

    template <typename F>
    static constexpr bool kUseBufferCondition =
        sizeof(F) <= kBufferSize && std::is_trivially_copyable_v<F>;

    bool is_in_buffer() const {
        return fptr_ == buffer_;
    }

    void destroy_self() {
        if (operator bool()) {
            cmp_(fptr_, nullptr, nullptr, !is_in_buffer(), FunctionMethod::Destroy);
            fptr_ = nullptr;
        }
    }

public:
    template <typename F>
    FunctionImpl(F&& f)
        requires detail::is_function_constructible_from<F, FunctionImpl, Args...>
    {
        using PureF = std::remove_reference_t<F>;
        if constexpr (std::is_function_v<PureF>) {
            invoker_ = &FunctionWrapper<PureF*>::invoker;
            cmp_ = FunctionWrapper<PureF*>::call_method_p;
            new (buffer_)(PureF*)(&f);
            fptr_ = buffer_;
        } else {
            invoker_ = &FunctionWrapper<PureF>::invoker;
            cmp_ = FunctionWrapper<PureF>::call_method_p;
            if constexpr (kUseBufferCondition<PureF>) {
                new (buffer_) PureF(std::forward<F>(f));
                fptr_ = buffer_;
            } else {
                fptr_ = new PureF(std::forward<F>(f));
            }
        }
    }

    FunctionImpl() = default;

    FunctionImpl(nullptr_t)
        : FunctionImpl() {
    }

    ReturnType operator()(Args... args) const {
        if (!operator bool()) {
            throw std::bad_function_call();
        }
        return invoker_(fptr_, std::forward<Args>(args)...);
    }

    FunctionImpl(FunctionImpl&& other)
        : cmp_(other.cmp_),
          invoker_(other.invoker_) {
        if (other.is_in_buffer()) {
            cmp_(buffer_, other.buffer_, nullptr, false, FunctionMethod::Move);
            fptr_ = buffer_;
        } else {
            fptr_ = other.fptr_;
        }
        other.fptr_ = nullptr;
    }

    FunctionImpl(const FunctionImpl& other)
        requires(!IsMoveOnly)
        : cmp_(other.cmp_),
          invoker_(other.invoker_) {
        if (other.is_in_buffer()) {
            cmp_(buffer_, nullptr, other.buffer_, false, FunctionMethod::Copy);
            fptr_ = buffer_;
        } else {
            cmp_(fptr_, nullptr, other.fptr_, false, FunctionMethod::Copy);
        }
    }

    void swap(FunctionImpl& other) noexcept {
        std::swap(invoker_, other.invoker_);
        std::swap(cmp_, other.cmp_);
        std::swap(fptr_, other.fptr_);
        std::swap_ranges(buffer_, buffer_ + kBufferSize, other.buffer_);
        if (fptr_ == other.buffer_) {
            fptr_ = buffer_;
        }
        if (other.fptr_ == buffer_) {
            other.fptr_ = other.buffer_;
        }
    }

    FunctionImpl& operator=(FunctionImpl&& other) {
        if (this == &other) {
            return *this;
        }
        FunctionImpl tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    FunctionImpl& operator=(const FunctionImpl& other)
        requires(!IsMoveOnly)
    {
        if (this == &other) {
            return *this;
        }
        FunctionImpl tmp(other);
        swap(tmp);
        return *this;
    }

    template <typename F>
    FunctionImpl& operator=(F&& f)
        requires detail::is_function_constructible_from<F, FunctionImpl, Args...>
    {
        FunctionImpl(std::forward<F>(f)).swap(*this);
        return *this;
    }

    template <typename F>
    FunctionImpl& operator=(std::reference_wrapper<F> f)
        requires detail::is_function_constructible_from<F, FunctionImpl, Args...>
    {
        FunctionImpl(f).swap(*this);
        return *this;
    }

    operator bool() const {
        return fptr_ != nullptr;
    }

    void* target() {
        return fptr_;
    }

    const std::type_info& target_type() const noexcept {
        return operator bool() ? cmp_(nullptr, nullptr, nullptr, false, FunctionMethod::TypeInfo)
                               : typeid(void);
    }

    ~FunctionImpl() {
        destroy_self();
    }
};

}  // namespace function_impl

template <typename F>
class Function : public function_impl::FunctionImpl<false, F> {
    using function_impl::FunctionImpl<false, F>::FunctionImpl;
};

template <class ReturnType, class... Args>
bool operator==(const Function<ReturnType(Args...)>& f, std::nullptr_t) noexcept {
    return !f.operator bool();
}

template <typename F>
Function(F&& f) -> Function<functional_signature_t<F>>;

template <typename F>
class MoveOnlyFunction : public function_impl::FunctionImpl<true, F> {
    using function_impl::FunctionImpl<true, F>::FunctionImpl;
};

template <class ReturnType, class... Args>
bool operator==(const MoveOnlyFunction<ReturnType(Args...)>& f, std::nullptr_t) noexcept {
    return !f.operator bool();
}

template <typename F>
MoveOnlyFunction(F&& f) -> MoveOnlyFunction<functional_signature_t<F>>;