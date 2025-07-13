#pragma once

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <utility>

class WrongVariantType : public std::exception {
public:
    char const* what() const noexcept override {
        return "Wrong variant type";
    }
};

static constexpr size_t kVariantNPos = -1;

template <typename... Types>
class Variant;

namespace detail {
template <size_t I, typename... Args>
struct VariantElement;

template <typename Head, typename... Tail>
struct VariantElement<0, Head, Tail...> {
    using type = Head;
};

template <size_t I, typename Head, typename... Tail>
struct VariantElement<I, Head, Tail...> {
    using type = typename VariantElement<I - 1, Tail...>::type;
};

template <typename... Types>
struct GetMaxSize;

template <>
struct GetMaxSize<> {
    static constexpr size_t kValue = 0;
};

template <typename Head, typename... Tail>
struct GetMaxSize<Head, Tail...> {
    static constexpr size_t kValue = std::max(sizeof(Head), GetMaxSize<Tail...>::kValue);
};

template <typename Temp, typename... Args>
struct TypeCount;

template <typename Temp>
struct TypeCount<Temp> {
    static constexpr size_t kValue = 0;
};

template <typename Temp, typename Head, typename... Tail>
struct TypeCount<Temp, Head, Tail...> {
    static constexpr size_t kValue =
        (std::is_same_v<Temp, Head> ? 0 : 1 + TypeCount<Temp, Tail...>::kValue);
};

template <typename T, typename... Types>
class VariantAlternative;

template <typename T, typename... Types>
class VariantAlternative {
public:
    static constexpr std::integral_constant<size_t, TypeCount<T, Types...>::kValue> getIndex(T&&);

    static constexpr std::integral_constant<size_t, TypeCount<T, Types...>::kValue> getIndex(
        const T&);

    void call_destructor() noexcept {
        auto* self = static_cast<Variant<Types...>*>(this);
        if (self->index() == TypeCount<T, Types...>::kValue) {
            std::destroy_at(reinterpret_cast<T*>(self->data_));
        }
    }
};

struct Swapper {
    template <typename T>
    void operator()(T& lhs, T& rhs) {
        std::swap(lhs, rhs);
    }
};

template <size_t I, typename... Types>
decltype(auto) get_impl(auto&&);

template <size_t I, typename... Types>
decltype(auto) get_impl(auto&& variant) {
    using T = typename VariantElement<I, Types...>::type;
    if (!holds_alternative<T>(variant)) {
        throw WrongVariantType();
    }

    using RawT = std::remove_reference_t<T>;
    using VariantLike = decltype(variant);

    using QualifiedPtr = std::conditional_t<std::is_const_v<std::remove_reference_t<VariantLike>>,
                                            const RawT*, RawT*>;

    return std::forward_like<VariantLike>(*reinterpret_cast<QualifiedPtr>(variant.data_));
}

template <typename>
struct VariantSize;

template <typename... Types>
struct VariantSize<Variant<Types...>> {
    static constexpr size_t kValue = sizeof...(Types);
};

template <size_t I, size_t... Sequence, size_t... Indexes>
decltype(auto) make_zero(std::index_sequence<Sequence...>, std::index_sequence<Indexes...>) {
    return std::index_sequence<(Indexes == I ? 0 : Sequence)...>();
}

template <size_t I, size_t... Sequence, size_t... Indexes>
decltype(auto) increase_by_one(std::index_sequence<Sequence...>, std::index_sequence<Indexes...>) {
    return std::index_sequence<(Indexes == I ? Sequence + 1 : Sequence)...>();
}

template <size_t I, typename... Variants, size_t... Sequence, size_t... Indexes>
    requires(!((Sequence == VariantSize<std::remove_cvref_t<Variants>>::kValue) && ...) &&
             ((I != Indexes || VariantSize<std::remove_cvref_t<Variants>>::kValue > Sequence + 1) &&
              ...))
decltype(auto) get_next_permutation(std::index_sequence<Sequence...> cur,
                                    std::index_sequence<Indexes...> inds) {
    return increase_by_one<I>(cur, inds);
}

template <size_t I, typename... Variants, size_t... Sequence, size_t... Indexes>
    requires(!((Sequence == VariantSize<std::remove_cvref_t<Variants>>::kValue) && ...) &&
             !((I != Indexes ||
                VariantSize<std::remove_cvref_t<Variants>>::kValue > Sequence + 1) &&
               ...))
decltype(auto) get_next_permutation(std::index_sequence<Sequence...> cur,
                                    std::index_sequence<Indexes...> inds) {
    return get_next_permutation<I + 1, Variants...>(make_zero<I>(cur, inds), inds);
}

template <typename... Variants, size_t... Indexes, size_t... Sequence>
    requires(((Sequence == VariantSize<std::remove_cvref_t<Variants>>::kValue) && ...))
decltype(auto) get_next_permutation(std::index_sequence<Sequence...>,
                                    std::index_sequence<Indexes...>) {
    return std::index_sequence<>();
}

template <typename R, size_t... Sequence, typename Visitor, typename... Variants>
R visit_impl(std::index_sequence<Sequence...> cur, Visitor&& visitor, Variants&&... variants) {
    if constexpr (std::invocable<Visitor,
                                 decltype(get<Sequence>(std::forward<Variants>(variants)))...>) {
        if (((Sequence == variants.index()) && ...)) {
            return std::invoke(std::forward<Visitor>(visitor),
                               get<Sequence>(std::forward<Variants>(variants))...);
        }
    }
    using index_sequence = std::make_index_sequence<sizeof...(Variants)>;
    return visit_impl<R>(detail::get_next_permutation<0, Variants...>(cur, index_sequence()),
                         std::forward<Visitor>(visitor), std::forward<Variants>(variants)...);
}

template <typename>
struct ConstantZero {
    static constexpr size_t kValue = 0;
};

}  // namespace detail

template <typename T, typename... Types>
bool holds_alternative(const Variant<Types...>&);

template <typename R, typename Visitor, typename... Variants>
R visit(Visitor&& visitor, Variants&&... variants);

template <typename Visitor, typename... Variants>
decltype(auto) visit(Visitor&& visitor, Variants&&... variants);

template <typename... Types>
class Variant : private detail::VariantAlternative<Types, Types...>... {
private:
    template <size_t I, typename... UTypes>
    friend decltype(auto) detail::get_impl(auto&&);

    template <typename T, typename... UTypes>
    friend class detail::VariantAlternative;

    alignas(std::max_align_t) std::byte data_[detail::GetMaxSize<Types...>::kValue];
    size_t type_index_ = kVariantNPos;

    using detail::VariantAlternative<Types, Types...>::getIndex...;

    void deleter() noexcept {
        (static_cast<detail::VariantAlternative<Types, Types...>*>(this)->call_destructor(), ...);
        type_index_ = kVariantNPos;
    }

public:
    Variant() noexcept(
        std::is_nothrow_default_constructible_v<typename detail::VariantElement<0, Types...>::type>)
        requires(
            std::is_default_constructible_v<typename detail::VariantElement<0, Types...>::type>)
    {
        emplace<0>();
    }

    template <typename T>
    Variant(T&& val)
        requires(decltype(getIndex(std::declval<T>()))::value < sizeof...(Types))
    {
        constexpr size_t kIndexOfType = decltype(getIndex(std::declval<T>()))::value;
        emplace<kIndexOfType>(std::forward<T>(val));
    }

    void swap(Variant& other) noexcept((std::is_nothrow_swappable_v<Types> && ...)) {
        if (index() == other.index()) {
            visit(detail::Swapper(), *this, other);
        } else {
            std::swap(other, *this);
        }
    }

    template <typename T>
    Variant& operator=(T&& val)
        requires(decltype(getIndex(val))::value < sizeof...(Types))
    {
        Variant tmp(std::forward<T>(val));
        swap(tmp);
        return *this;
    }

    template <typename T, typename... Args>
        requires(std::is_constructible_v<T, Args...>)
    T& emplace(Args&&... args) {
        deleter();
        std::construct_at(reinterpret_cast<T*>(data_), std::forward<Args>(args)...);
        type_index_ = detail::TypeCount<T, Types...>::kValue;
        return *reinterpret_cast<T*>(data_);
    }

    template <typename T, typename U, typename... Args>
        requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>)
    T& emplace(std::initializer_list<U> il, Args&&... args) {
        deleter();
        std::construct_at(reinterpret_cast<T*>(data_), il, std::forward<Args>(args)...);
        type_index_ = detail::TypeCount<T, Types...>::kValue;
        return *reinterpret_cast<T*>(data_);
    }

    template <size_t I, typename... Args>
        requires(
            std::is_constructible_v<typename detail::VariantElement<I, Types...>::type, Args...>)
    typename detail::VariantElement<I, Types...>::type& emplace(Args&&... args) {
        return emplace<typename detail::VariantElement<I, Types...>::type>(
            std::forward<Args>(args)...);
    }

    template <size_t I, typename U, typename... Args>
        requires(std::is_constructible_v<typename detail::VariantElement<I, Types...>::type,
                                         std::initializer_list<U>&, Args...>)
    typename detail::VariantElement<I, Types...>::type& emplace(std::initializer_list<U> il,
                                                                Args&&... args) {
        return emplace<typename detail::VariantElement<I, Types...>::type>(
            il, std::forward<Args>(args)...);
    }

    bool valueless_by_exception() const noexcept {
        return index() == kVariantNPos;
    }

    size_t index() const noexcept {
        return type_index_;
    }

    Variant(const Variant& other) {
        if (!other.valueless_by_exception()) {
            visit([this]<typename T>(const T& val) { this->template emplace<T>(val); }, other);
        }
    }

    Variant(Variant&& other) noexcept {
        if (!other.valueless_by_exception()) {
            visit([this]<typename T>(T&& val) { this->template emplace<T>(std::move(val)); },
                  std::move(other));
        }
    }

    Variant& operator=(Variant&& other) noexcept {
        if (other.valueless_by_exception()) {
            deleter();
            return *this;
        }
        if (index() == other.index()) {
            visit([]<typename T>(T& lhs, T&& rhs) { lhs = std::move(rhs); }, *this,
                  std::move(other));
        } else {
            deleter();
            visit([this]<typename T>(T&& val) { this->template emplace<T>(std::move(val)); },
                  std::move(other));
        }
        return *this;
    }

    Variant& operator=(const Variant& other) {
        if (other.valueless_by_exception()) {
            deleter();
            return *this;
        }
        if (index() == other.index()) {
            visit([]<typename T>(T& lhs, const T& rhs) { lhs = rhs; }, *this, other);
        } else {
            deleter();
            visit([this]<typename T>(const T& val) { this->template emplace<T>(val); }, other);
        }
        return *this;
    }

    ~Variant() {
        deleter();
    }
};

template <typename T, typename... Types>
bool holds_alternative(const Variant<Types...>& variant) {
    return detail::TypeCount<T, Types...>::kValue == variant.index();
}

template <size_t I, typename... Types>
typename detail::VariantElement<I, Types...>::type& get(Variant<Types...>& variant) {
    return detail::get_impl<I, Types...>(variant);
}

template <typename T, typename... Types>
T& get(Variant<Types...>& variant) {
    return get<detail::TypeCount<T, Types...>::kValue>(variant);
}

template <size_t I, typename... Types>
const typename detail::VariantElement<I, Types...>::type& get(const Variant<Types...>& variant) {
    return detail::get_impl<I, Types...>(variant);
}

template <typename T, typename... Types>
const T& get(const Variant<Types...>& variant) {
    return get<detail::TypeCount<T, Types...>::kValue>(variant);
}

template <size_t I, typename... Types>
typename detail::VariantElement<I, Types...>::type&& get(Variant<Types...>&& variant) {
    return detail::get_impl<I, Types...>(std::move(variant));
}

template <typename T, typename... Types>
T&& get(Variant<Types...>&& variant) {
    return get<detail::TypeCount<T, Types...>::kValue>(std::move(variant));
}

template <size_t I, typename... Types>
const typename detail::VariantElement<I, Types...>::type&& get(const Variant<Types...>&& variant) {
    return detail::get_impl<I, Types...>(std::move(variant));
}

template <typename T, typename... Types>
const T&& get(const Variant<Types...>&& variant) {
    return get<detail::TypeCount<T, Types...>::kValue>(std::move(variant));
}

template <typename R, typename Visitor, typename... Variants>
R visit(Visitor&& visitor, Variants&&... variants) {
    if ((variants.valueless_by_exception() || ...)) {
        throw WrongVariantType();
    }
    return detail::visit_impl<R>(std::index_sequence<detail::ConstantZero<Variants>::kValue...>(),
                                 std::forward<Visitor>(visitor),
                                 std::forward<Variants>(variants)...);
}

template <typename Visitor, typename... Variants>
decltype(auto) visit(Visitor&& visitor, Variants&&... variants) {
    using ReturnType = decltype(std::invoke(std::forward<Visitor>(visitor),
                                            get<0>(std::forward<Variants>(variants))...));
    return detail::visit_impl<ReturnType>(
        std::index_sequence<detail::ConstantZero<Variants>::kValue...>(),
        std::forward<Visitor>(visitor), std::forward<Variants>(variants)...);
}
