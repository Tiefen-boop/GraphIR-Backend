
#pragma once

#include <string>
#include <variant>
#include <ostream>
#include <cmath>

#include "Object.h"
#include "DynamicArray.h"

template <typename T, typename... Ts>
constexpr bool contains_type_v = std::disjunction_v<std::is_same<T, Ts>...>;

using Undefined = std::monostate;
using Null = std::monostate;

template <typename T>
double operator+(const T&, Undefined) {
    return NAN;
}

template <typename T>
double operator+(Undefined, const T&) {
    return NAN;
}

inline double operator+(Undefined, Undefined) {
    return NAN;
}

template <typename T>
double operator*(const T&, Undefined) {
    return NAN;
}

template <typename T>
double operator*(Undefined, const T&) {
    return NAN;
}

inline double operator*(Undefined, Undefined) {
    return NAN;
}

template <typename T>
bool operator<(const T&, Undefined) {
    return false;
}

template <typename T>
bool operator<(Undefined, const T&) {
    return false;
}

template <typename... Types>
class Union {
    std::variant<Types...> value;

    template<typename... Ts>
    struct _GetElementTypes;

    template<typename... Ts>
    using GetElementTypes = typename _GetElementTypes<Ts...>::t;

    template<typename T, typename... Ts>
    struct _GetElementTypes<T, Ts...> {
        using t = GetElementTypes<Ts...>;
    };

    template<typename T, typename... Ts>
    struct _GetElementTypes<DynamicArray<T>, Ts...> {
        using t = T&;
    };

    template<typename... Ts>
    struct _GetElementTypes<std::string, Ts...> {
        using t = char;
    };

    template<typename T, typename... Ts>
    struct _GetElementTypes<Object<T>, Ts...> {
        using t = T&;
    };

    template<>
    struct _GetElementTypes<> {
        using t = Undefined;
    };

    template <typename T>
    struct IsSharedPtr : std::false_type {};

    template <typename T>
    struct IsSharedPtr<std::shared_ptr<T>> : std::true_type {};

    template <typename T>
    struct IsDynamicArray : std::false_type {};

    template <typename T>
    struct IsDynamicArray<DynamicArray<T>> : std::true_type {};

    template <typename T>
    struct IsObject : std::false_type {};

    template <typename T>
    struct IsObject<Object<T>> : std::true_type {};

public:
    template <typename T>
    struct IsUnion : std::false_type {};

    template <typename... Ts>
    struct IsUnion<Union<Ts...>> : std::true_type {};

    Union() : value() {
        if constexpr (contains_type_v<Undefined, Types...>) {
            value = Undefined();
        }
    }

    template <typename T>
    Union(const T& arg) : value(arg) {}

    Union(int64_t arg) {
        if constexpr (contains_type_v<int64_t, Types...>) {
            value = arg;
        }
        else if constexpr (contains_type_v<double, Types...>) {
            value = static_cast<double>(arg);
        }
        else {
            throw std::bad_variant_access();
        }
    }

    template <typename T>
    Union& operator=(T&& arg) {
        if constexpr (std::is_same_v<std::decay_t<T>, Union>) {
            value = arg.value;
        }
        else if constexpr (IsUnion<std::decay_t<T>>::value) {
            visit([this](auto& arg) {
                using U = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<U, int64_t> && contains_type_v<double, Types...>) {
                    value = static_cast<double>(arg);
                }
                else {
                    value = arg;
                }
            }, arg.value);
        }
        else if constexpr (contains_type_v<double, Types...> && std::is_same_v<std::decay_t<T>, int64_t>) {
            value = (double)arg;
        }
        else {
            value = arg;
        }
        return *this;
    }

    Union& operator=(double arg) {
        if constexpr (contains_type_v<double, Types...>) {
            value = arg;
        }
        else if constexpr (contains_type_v<int64_t, Types...>) {
            value = static_cast<int64_t>(arg);
        }
        else {
            throw std::bad_variant_access();
        }
        return *this;
    }

    explicit operator bool() const {
        return std::visit([](const auto& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Undefined>) {
                return false;
            }
            else if constexpr (IsObject<T>::value || IsDynamicArray<T>::value) {
                // Objects and DynamicArrays are always truthy (like JS objects).
                return true;
            }
            else {
                return arg;
            }
        }, value);
    }

    // Typed accessor: extracts the T alternative directly.
    // Throws std::bad_variant_access if T is not the active alternative.
    template <typename T>
    T& get() { return std::get<T>(value); }

    template <typename T>
    const T& get() const { return std::get<T>(value); }

    operator double() const {
        return std::visit([](const auto& arg) -> double {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, double>) {
                return arg;
            }
            else if constexpr (std::is_same_v<T, int64_t>) {
                return static_cast<double>(arg);
            }
            else {
                throw std::bad_variant_access();
            }
        }, value);
    }

    operator std::string() const {
        return std::visit([](const auto& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return arg;
            }
            else if constexpr (std::is_same_v<T, Undefined>) {
                return "undefined";
            }
            else {
                return std::to_string(arg);
            }
        }, value);
    }

    using ElementType = GetElementTypes<Types...>;
    ElementType operator[](size_t index) {
        return std::visit([index](auto& arg) -> ElementType {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (IsDynamicArray<T>::value || std::is_same_v<T, std::string>) {
                return arg[index];
            }
            throw std::bad_variant_access();
        }, value);
    }

    // Const overload — ConstElementType strips the reference from ElementType and adds
    // const, so the type system enforces read-only access on const Union objects.
    using ConstElementType = const std::remove_reference_t<ElementType>&;
    ConstElementType operator[](size_t index) const {
        return std::visit([index](const auto& arg) -> ConstElementType {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (IsDynamicArray<T>::value || std::is_same_v<T, std::string>) {
                return arg[index];
            }
            throw std::bad_variant_access();
        }, value);
    }

    ElementType operator[](const char* prop) {
        return std::visit([prop](auto& arg) -> ElementType {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (IsObject<T>::value) {
                return arg[prop];
            }
            throw std::bad_variant_access();
        }, value);
    }

    size_t size() const {
        return std::visit([](const auto& arg) -> size_t {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (IsDynamicArray<T>::value || std::is_same_v<T, std::string>) {
                return arg.size();
            }
            throw std::bad_variant_access();
        }, value);
    }

    template <typename... Types1, typename... S>
    friend bool operator==(const Union<Types1...>& u1, const Union<S...>& u2);

    template <typename T, typename... S>
    friend bool operator<(const T& lhs, const Union<S...>& u2);

    template <typename... Ts1, typename... Ts2>
    friend auto operator+(const Union<Ts1...>& u, const Union<Ts2...>& v);

    template <typename T, typename... Types1>
    friend std::ostream& operator<<(std::ostream& os, const Union<T, Types1...>& u);

    template <typename... OtherTypes>
    friend class Union;

    template <typename T, typename S>
    friend bool _strictEquals(const T& a, const S& b);
};


template <typename... Types, typename T>
bool operator==(const T& val, const Union<Types...>& u) {
    return u == Union<T>(val);
}

template <typename... Types, typename T>
bool operator==(const Union<Types...>& u, const T& val) {
    return u == Union<T>(val);
}

template <typename... Types, typename T>
bool operator!=(const T& val, const Union<Types...>& u) {
    return u != Union<T>(val);
}

template <typename... Types, typename T>
bool operator!=(const Union<Types...>& u, const T& val) {
    return u != Union<T>(val);
}


template <typename T>
bool operator==(const Undefined&, const T&) {
    return false;
}

template <typename T>
bool operator==(const T&, const Undefined&) {
    return false;
}

inline bool operator==(double n, const std::string& s) {
    return s == std::to_string(n);
}

inline bool operator==(const std::string& s, double n) {
    return n == s;
}

template <typename... Ts1, typename... Ts2>
auto operator+(const Union<Ts1...>& u, const Union<Ts2...>& v) {
    return visit([v](const auto& arg) {
        return visit([&arg](const auto& otherArg) {
            return arg + otherArg;
        }, v.value);
    }, u.value);
}

template <typename... Types>
double operator+(const Union<Types...>& u, double n) {
    return (double)u + n;
}

template <typename... Types>
double operator+(double n, const Union<Types...>& u) {
    return (double)u + n;
}

template <typename... Types>
double operator+(const Union<Types...>& u, int64_t n) {
    return (double)u + n;
}

template <typename... Types>
double operator+(const Union<Types...>& u, int32_t n) {
    return (double)u + n;
}

// JS-style string concatenation: "str" + union and union + "str"
// Route through operator std::string() so the correct string representation
// is produced rather than going via operator double() which throws for
// non-numeric union alternatives.
template <typename... Types>
std::string operator+(const std::string& s, const Union<Types...>& u) {
    return s + static_cast<std::string>(u);
}

template <typename... Types>
std::string operator+(const Union<Types...>& u, const std::string& s) {
    return static_cast<std::string>(u) + s;
}


template <typename... Types1, typename... S>
bool operator==(const Union<Types1...>& u1, const Union<S...>& u2) {
    return std::visit([&u2](const auto& arg) {
        return std::visit([&arg](const auto& otherArg) {
            return arg == otherArg;
        }, u2.value);
    }, u1.value);
}

template <typename... Types1, typename... S>
bool operator!=(const Union<Types1...>& u1, const Union<S...>& u2) {
    return !(u1 == u2);
}

template <typename T, typename... Types>
bool operator<(const T& lhs, const Union<Types...>& u2) {
    if constexpr (Union<T>::template IsUnion<std::decay_t<T>>::value) {
        return std::visit([&u2](const auto& arg) {
            return arg < u2;
        }, lhs.value);
    }
    else {
        return std::visit([&lhs](const auto& arg) {
            return lhs < arg;
        }, u2.value);
    }
}

template <typename T, typename S>
bool operator>(const T& lhs, const S& rhs) {
    return rhs < lhs;
}

inline std::ostream& operator<<(std::ostream& os, const Undefined&) {
    return os << "undefined";
}

template <typename T, typename... Types>
std::ostream& operator<<(std::ostream& os, const Union<T, Types...>& u) {
    return std::visit([&os](const auto& arg) -> std::ostream& {
        return os << arg;
    }, u.value);
}
