#pragma once
#ifndef WRITER_HPP__VHIS1CJC
#define WRITER_HPP__VHIS1CJC

#include <algorithm>
#include <any>
#include <array>
#include <cstring>
#include <deque>
#include <forward_list>
#include <functional>
#include <initializer_list>
#include <list>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif
#endif

#include <jsonwriter/erthink/erthink_d2a.h++>
#include <jsonwriter/int.hpp>

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

namespace jsonwriter {

namespace detail {

class NoCopyMove
{
public:
    NoCopyMove() = default;

private:
    NoCopyMove(const NoCopyMove&) = delete;
    NoCopyMove& operator=(const NoCopyMove&) = delete;
    NoCopyMove(NoCopyMove&&) = delete;
    NoCopyMove& operator=(NoCopyMove&&) = delete;
};

} // namespace detail

/// Optimized buffer for serialization. Writing to the reserved space is valid
/// if consume() is called afterwards before calling make_room() or reserve().
/// The buffer is not copyable to avoid unwanted copies.
class Buffer
{
public:
    explicit Buffer() = default;
    virtual ~Buffer() = default;

    Buffer(const Buffer& other) = delete;
    Buffer& operator=(const Buffer& other) = delete;

    Buffer(Buffer&& other) = default;
    Buffer& operator=(Buffer&& other) = default;

    char* begin() noexcept { return m_ptr; }
    char* end() noexcept { return m_working_end; }
    const char* data() const noexcept { return begin(); }
    const char* end() const noexcept { return m_working_end; }

    char* data() noexcept { return begin(); }
    const char* begin() const noexcept { return m_ptr; }

    // Use this pointer for appending data to the reserved space.
    char* working_end() noexcept { return m_working_end; }

    size_t size() const noexcept { return static_cast<size_t>(end() - begin()); }
    size_t capacity() const noexcept { return m_capacity; }
    size_t room() const noexcept { return m_capacity - size(); }

    void reserve(const size_t count)
    {
        assert(m_ptr != nullptr);
        if (erthink_unlikely(count > m_capacity)) {
            realloc(size(), count);
        }
        assert(size() + room() == capacity());
    }

    /// Keep the allocated space but discard all data.
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

    /// Put anything you need in here.
    std::any context{};

protected:
    /// Implement in derived class. It is called only if the current capacity
    /// is exhausted.
    virtual void realloc(size_t data_size, size_t new_capacity) = 0;

    /// Must be called by the derived class on every data pointer change:
    /// * construction
    /// * move construction/assignment
    /// * realloc
    void set_data(char* ptr, size_t size, size_t capacity)
    {
        assert(capacity >= m_capacity);

        m_ptr = ptr;
        m_working_end = ptr + size;
        m_capacity = capacity;
    }

private:
    char* m_ptr{nullptr};
    char* m_working_end{nullptr};
    size_t m_capacity{0};
};

/// Simple growing buffer with a fixed initial capacity.
/// Moved-from instance behavior is undefined.
template<size_t INITIAL_FIXED_CAPACITY = 1024>
class SimpleBuffer : public Buffer
{
public:
    explicit SimpleBuffer() { set_data(m_ptr, 0, m_capacity); }

    SimpleBuffer(SimpleBuffer&& other) { move_from(std::move(other)); }
    SimpleBuffer& operator=(SimpleBuffer&& other)
    {
        move_from(std::move(other));
        return *this;
    }

    void realloc(const size_t data_size, const size_t new_capacity) override
    {
        // should be only growing
        assert(new_capacity >= m_capacity);

        // still fits in the static buffer
        if (erthink_likely(new_capacity <= INITIAL_FIXED_CAPACITY)) {
            return;
        }

        const auto bulk_new_capacity = std::max(new_capacity, m_capacity + m_capacity / 2);
        auto new_dynamic = Dynamic{new char[bulk_new_capacity]};
        assert(m_ptr != nullptr);
        ::memcpy(new_dynamic.get(), m_ptr, data_size);

        m_dynamic = std::move(new_dynamic);
        m_ptr = m_dynamic.get();
        m_capacity = bulk_new_capacity;

        set_data(m_ptr, data_size, m_capacity);
    }

private:
    void move_from(SimpleBuffer&& other)
    {
        if (&other != this) {
            const auto old_size = other.size();

            if (other.m_ptr == other.m_static.data()) {
                m_static = other.m_static;
                m_ptr = m_static.data();
            } else {
                m_dynamic = std::move(other.m_dynamic);
                m_ptr = m_dynamic.get();
            }
            m_capacity = other.m_capacity;

            other.~SimpleBuffer();
            new (&other) SimpleBuffer{};

            set_data(m_ptr, old_size, m_capacity);
            other.set_data(nullptr, 0, m_capacity);
        }
    }

    using Dynamic = std::unique_ptr<char[]>;

    std::array<char, INITIAL_FIXED_CAPACITY> m_static;
    Dynamic m_dynamic{};
    char* m_ptr{m_static.data()};
    size_t m_capacity{m_static.size()};
};

template<typename T>
void write(Buffer& buffer, T&& value);

namespace detail {

/// character -> output sequence
struct EscapeMaps
{
    static constexpr size_t SIZE{256};
    // Allow the compiler to copy characters as 8 byte words.
    static constexpr size_t MAX_LEN{8};

    std::array<bool, SIZE> is_escaped{};
    std::array<std::pair<std::array<char, MAX_LEN>, uint8_t>, SIZE> char_map{};

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

template<typename T>
class HasWriteFunction
{
    template<typename U>
    static std::true_type test_signature(void (U::*)(Buffer&));
    template<typename U>
    static std::true_type test_signature(void (U::*)(Buffer&) const);

    template<typename U>
    static decltype(test_signature(&U::write)) test(std::nullptr_t);

    template<typename>
    static std::false_type test(...);

    using type = decltype(test<T>(nullptr));

public:
    static const bool value = type::value;
};

} // namespace detail

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
        FormatInt format_int{};
        const auto str = format_int.itostr(value);
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

template<>
struct Formatter<float> : FormatterFloat
{ };
template<>
struct Formatter<double> : FormatterFloat
{ };

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
            buffer.make_room(std::max(BULK * detail::EscapeMaps::MAX_LEN + 1, buffer.room()));

            const size_t bulk_size = std::min(BULK, static_cast<size_t>(value.end() - it));
            for (size_t i{0}; i < bulk_size; ++i, ++it) {
                const char c = *it;
                const auto char_index = static_cast<uint8_t>(c);
                if (erthink_unlikely(detail::escape_maps.is_escaped[char_index])) {
                    const auto& [replacement, len] = detail::escape_maps.char_map[char_index];
                    std::copy_n(replacement.begin(), detail::EscapeMaps::MAX_LEN,
                                buffer.working_end());
                    buffer.consume(len);
                } else {
                    buffer.append_no_grow(c);
                }
            }
        }

        buffer.append_no_grow('"');
    }
};

template<>
struct Formatter<const char*> : Formatter<std::string_view>
{ };
template<>
struct Formatter<std::string> : Formatter<std::string_view>
{ };

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
    static void write(Buffer& buffer, const std::nullopt_t) { buffer.append("null"); }
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

class ListProxy : private detail::NoCopyMove
{
public:
    ListProxy(Buffer& buffer)
        : m_buffer{buffer}
    {
    }

    template<typename T>
    void push_back(const T& value)
    {
        push_back_impl(value);
    }

    template<typename T>
    void push_back(const std::initializer_list<T> value)
    {
        push_back_impl(value);
    }

private:
    template<typename T>
    void push_back_impl(const T& value)
    {
        if (erthink_likely(!m_first)) {
            m_buffer.append(',');
        }
        jsonwriter::write(m_buffer, value);
        m_first = false;
    }

    Buffer& m_buffer;
    bool m_first{true};
};

template<typename Callback>
class List
{
public:
#ifndef _MSC_VER
    static_assert(std::is_convertible_v<Callback, std::function<void(ListProxy&)>>,
                  "the callback must be a callable like `void(ListProxy&)`.");
#endif

    List(const Callback& callback)
        : m_callback{callback}
    {
    }

private:
    const Callback& m_callback;

    friend struct Formatter<List<Callback>>;
};

template<typename Callback>
struct Formatter<List<Callback>>
{
    static void write(Buffer& buffer, const List<Callback>& value)
    {
        buffer.append('[');
        ListProxy proxy{buffer};
        value.m_callback(proxy);
        buffer.append(']');
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

template<typename T>
struct Formatter<std::initializer_list<T>> : FormatterList
{ };
template<typename T>
struct Formatter<std::vector<T>> : FormatterList
{ };
template<typename T, size_t N>
struct Formatter<std::array<T, N>> : FormatterList
{ };
template<typename T>
struct Formatter<std::deque<T>> : FormatterList
{ };
template<typename T>
struct Formatter<std::forward_list<T>> : FormatterList
{ };
template<typename T>
struct Formatter<std::list<T>> : FormatterList
{ };

/// A proxy to provide `object[key] = value` semantics.
class ObjectProxy : private detail::NoCopyMove
{
private:
    class AssignmentProxy : private detail::NoCopyMove
    {
    public:
        AssignmentProxy(Buffer& buffer)
            : m_buffer{buffer}
        {
        }

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

public:
    ObjectProxy(Buffer& buffer)
        : m_buffer{buffer}
    {
    }

    AssignmentProxy operator[](const std::string_view key)
    {
        if (erthink_likely(!m_first)) {
            m_buffer.append(',');
        }
        m_first = false;
        Formatter<std::string_view>::write(m_buffer, key);
        m_buffer.append(':');
        return AssignmentProxy{m_buffer};
    }

private:
    Buffer& m_buffer;
    bool m_first{true};
};

template<typename Callback>
class Object
{
public:
#ifndef _MSC_VER
    static_assert(std::is_convertible_v<Callback, std::function<void(ObjectProxy&)>>,
                  "the callback must be a callable like `void(ObjectProxy&)`.");
#endif

    Object(const Callback& callback)
        : m_callback{callback}
    {
    }

private:
    const Callback& m_callback;

    friend struct Formatter<Object<Callback>>;
};

template<typename Callback>
struct Formatter<Object<Callback>>
{
    static void write(Buffer& buffer, const Object<Callback>& value)
    {
        buffer.append('{');
        ObjectProxy proxy{buffer};
        value.m_callback(proxy);
        buffer.append('}');
    }
};

constexpr inline std::array<int, 0> empty_list{};
constexpr inline struct EmptyObject
{
} empty_object{};

template<>
struct Formatter<EmptyObject>
{
    static void write(Buffer& buffer, const EmptyObject&) { buffer.append("{}"); }
};

/// JSON serialization without inherent memory allocations. See tests for usage.
template<typename T>
void write(Buffer& buffer, T&& value)
{
    using RawT = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr (detail::HasWriteFunction<RawT>::value) {
        value.write(buffer);
    } else {
        Formatter<RawT>::write(buffer, std::forward<T>(value));
    }
}

template<typename T>
void write(Buffer& buffer, const std::initializer_list<T> value)
{
    FormatterList::write(buffer, value);
}

} // namespace jsonwriter

#endif /* include guard */
