#pragma once
#ifndef WRITER_HPP__VHIS1CJC
#define WRITER_HPP__VHIS1CJC

#include <algorithm>
#include <array>
#include <cstring>
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

/// Optimized buffer for serialization. Writing to the reserved space is valid
/// if consume() is called afterwards before calling make_room() or reserve().
/// The buffer is not copyable to avoid unwanted copies.
class Buffer
{
public:
    explicit Buffer() { assert(size() + room() == capacity()); }

    Buffer(const Buffer& other) = delete;
    Buffer& operator=(const Buffer& other) = delete;

    Buffer(Buffer&& other) { move_from(std::move(other)); }
    Buffer& operator=(Buffer&& other) { move_from(std::move(other)); return *this; }

    char* begin() noexcept { return m_ptr.get(); }
    char* end() noexcept { return m_working_end; }
    const char* data() const noexcept { return begin(); }
    const char* end() const noexcept { return m_working_end; }

    char* data() noexcept { return begin(); }
    const char* begin() const noexcept { return m_ptr.get(); }

    // Use this pointer for appending data to the reserved space.
    char* working_end() noexcept { return m_working_end; }

    size_t size() const noexcept { return static_cast<size_t>(end() - begin()); }
    size_t capacity() const noexcept { return m_capacity; }
    size_t room() const noexcept { return m_capacity - size(); }

    void reserve(const size_t count)
    {
        assert(m_ptr != nullptr);
        if (erthink_unlikely(count > m_capacity)) {
            const auto old_size = size();
            const auto new_capacity = std::max(count, m_capacity + m_capacity / 2);
            auto new_ptr = allocate(new_capacity);
            ::memcpy(new_ptr.get(), m_ptr.get(), old_size);
            m_ptr = std::move(new_ptr);
            m_working_end = begin() + old_size;
            m_capacity = new_capacity;
        }
        assert(capacity() >= count);
        assert(size() + room() == capacity());
    }

    void clear() noexcept { m_working_end = begin(); }

    /// Make room for at least "count" characters.
    void make_room(const size_t count) { reserve(size() + count); }

    /// Shift the working end by diff. Room is reduced by diff.
    void consume(const size_t diff) noexcept
    {
        assert(room() >= diff);
        m_working_end += diff;
    }

    void consume(char* const new_working_end) noexcept
    {
        assert(new_working_end >= m_working_end);
        assert(new_working_end <= begin() + m_capacity);
        m_working_end = new_working_end;
    }

    /// Adds the character and consumes 1.
    void append_no_grow(const char c) noexcept
    {
        *m_working_end = c;
        consume(1);
    }

    /// Grows by 1, adds the character and consumes 1.
    void append(const char c)
    {
        make_room(1);
        *m_working_end = c;
        consume(1);
    }

    /// Grows, adds, consumes a C-string literal.
    template<size_t N>
    void append(const char (&c)[N])
    {
        // exclude null termination
        make_room(N - 1);
        consume(std::copy_n(c, N - 1, m_working_end));
    }

private:
    using holder_t = std::unique_ptr<char[]>;

    void move_from(Buffer&& other) {
        if (&other != this) {
            const auto old_size = other.size();
            m_ptr = std::move(other.m_ptr);
            m_working_end = begin() + old_size;
            m_capacity = other.m_capacity;

            other.~Buffer();
            new (&other) Buffer{};
        }
    }

    holder_t allocate(const size_t count)
    {
        return holder_t{new (std::align_val_t{64}) char[count]};
    }

    static constexpr size_t INITIAL_CAPACITY{512};

    holder_t m_ptr{allocate(INITIAL_CAPACITY)};
    char* m_working_end{m_ptr.get()};
    size_t m_capacity{INITIAL_CAPACITY};
};

template<typename T>
void write(Buffer& buffer, T&& value);

namespace detail {

template<typename Buffer>
class WriterIterRef
{
public:
    WriterIterRef(Buffer& buffer)
        : m_buffer{buffer}
    { }

    template<typename T>
    void operator=(T&& value)
    {
        jsonwriter::write(m_buffer, std::forward<T>(value));
    }

    template<typename T>
    void operator=(const std::initializer_list<T> list)
    {
        jsonwriter::write(m_buffer, list);
    }

private:
    Buffer& m_buffer;
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

template<typename Buffer>
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
    static void write(Buffer& buffer, const T value)
    {
        static_assert(std::is_integral_v<T>);
        fmt::format_int str{value};
        buffer.make_room(str.size());
        buffer.consume(std::copy_n(str.data(), str.size(), buffer.working_end()));
    }
};

struct FormatterFloat
{
    static void write(Buffer& buffer, const double value)
    {
        buffer.make_room(erthink::d2a_max_chars);
        buffer.consume(erthink::d2a(value, buffer.working_end()));
    }
};

template<> struct Formatter<float> : FormatterFloat { };
template<> struct Formatter<double> : FormatterFloat { };

template<>
struct Formatter<bool>
{
    static void write(Buffer& buffer, const bool value)
    {
        if (value) {
            buffer.append("true");
        } else {
            buffer.append("false");
        }
    }
};

template<>
struct Formatter<std::string_view>
{
    static void write(Buffer& buffer, const std::string_view value)
    {
        // for two '"'
        buffer.make_room(2);
        buffer.append_no_grow('"');

        auto it = value.begin();
        while (it != value.end()) {
            static constexpr size_t BULK{64};

            // enough room for all characters to be `\uXXXX` and a terminating '"'
            buffer.make_room(std::max(BULK * 6 + 1, buffer.room()));

            const size_t bulk_size = std::min(BULK, static_cast<size_t>(value.end() - it));
            for (size_t i{0}; i < bulk_size; ++i, ++it) {
                const char c = *it;
                if (erthink_unlikely(detail::escape_maps.is_escaped[static_cast<uint8_t>(c)])) {
                    const auto& [replacement, len]
                        = detail::escape_maps.char_map[static_cast<uint8_t>(c)];
                    buffer.consume(std::copy_n(replacement.begin(), len, buffer.working_end()));
                } else {
                    buffer.append_no_grow(c);
                }
            }
        }

        buffer.append_no_grow('"');
    }
};

template<> struct Formatter<const char*> : Formatter<std::string_view> { };
template<> struct Formatter<std::string> : Formatter<std::string_view> { };

template<size_t N>
struct Formatter<char[N]>
{
    static void write(Buffer& buffer, const char* value)
    {
        // -1 to avoid the null termination character
        jsonwriter::write(buffer, std::string_view{value, N - 1});
    }
};

template<>
struct Formatter<char>
{
    static void write(Buffer& buffer, const char value)
    {
        jsonwriter::write(buffer, std::string_view{&value, 1});
    }
};

template<>
struct Formatter<std::nullopt_t>
{
    static void write(Buffer& buffer, const std::nullopt_t)
    {
        buffer.append("null");
    }
};

template<typename T>
struct Formatter<std::optional<T>>
{
    static void write(Buffer& buffer, const std::optional<T>& value)
    {
        if (value.has_value()) {
            jsonwriter::write(buffer, *value);
        } else {
            jsonwriter::write(buffer, std::nullopt);
        }
    }
};

struct FormatterList
{
    template<typename Container>
    static void write(Buffer& buffer, const Container& container)
    {
        buffer.append('[');
        auto it = container.begin();
        if (it != container.end()) {
            jsonwriter::write(buffer, *it);
            ++it;
        }
        for (; it != container.end(); ++it) {
            buffer.append(',');
            jsonwriter::write(buffer, *it);
        }
        buffer.append(']');
    }
};

template<typename T> struct Formatter<std::initializer_list<T>> : FormatterList { };
template<typename T> struct Formatter<std::vector<T>> : FormatterList { };
template<typename T, size_t N> struct Formatter<std::array<T, N>> : FormatterList { };
template<typename T> struct Formatter<std::deque<T>> : FormatterList { };
template<typename T> struct Formatter<std::forward_list<T>> : FormatterList { };
template<typename T> struct Formatter<std::list<T>> : FormatterList { };

/// A proxy to provide `object[key] = value` semantics.
template<typename Buffer>
class Object
{
public:
    Object(Buffer& buffer)
        : m_buffer{buffer}
    { }

    detail::WriterIterRef<Buffer> operator[](const std::string_view key)
    {
        if (erthink_likely(m_counter > 0)) {
            m_buffer.append(',');
        }
        ++m_counter;
        Formatter<std::string_view>::write(m_buffer, key);
        m_buffer.append(':');
        return detail::WriterIterRef<Buffer>{m_buffer};
    }

private:
    Buffer& m_buffer;
    size_t m_counter{0};
};

namespace detail {

template<typename Buffer>
using ObjectCallback = std::function<void(Object<Buffer>&)>;

} // namespace detail

template<>
struct Formatter<detail::ObjectCallback<Buffer>>
{
    template<typename Callback>
    static void write(Buffer& buffer, const Callback& callback)
    {
        buffer.append('{');
        Object<Buffer> wo{buffer};
        callback(wo);
        buffer.append('}');
    }
};

/// JSON serializatin without inherent memory allocations. See tests for usage.
template<typename T>
void write(Buffer& buffer, T&& value)
{
    if constexpr (std::is_convertible_v<T, detail::ObjectCallback<Buffer>>) {
        Formatter<detail::ObjectCallback<Buffer>>::write(buffer, value);
    } else {
        Formatter<std::remove_cv_t<std::remove_reference_t<T>>>::write(
            buffer, std::forward<T>(value));
    }
}

template<typename T>
void write(Buffer& buffer, const std::initializer_list<T> value)
{
    FormatterList::write(buffer, value);
}

} // namespace jsonwriter

#endif /* include guard */
