#pragma once
#ifndef WRITER_HPP__VHIS1CJC
#define WRITER_HPP__VHIS1CJC

#include <array>
#include <deque>
#include <forward_list>
#include <initializer_list>
#include <list>
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
template<typename T>
class Formatter
{ };

template<typename T>
struct FormatterInt
{
    template<typename Iterator>
    static Iterator write(Iterator output, const T value)
    {
        static_assert(std::is_integral_v<T>);
        fmt::format_int str{value};
        return std::copy_n(str.data(), str.size(), output);
    }
};

template<> struct Formatter<int8_t> : FormatterInt<int8_t> { };
template<> struct Formatter<uint8_t> : FormatterInt<uint8_t> { };
template<> struct Formatter<int16_t> : FormatterInt<int16_t> { };
template<> struct Formatter<uint16_t> : FormatterInt<uint16_t> { };
template<> struct Formatter<int32_t> : FormatterInt<int32_t> { };
template<> struct Formatter<uint32_t> : FormatterInt<uint32_t> { };
template<> struct Formatter<int64_t> : FormatterInt<int64_t> { };
template<> struct Formatter<uint64_t> : FormatterInt<uint64_t> { };

template<typename T>
struct FormatterFloat
{
    template<typename Iterator>
    static Iterator write(Iterator output, const double value)
    {
        static_assert(std::is_floating_point_v<T>);
        std::array<char, erthink::d2a_max_chars> buf;
        const auto end = erthink::d2a(value, buf.begin());
        return std::copy(buf.begin(), end, output);
    }
};

template<> struct Formatter<float> : FormatterFloat<float> { };
template<> struct Formatter<double> : FormatterFloat<double> { };

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
        *output = '"';
        ++output;
        for (const auto c : value) {
            if (erthink_unlikely(detail::escape_maps.is_escaped[static_cast<uint8_t>(c)])) {
                const auto& [replacement, len]
                    = detail::escape_maps.char_map[static_cast<uint8_t>(c)];
                output = std::copy_n(replacement.begin(), len, output);
            } else {
                *output = c;
                ++output;
            }
        }
        *output = '"';
        ++output;
        return output;
    }
};

template<> struct Formatter<const char*> : Formatter<std::string_view> { };
template<> struct Formatter<std::string> : Formatter<std::string_view> { };

template<size_t N>
struct Formatter<const char (&)[N]>
{
    template<typename Iterator>
    static Iterator write(Iterator output, const char* value)
    {
        // -1 to avoid the null termination character
        return Formatter<std::string_view>::write(output, std::string_view{value, N - 1});
    }
};

struct FormatterList
{
    template<typename Iterator, typename Container>
    static Iterator write(Iterator output, const Container& container)
    {
        *output = '[';
        ++output;
        auto it = container.begin();
        if (it != container.end()) {
            output = Formatter<std::decay_t<decltype(*it)>>::write(output, *it);
            ++it;
        }
        for (; it != container.end(); ++it) {
            *output = ',';
            ++output;
            output = Formatter<std::decay_t<decltype(*it)>>::write(output, *it);
        }
        *output = ']';
        ++output;
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
        if (m_counter > 0) {
            *m_output = ',';
            ++m_output;
        }
        ++m_counter;
        m_output = Formatter<std::string_view>::write(m_output, key);
        *m_output = ':';
        ++m_output;
        detail::WriterIterRef<Iterator> m_writer{m_output};
        return m_writer;
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
        return Formatter<detail::ObjectCallback<Iterator>>::write(
            output, detail::ObjectCallback<Iterator>{value});
    } else {
        return Formatter<std::decay_t<T>>::write(output, std::forward<T>(value));
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
    static Iterator write(Iterator output, const detail::ObjectCallback<Iterator> callback)
    {
        *output = '{';
        ++output;
        Object<Iterator> wo{output};
        callback(wo);
        *output = '}';
        ++output;
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
