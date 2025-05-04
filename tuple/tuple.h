#pragma once

#include <cmath>
#include <functional>
#include <iterator>
#include <stdexcept>

template <typename... Args>
class Tuple;

struct internal_construct_tag {};

namespace detail {
/// IsCopyListInitializable
template <typename>
struct IsCopyListInitializable {
    static constexpr bool kValue = false;
};

template <typename T>
    requires requires(T t) { t = {}; }
struct IsCopyListInitializable<T> {
    static constexpr bool kValue = true;
};

/// Tuple Element
template <size_t I, typename T>
struct TupleElement;

template <typename Head, typename... Tail>
struct TupleElement<0, Tuple<Head, Tail...>> {
    using type = Head;
};

template <size_t I, typename Head, typename... Tail>
struct TupleElement<I, Tuple<Head, Tail...>> {
    using type = TupleElement<I - 1, Tuple<Tail...>>::type;
};

template <size_t I, typename... Args>
struct TupleElement<I, const Tuple<Args...>> {
    using type = std::add_const<typename TupleElement<I, std::tuple<Args...>>::type>::type;
};

/// count types
template <typename Temp, typename... Args>
struct TypeCount;

template <typename Temp>
struct TypeCount<Temp> {
    static constexpr size_t kValue = 0;
};

template <typename Temp, typename Head, typename... Tail>
struct TypeCount<Temp, Head, Tail...> {
    static constexpr size_t kValue =
        (std::is_same_v<Temp, Head> ? 1 : 0) + TypeCount<Temp, Tail...>::kValue;
};
}  // namespace detail

template <size_t I, typename T>
using TupleElementType = typename detail::TupleElement<I, T>::type;

template <typename T>
constexpr bool kIsCopyListInitializableV = detail::IsCopyListInitializable<T>::kValue;

template <typename... Args>
constexpr bool kMeetOnlyOnce = detail::TypeCount<Args...>::kValue == 1;

template <size_t I, typename... Args>
TupleElementType<I, Tuple<Args...>>& get(Tuple<Args...>& tuple) {
    if constexpr (I == 0) {
        return tuple.head_;
    } else {
        return get<I - 1>(tuple.tail_);
    }
}

template <size_t I, typename... Args>
const TupleElementType<I, Tuple<Args...>>& get(const Tuple<Args...>& tuple) {
    if constexpr (I == 0) {
        return tuple.head_;
    } else {
        return get<I - 1>(tuple.tail_);
    }
}

template <size_t I, typename... Args>
TupleElementType<I, Tuple<Args...>>&& get(Tuple<Args...>&& tuple) {
    if constexpr (I == 0) {
        return tuple.head_;
    } else {
        return get<I - 1>(tuple.tail_);
    }
}

template <size_t I, typename... Args>
const TupleElementType<I, Tuple<Args...>>&& get(const Tuple<Args...>&& tuple) {
    if constexpr (I == 0) {
        return tuple.head_;
    } else {
        return get<I - 1>(tuple.tail_);
    }
}

template <typename Temp, typename Head, typename... Tail>
    requires kMeetOnlyOnce<Temp, Head, Tail...>
constexpr const Temp& get(const Tuple<Head, Tail...>& tuple) {
    if constexpr (std::is_same_v<Temp, Head>) {
        return tuple.head_;
    } else {
        return get<Temp>(tuple.tail_);
    }
}

template <typename Temp, typename Head, typename... Tail>
    requires kMeetOnlyOnce<Temp, Head, Tail...>
constexpr Temp&& get(Tuple<Head, Tail...>&& tuple) {
    if constexpr (std::is_same_v<Temp, Head>) {
        return tuple.head_;
    } else {
        return get<Temp>(tuple.tail_);
    }
}

template <typename Temp, typename Head, typename... Tail>
    requires kMeetOnlyOnce<Temp, Head, Tail...>
constexpr const Temp&& get(const Tuple<Head, Tail...>&& tuple) {
    if constexpr (std::is_same_v<Temp, Head>) {
        return tuple.head_;
    } else {
        return get<Temp>(tuple.tail_);
    }
}

template <typename Temp, typename Head, typename... Tail>
    requires kMeetOnlyOnce<Temp, Head, Tail...>
constexpr Temp& get(Tuple<Head, Tail...>& tuple) {
    if constexpr (std::is_same_v<Temp, Head>) {
        return tuple.head_;
    } else {
        return get<Temp>(tuple.tail_);
    }
}
template <>
class Tuple<> {};

template <typename...>
struct getCatType;

// template <>
// struct getCatType<> {
//     using type = Tuple<>;
// };

template <typename T>
struct getCatType<T> {
    using type = T;
};

template <typename... Args1, typename... Args2, typename... Tuples>
struct getCatType<Tuple<Args1...>, Tuple<Args2...>, Tuples...> {
    using type = typename getCatType<Tuple<Args1..., Args2...>, Tuples...>::type;
};

template <typename Head, typename... Tail>
class Tuple<Head, Tail...> {
    Head head_;
    [[no_unique_address]] Tuple<Tail...> tail_;

    template <typename... Args>
    friend class Tuple;

    template <size_t I, typename... Args>
    friend TupleElementType<I, Tuple<Args...>>& get(Tuple<Args...>& tuple);

    template <size_t I, typename... Args>
    friend const TupleElementType<I, Tuple<Args...>>& get(const Tuple<Args...>& tuple);

    template <size_t I, typename... Args>
    friend TupleElementType<I, Tuple<Args...>>&& get(Tuple<Args...>&& tuple);

    template <size_t I, typename... Args>
    friend const TupleElementType<I, Tuple<Args...>>&& get(const Tuple<Args...>&& tuple);

    template <typename Temp, typename UHead, typename... UTail>
        requires kMeetOnlyOnce<Temp, UHead, UTail...>
    friend constexpr Temp& get(Tuple<UHead, UTail...>& tuple);

    template <typename Temp, typename UHead, typename... UTail>
        requires kMeetOnlyOnce<Temp, UHead, UTail...>
    friend constexpr const Temp& get(const Tuple& tuple);

    template <typename Temp, typename UHead, typename... UTail>
        requires kMeetOnlyOnce<Temp, UHead, UTail...>
    friend constexpr Temp&& get(Tuple&& tuple);

    template <typename Temp, typename UHead, typename... UTail>
        requires kMeetOnlyOnce<Temp, UHead, UTail...>
    friend constexpr const Temp&& get(const Tuple&& tuple);

    Tuple(Head&& head, Tuple<Tail...>&& tail)
        : head_(std::forward<Head>(head)),
          tail_(std::forward<Tuple<Tail...>>(tail)) {
    }

    template <typename UHead, typename... TailOfTuples>
    Tuple(internal_construct_tag tag, UHead&& head, TailOfTuples&&... tail)
        requires(std::is_same_v<decltype(head.tail_), Tuple<>>)
        : head_(std::forward<decltype(head.head_)>(head.head_)),
          tail_(tag, std::forward<TailOfTuples>(tail)...) {
    }

    template <typename UHead, typename... TailOfTuples>
    Tuple(internal_construct_tag tag, UHead&& head, TailOfTuples&&... tail)
        requires(!std::is_same_v<decltype(head.tail_), Tuple<>>)
        : head_(head.head_),
          tail_(tag, head.tail_, std::forward<TailOfTuples>(tail)...) {
    }

    template <typename UHead>
    Tuple(internal_construct_tag, UHead&& head)
        : head_(head.head_),
          tail_(std::forward<decltype(head.tail_)>(head.tail_)) {
    }

public:
    constexpr explicit(kIsCopyListInitializableV<Head>) Tuple()
        requires std::is_default_constructible_v<Head> &&
                     (std::is_default_constructible_v<Tail> && ...)
        : head_(),
          tail_() {
    }

    explicit(!std::is_convertible_v<const Head&, Head>) Tuple(const Head& head, const Tail&... tail)
        requires std::is_copy_constructible_v<Head> && (std::is_copy_constructible_v<Tail> && ...)
        : head_(head),
          tail_(tail...) {
    }

    template <typename UHead, typename... UTail>
    explicit(!(std::is_convertible_v<UHead, Head> && (std::is_convertible_v<UTail, Tail> && ...)))
        Tuple(UHead&& head, UTail&&... tail)
        requires(sizeof...(UTail) == sizeof...(Tail) && std::is_constructible_v<Head, UHead> &&
                 (std::is_constructible_v<Tail, UTail> && ...))
        : head_(std::forward<UHead>(head)),
          tail_(std::forward<UTail>(tail)...) {
    }

    template <typename UHead, typename... UTail>
    explicit(!(std::is_convertible_v<UHead, Head> && (std::is_convertible_v<UTail, Tail> && ...)))
        Tuple(const Tuple<UHead, UTail...>& other)
        requires(sizeof...(UTail) == sizeof...(Tail) &&
                 std::is_constructible_v<Head, const UHead&> &&
                 (std::is_constructible_v<Tail, const UTail&> && ...) &&
                 (sizeof...(UTail) != 0 || (!std::is_convertible_v<decltype(other), Head> &&
                                            !std::is_constructible_v<Head, decltype(other)> &&
                                            !std::is_same_v<Head, UHead>)))
        : head_(other.head_),
          tail_(other.tail_) {
    }

    template <typename UHead, typename... UTail>
    explicit(!(std::is_convertible_v<UHead, Head> && (std::is_convertible_v<UTail, Tail> && ...)))
        Tuple(Tuple<UHead, UTail...>&& other)
        requires(sizeof...(UTail) == sizeof...(Tail) && std::is_constructible_v<Head, UHead &&> &&
                 (std::is_constructible_v<Tail, UTail &&> && ...) &&
                 (sizeof...(UTail) != 0 || (!std::is_convertible_v<decltype(other), Head> &&
                                            !std::is_constructible_v<Head, decltype(other)> &&
                                            !std::is_same_v<Head, UHead>)))
        : head_(std::forward<UHead>(other.head_)),
          tail_(std::move(other.tail_)) {
    }

    Tuple(const Tuple& other)
        requires std::is_copy_constructible_v<Head> && (std::is_copy_constructible_v<Tail> && ...)
        : head_(other.head_),
          tail_(other.tail_) {
    }

    Tuple(Tuple&& other)
        requires std::is_move_constructible_v<Head> && (std::is_move_constructible_v<Tail> && ...)
        : head_(std::forward<Head>(other.head_)),
          tail_(std::move(other.tail_)) {
    }

    template <typename T1, typename T2>
    Tuple(const std::pair<T1, T2>& pair)
        : head_(pair.first),
          tail_(pair.second) {
    }

    template <typename T1, typename T2>
    Tuple(std::pair<T1, T2>&& pair)
        : head_(std::forward<T1>(pair.first)),
          tail_(std::forward<T2>(pair.second)) {
    }

    Tuple& operator=(const Tuple& other)
        requires std::is_copy_assignable_v<Head> && (std::is_copy_assignable_v<Tail> && ...)
    {
        if (this == &other) {
            return *this;
        }
        head_ = other.head_;
        tail_ = other.tail_;
        return *this;
    }

    Tuple& operator=(Tuple&& other)
        requires std::is_move_assignable_v<Head> && (std::is_move_assignable_v<Tail> && ...)
    {
        if (this == &other) {
            return *this;
        }

        head_ = std::forward<Head>(other.head_);
        tail_ = std::move(other.tail_);
        return *this;
    }

    template <typename UHead, typename... UTail>
    Tuple& operator=(const Tuple<UHead, UTail...>& other)
        requires(sizeof...(UTail) == sizeof...(Tail) && std::is_assignable_v<Head&, const UHead&> &&
                 (std::is_assignable_v<Tail&, const UTail&> && ...))
    {
        head_ = other.head_;
        tail_ = other.tail_;
        return *this;
    }

    template <typename UHead, typename... UTail>
    Tuple& operator=(Tuple<UHead, UTail...>&& other)
        requires(sizeof...(UTail) == sizeof...(Tail) && std::is_assignable_v<Head&, UHead> &&
                 (std::is_assignable_v<Tail&, UTail> && ...))
    {
        head_ = std::forward<UHead>(other.head_);
        tail_ = std::move(other.tail_);
        return *this;
    }

    template <typename T1, typename T2>
    Tuple& operator=(const std::pair<T1, T2>& other) {
        head_ = other.first;
        tail_.head_ = other.second;
        return *this;
    }

    template <typename T1, typename T2>
    Tuple& operator=(std::pair<T1, T2>&& other) {
        head_ = std::forward<T1>(other.first);
        tail_.head_ = std::forward<T2>(other.second);
        return *this;
    }

    ~Tuple() {
    }

    // template <typename T>
    // friend T tupleCat(T&& t);

    // template <typename UHead, typename... TailOfTuples>
    // friend getCatType<std::remove_reference_t<UHead>,
    //                   std::remove_reference_t<TailOfTuples>...>::type
    // tupleCat(UHead&& head, TailOfTuples&&... tail);

    template <typename... Args>
    friend getCatType<std::remove_reference_t<Args>...>::type tupleCat(Args&&... args);
};

template <typename T1, typename T2>
Tuple(const std::pair<T1, T2>& pair) -> Tuple<T1, T2>;

template <typename T1, typename T2>
Tuple(std::pair<T1, T2>&& pair) -> Tuple<T1, T2>;

// template <typename Head, typename... Tail>
// Tuple(Head&& head, Tuple<Tail...>&& tail) -> Tuple<Head, Tail...>;

// template <typename Head, typename... Tail>
// Tuple(Head&& head, Tuple<Tail...>& tail) -> Tuple<Head, Tail...>;

template <typename... Args>
constexpr Tuple<std::decay_t<Args>...> makeTuple(Args&&... args) {
    return Tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
}

template <class... Args>
Tuple<Args&...> tie(Args&... args) noexcept {
    return Tuple<Args&...>(std::forward<Args>(args)...);
}

template <typename... Args>
Tuple<Args&&...> forwardAsTuple(Args&&... args) {
    return Tuple<Args&&...>(std::forward<Args>(args)...);
}

template <typename... Args>
getCatType<std::remove_reference_t<Args>...>::type tupleCat(Args&&... args) {
    return typename getCatType<std::remove_reference_t<Args>...>::type(internal_construct_tag(),
                                                                       args...);
}
