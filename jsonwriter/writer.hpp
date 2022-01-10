#pragma once
#ifndef WRITER_HPP__VHIS1CJC
#define WRITER_HPP__VHIS1CJC

#include <array>
#include <deque>
#include <forward_list>
#include <initializer_list>
#include <list>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <fmt/format.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <jsonwriter/erthink/erthink_d2a.h++>
#pragma GCC diagnostic pop

namespace jsonwriter {

template<typename Iterator, typename T>
Iterator write(Iterator output, T&& value);

namespace detail {

template<typename Iterator>
class WriterIterRef
{
public:
    WriterIterRef(Iterator& output)
        : m_output{output}
    { }

    template<typename T>
    void operator=(T&& value);

    template<typename T>
    void operator=(const std::initializer_list<T> list);

private:
    Iterator& m_output;
};

/// character -> output sequence
struct EscapeMaps
{
    static constexpr size_t SIZE{256};

    std::array<bool, SIZE> is_escaped{};
    std::array<std::pair<std::array<char, 6>, uint8_t>, SIZE> char_map{};

    constexpr EscapeMaps()
    {
        for (size_t i{0}; i < SIZE; ++i) {
            const auto set_item = [this, i](const auto& source, const size_t count) {
                for (size_t ofs{0}; ofs < count; ++ofs) {
                    char_map[i].first[ofs] = source[ofs];
                }
                char_map[i].second = static_cast<uint8_t>(count);
                is_escaped[i] = true;
            };

            is_escaped[i] = false;

            switch (static_cast<char>(i)) {
                case '"':
                    set_item("\\\"", 2);
                    break;
                case '\t':
                    set_item("\\t", 2);
                    break;
                case '\f':
                    set_item("\\f", 2);
                    break;
                case '\r':
                    set_item("\\r", 2);
                    break;
                case '\n':
                    set_item("\\n", 2);
                    break;
                case '\b':
                    set_item("\\b", 2);
                    break;
                case '\\':
                    set_item("\\\\", 2);
                    break;
                default:
                    // non-printable characters
                    if (i < 32) {
                        constexpr char hex_digits[] = "0123456789abcdef";
                        char_map[i].first[0] = '\\';
                        char_map[i].first[1] = 'u';
                        char_map[i].first[2] = '0';
                        char_map[i].first[3] = '0';
                        char_map[i].first[4] = hex_digits[(i & 0xf0) >> 4];
                        char_map[i].first[5] = hex_digits[i & 0xf];
                        char_map[i].second = 6;
                        is_escaped[i] = true;
                    } else {
                        char_map[i].first[0] = static_cast<char>(i);
                        char_map[i].second = 1;
                    }
            }
        }
    }
};

static constexpr EscapeMaps escape_maps{};

} // namespace detail

template<typename Iterator>
class Object;

/// Similar to fmt::formatter. Specialize the template for custom types.
/// T2 template parameter is for custom use, e.g. a conditional specialization.
template<typename T, typename T2 = void>
struct Formatter
{
    Formatter() = delete;
};

template<typename T>
struct Formatter<T, typename std::enable_if_t<std::is_integral_v<T>>>
{
    template<typename Iterator>
    static Iterator write(Iterator output, const T value)
    {
        static_assert(std::is_integral_v<T>);
        fmt::format_int str{value};
        return std::copy_n(str.data(), str.size(), output);
    }
};

struct FormatterFloat
{
    template<typename Iterator>
    static Iterator write(Iterator output, const double value)
    {
        std::array<char, erthink::d2a_max_chars> buf;
        const auto end = erthink::d2a(value, buf.begin());
        return std::copy(buf.begin(), end, output);
    }
};

template<> struct Formatter<float> : FormatterFloat { };
template<> struct Formatter<double> : FormatterFloat { };

template<>
struct Formatter<bool>
{
    template<typename Iterator>
    static Iterator write(Iterator output, const bool value)
    {
        if (value) {
            return std::copy_n("true", 4, output);
        } else {
            return std::copy_n("false", 5, output);
        }
    }
};

template<>
struct Formatter<std::string_view>
{
    template<typename Iterator>
    static Iterator write(Iterator output, const std::string_view value)
    {
        *output++ = '"';
        for (const auto c : value) {
            if (erthink_unlikely(detail::escape_maps.is_escaped[static_cast<uint8_t>(c)])) {
                const auto& [replacement, len]
                    = detail::escape_maps.char_map[static_cast<uint8_t>(c)];
                output = std::copy_n(replacement.begin(), len, output);
            } else {
                *output++ = c;
            }
        }
        *output++ = '"';
        return output;
    }
};

template<> struct Formatter<const char*> : Formatter<std::string_view> { };
template<> struct Formatter<std::string> : Formatter<std::string_view> { };

template<size_t N>
struct Formatter<char[N]>
{
    template<typename Iterator>
    static Iterator write(Iterator output, const char* value)
    {
        // -1 to avoid the null termination character
        return jsonwriter::write(output, std::string_view{value, N - 1});
    }
};

template<>
struct Formatter<char>
{
    template<typename Iterator>
    static Iterator write(Iterator output, const char value)
    {
        *output++ = '"';
        *output++ = value;
        *output++ = '"';
        return output;
    }
};

template<>
struct Formatter<std::nullopt_t>
{
    template<typename Iterator>
    static Iterator write(Iterator output, const std::nullopt_t)
    {
        return std::copy_n("null", 4, output);
    }
};

template<typename T>
struct Formatter<std::optional<T>>
{
    template<typename Iterator>
    static Iterator write(Iterator output, const std::optional<T>& value)
    {
        if (value.has_value()) {
            return jsonwriter::write(output, *value);
        } else {
            return jsonwriter::write(output, std::nullopt);
        }
    }
};

struct FormatterList
{
    template<typename Iterator, typename Container>
    static Iterator write(Iterator output, const Container& container)
    {
        *output++ = '[';
        auto it = container.begin();
        if (it != container.end()) {
            output = jsonwriter::write(output, *it);
            ++it;
        }
        for (; it != container.end(); ++it) {
            *output++ = ',';
            output = jsonwriter::write(output, *it);
        }
        *output++ = ']';
        return output;
    }
};

template<typename T> struct Formatter<std::initializer_list<T>> : FormatterList { };
template<typename T> struct Formatter<std::vector<T>> : FormatterList { };
template<typename T, size_t N> struct Formatter<std::array<T, N>> : FormatterList { };
template<typename T> struct Formatter<std::deque<T>> : FormatterList { };
template<typename T> struct Formatter<std::forward_list<T>> : FormatterList { };
template<typename T> struct Formatter<std::list<T>> : FormatterList { };

/// A proxy to provide `object[key] = value` semantics.
template<typename Iterator>
class Object
{
public:
    Object(Iterator& output)
        : m_output{output}
    { }

    detail::WriterIterRef<Iterator> operator[](const std::string_view key)
    {
        if (erthink_likely(m_counter > 0)) {
            *m_output = ',';
            ++m_output;
        }
        ++m_counter;
        m_output = Formatter<std::string_view>::write(m_output, key);
        *m_output = ':';
        ++m_output;
        return detail::WriterIterRef<Iterator>{m_output};
    }

private:
    Iterator& m_output;
    size_t m_counter{0};
};

namespace detail {

template<typename Iterator>
using ObjectCallback = std::function<void(Object<Iterator>&)>;

} // namespace detail

/// JSON serializatin without inherent memory allocations. See tests for usage.
template<typename Iterator, typename T>
Iterator write(Iterator output, T&& value)
{
    if constexpr (std::is_convertible_v<T, detail::ObjectCallback<Iterator>>) {
        return Formatter<detail::ObjectCallback<Iterator>>::write(output, value);
    } else {
        return Formatter<std::remove_cv_t<std::remove_reference_t<T>>>::write(
            output, std::forward<T>(value));
    }
}

template<typename Iterator, typename T>
Iterator write(Iterator output, const std::initializer_list<T> value)
{
    return FormatterList::write(output, value);
}

template<typename Iterator>
struct Formatter<detail::ObjectCallback<Iterator>>
{
    template<typename Callback>
    static Iterator write(Iterator output, const Callback& callback)
    {
        *output++ = '{';
        Object<Iterator> wo{output};
        callback(wo);
        *output++ = '}';
        return output;
    }
};

template<typename Iterator>
template<typename T>
void detail::WriterIterRef<Iterator>::operator=(T&& value)
{
    m_output = jsonwriter::write(m_output, std::forward<T>(value));
}

template<typename Iterator>
template<typename T>
void detail::WriterIterRef<Iterator>::operator=(const std::initializer_list<T> list)
{
    m_output = jsonwriter::write(m_output, list);
}

} // namespace jsonwriter

#endif /* include guard */
