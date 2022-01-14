#pragma once
#ifndef WRITER_HPP__VHIS1CJC
#define WRITER_HPP__VHIS1CJC

#include <algorithm>
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

/// Convenience wrapper for simpler/faster appending to a container.
template<typename Container>
class TailBuffer
{
public:
    static_assert(std::is_same_v<decltype(*std::declval<Container>().begin()), char&>,
                  "container's dereferenced begin() must reference to char");
    static_assert(std::is_same_v<decltype(*std::declval<Container>().end()), char&>,
                  "container's dereferenced end() must reference to char");

    TailBuffer(Container& container)
        : m_container{container}
        , m_start{&*container.end()}
    { }

    TailBuffer(const TailBuffer&) = delete;
    TailBuffer& operator=(const TailBuffer&) = delete;
    TailBuffer(TailBuffer&&) = delete;
    TailBuffer& operator=(TailBuffer&&) = delete;

    /// Grow room by diff amount.
    void grow(const size_t diff)
    {
        resize(m_container.size() + diff);
    }

    /// Shift the beginning by diff. Room is reduced by diff.
    void consume(const size_t diff)
    {
        assert(m_start != nullptr);
        assert(room() >= diff);
        m_start += diff;
    }

    /// Fit the container size to the current consumed space.
    void fit() { resize(m_start - &*(m_container.begin())); }

    /// How much chars can fit without growing.
    size_t room() const { return end() - begin(); }

    char* begin() const
    {
        assert(m_start != nullptr);
        return m_start;
    }

    char* end() const { return &*(m_container.end()); }

    /// Adds the character and consumes 1.
    void append_no_grow(const char c)
    {
        assert(m_start != nullptr);
        assert(room() > 0);
        *m_start = c;
        consume(1);
    }

    /// Grows by 1, adds the character and consumes 1.
    void append(const char c)
    {
        grow(1);
        *m_start = c;
        consume(1);
    }

    /// Grows, adds, consumes a C-string literal.
    template<size_t N>
    void append(const char (&c)[N])
    {
        // exclude null termination
        grow(N - 1);
        std::copy_n(c, N - 1, begin());
        consume(N - 1);
    }

private:
    void resize(const size_t count)
    {
        assert(m_start != nullptr);
        const auto old_offset = m_start - &*(m_container.begin());
        m_container.resize(count);
        m_start = &*(m_container.begin()) + old_offset;
    }

    Container& m_container;
    char* m_start{nullptr};
};

template<typename Output, typename T>
void write(Output& output, T&& value);

namespace detail {

template<typename Output>
class WriterIterRef
{
public:
    WriterIterRef(Output& output)
        : m_output{output}
    { }

    template<typename T>
    void operator=(T&& value);

    template<typename T>
    void operator=(const std::initializer_list<T> list);

private:
    Output& m_output;
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

template<typename Output>
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
    template<typename Output>
    static void write(Output& output, const T value)
    {
        static_assert(std::is_integral_v<T>);
        fmt::format_int str{value};
        TailBuffer tail{output};
        tail.grow(str.size());
        std::copy_n(str.data(), str.size(), tail.begin());
    }
};

struct FormatterFloat
{
    template<typename Output>
    static void write(Output& output, const double value)
    {
        TailBuffer tail{output};
        tail.grow(erthink::d2a_max_chars);
        const auto end = erthink::d2a(value, tail.begin());
        tail.consume(end - tail.begin());
        tail.fit();
    }
};

template<> struct Formatter<float> : FormatterFloat { };
template<> struct Formatter<double> : FormatterFloat { };

template<>
struct Formatter<bool>
{
    template<typename Output>
    static void write(Output& output, const bool value)
    {
        TailBuffer tail{output};
        if (value) {
            tail.append("true");
        } else {
            tail.append("false");
        }
    }
};

template<>
struct Formatter<std::string_view>
{
    template<typename Output>
    static void write(Output& output, const std::string_view value)
    {
        TailBuffer tail{output};
        // for two '"'
        tail.grow(2);
        tail.append_no_grow('"');

        auto it = value.begin();
        while (it != value.end()) {
            static constexpr size_t BULK{64};

            // enough room for all characters to be `\uXXXX` and a terminating '"'
            tail.grow(std::max(BULK * 6 + 1, tail.room()));

            const size_t bulk_size = std::min(BULK, static_cast<size_t>(value.end() - it));
            for (size_t i{0}; i < bulk_size; ++i, ++it) {
                const char c = *it;
                if (erthink_unlikely(detail::escape_maps.is_escaped[static_cast<uint8_t>(c)])) {
                    const auto& [replacement, len]
                        = detail::escape_maps.char_map[static_cast<uint8_t>(c)];
                    std::copy_n(replacement.begin(), len, tail.begin());
                    tail.consume(len);
                } else {
                    tail.append_no_grow(c);
                }
            }
        }

        tail.append_no_grow('"');
        tail.fit();
    }
};

template<> struct Formatter<const char*> : Formatter<std::string_view> { };
template<> struct Formatter<std::string> : Formatter<std::string_view> { };

template<size_t N>
struct Formatter<char[N]>
{
    template<typename Output>
    static void write(Output& output, const char* value)
    {
        // -1 to avoid the null termination character
        jsonwriter::write(output, std::string_view{value, N - 1});
    }
};

template<>
struct Formatter<char>
{
    template<typename Output>
    static void write(Output& output, const char value)
    {
        jsonwriter::write(output, std::string_view{&value, 1});
    }
};

template<>
struct Formatter<std::nullopt_t>
{
    template<typename Output>
    static void write(Output& output, const std::nullopt_t)
    {
        TailBuffer tail{output};
        tail.append("null");
    }
};

template<typename T>
struct Formatter<std::optional<T>>
{
    template<typename Output>
    static void write(Output& output, const std::optional<T>& value)
    {
        if (value.has_value()) {
            jsonwriter::write(output, *value);
        } else {
            jsonwriter::write(output, std::nullopt);
        }
    }
};

struct FormatterList
{
    template<typename Output, typename Container>
    static void write(Output& output, const Container& container)
    {
        TailBuffer{output}.append('[');
        auto it = container.begin();
        if (it != container.end()) {
            jsonwriter::write(output, *it);
            ++it;
        }
        for (; it != container.end(); ++it) {
            TailBuffer{output}.append(',');
            jsonwriter::write(output, *it);
        }
        TailBuffer{output}.append(']');
    }
};

template<typename T> struct Formatter<std::initializer_list<T>> : FormatterList { };
template<typename T> struct Formatter<std::vector<T>> : FormatterList { };
template<typename T, size_t N> struct Formatter<std::array<T, N>> : FormatterList { };
template<typename T> struct Formatter<std::deque<T>> : FormatterList { };
template<typename T> struct Formatter<std::forward_list<T>> : FormatterList { };
template<typename T> struct Formatter<std::list<T>> : FormatterList { };

/// A proxy to provide `object[key] = value` semantics.
template<typename Output>
class Object
{
public:
    Object(Output& output)
        : m_output{output}
    { }

    detail::WriterIterRef<Output> operator[](const std::string_view key)
    {
        if (erthink_likely(m_counter > 0)) {
            TailBuffer{m_output}.append(',');
        }
        ++m_counter;
        Formatter<std::string_view>::write(m_output, key);
        TailBuffer{m_output}.append(':');
        return detail::WriterIterRef<Output>{m_output};
    }

private:
    Output& m_output;
    size_t m_counter{0};
};

namespace detail {

template<typename Output>
using ObjectCallback = std::function<void(Object<Output>&)>;

} // namespace detail

/// JSON serializatin without inherent memory allocations. See tests for usage.
template<typename Output, typename T>
void write(Output& output, T&& value)
{
    if constexpr (std::is_convertible_v<T, detail::ObjectCallback<Output>>) {
        Formatter<detail::ObjectCallback<Output>>::write(output, value);
    } else {
        Formatter<std::remove_cv_t<std::remove_reference_t<T>>>::write(
            output, std::forward<T>(value));
    }
}

template<typename Output, typename T>
void write(Output& output, const std::initializer_list<T> value)
{
    FormatterList::write(output, value);
}

template<typename Output>
struct Formatter<detail::ObjectCallback<Output>>
{
    template<typename Callback>
    static void write(Output& output, const Callback& callback)
    {
        TailBuffer{output}.append('{');
        Object<Output> wo{output};
        callback(wo);
        TailBuffer{output}.append('}');
    }
};

template<typename Output>
template<typename T>
void detail::WriterIterRef<Output>::operator=(T&& value)
{
    jsonwriter::write(m_output, std::forward<T>(value));
}

template<typename Output>
template<typename T>
void detail::WriterIterRef<Output>::operator=(const std::initializer_list<T> list)
{
    jsonwriter::write(m_output, list);
}

} // namespace jsonwriter

#endif /* include guard */
