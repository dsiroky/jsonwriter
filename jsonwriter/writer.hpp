#pragma once
#ifndef WRITER_HPP__VHIS1CJC
#define WRITER_HPP__VHIS1CJC

#include <algorithm>
#include <any>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <deque>
#include <forward_list>
#include <functional>
#include <initializer_list>
#include <limits>
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

#include <jsonwriter/dragonbox/dragonbox_to_chars.h>
#include <jsonwriter/int.hpp>

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

// 16 byte blocks are scanned for characters that need escaping and copied in
// one go when there are none. Both backends are optional; the scalar path stays
// as a portable fallback.
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#define JSONWRITER_DETAIL_SSE2 1
#include <emmintrin.h>
#elif defined(_M_ARM64) || (defined(__aarch64__) && defined(__ARM_NEON))
#define JSONWRITER_DETAIL_NEON 1
#include <arm_neon.h>
#endif

// [[unlikely]] is a C++20 attribute, so it is only used when the language mode
// provides it.
#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#define JSONWRITER_UNLIKELY [[unlikely]]
#else
#define JSONWRITER_UNLIKELY
#endif

/// What happens when a value JSON cannot represent (infinity, NaN) is
/// serialized. Define one of the following before including this header to
/// override the default:
///
/// - JSONWRITER_NONFINITE_THROW - throw std::domain_error (the default)
/// - JSONWRITER_NONFINITE_NULL  - serialize as null
/// - JSONWRITER_NONFINITE_ABORT - abort the program (useful when building
///   without exceptions)
#ifndef JSONWRITER_NONFINITE
#define JSONWRITER_NONFINITE JSONWRITER_NONFINITE_THROW
#endif

#define JSONWRITER_NONFINITE_THROW 1
#define JSONWRITER_NONFINITE_NULL 2
#define JSONWRITER_NONFINITE_ABORT 3

#if JSONWRITER_NONFINITE != JSONWRITER_NONFINITE_THROW \
    && JSONWRITER_NONFINITE != JSONWRITER_NONFINITE_NULL \
    && JSONWRITER_NONFINITE != JSONWRITER_NONFINITE_ABORT
#error "JSONWRITER_NONFINITE must be JSONWRITER_NONFINITE_THROW, JSONWRITER_NONFINITE_NULL or JSONWRITER_NONFINITE_ABORT"
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

    // A derived buffer owns its storage, so the base cannot provide correct move
    // semantics: a defaulted move would only copy the pointers and leave two
    // objects referring to the same storage. Derived classes must implement
    // their own move operations (see SimpleBuffer).
    Buffer(Buffer&& other) = delete;
    Buffer& operator=(Buffer&& other) = delete;

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
        if (count > m_capacity) {
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

    /// Must be called by the derived class on every growing data pointer
    /// change:
    /// * construction
    /// * realloc
    void set_data(char* ptr, size_t size, size_t capacity) noexcept
    {
        assert(capacity >= m_capacity);
        reset_data(ptr, size, capacity);
    }

    /// Same as set_data(), but allows the capacity to shrink. It must be used
    /// for move construction/assignment and for resetting a moved-from
    /// instance.
    void reset_data(char* ptr, size_t size, size_t capacity) noexcept
    {
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
    explicit SimpleBuffer() { set_data(m_static.data(), 0, m_static.size()); }

    SimpleBuffer(SimpleBuffer&& other) noexcept { move_from(std::move(other)); }
    SimpleBuffer& operator=(SimpleBuffer&& other) noexcept
    {
        move_from(std::move(other));
        return *this;
    }

    void realloc(const size_t data_size, const size_t new_capacity) override
    {
        // Buffer::reserve() calls this only when the current capacity is
        // exhausted, so the new capacity is always larger.
        assert(new_capacity > capacity());

        const auto bulk_new_capacity = std::max(new_capacity, capacity() + capacity() / 2);
        auto new_dynamic = Dynamic{new char[bulk_new_capacity]};
        assert(begin() != nullptr);
        ::memcpy(new_dynamic.get(), begin(), data_size);

        m_dynamic = std::move(new_dynamic);
        set_data(m_dynamic.get(), data_size, bulk_new_capacity);
    }

private:
    void move_from(SimpleBuffer&& other) noexcept
    {
        if (&other != this) {
            context = std::move(other.context);

            const auto old_size = other.size();

            if (other.begin() == other.m_static.data()) {
                // only the used prefix is meaningful; the rest is uninitialized
                if (old_size > 0) {
                    ::memcpy(m_static.data(), other.m_static.data(), old_size);
                }
                m_dynamic.reset();
                reset_data(m_static.data(), old_size, m_static.size());
            } else {
                const auto old_capacity = other.capacity();
                m_dynamic = std::move(other.m_dynamic);
                reset_data(m_dynamic.get(), old_size, old_capacity);
            }

            other.reset_data(nullptr, 0, 0);
        }
    }

    using Dynamic = std::unique_ptr<char[]>;

    std::array<char, INITIAL_FIXED_CAPACITY> m_static;
    Dynamic m_dynamic{};
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
    // Longest escape sequence is `\u00XX`.
    static constexpr size_t MAX_ESCAPE_LEN{6};

    // Every character maps to its output sequence and its length. Characters
    // that need no escaping map to themselves with a length of one.
    std::array<std::pair<std::array<char, MAX_LEN>, uint8_t>, SIZE> char_map{};

    constexpr EscapeMaps()
    {
        for (size_t i{0}; i < SIZE; ++i) {
            const auto set_item = [this, i](const auto& source, const size_t count) {
                for (size_t ofs{0}; ofs < count; ++ofs) {
                    char_map[i].first[ofs] = source[ofs];
                }
                char_map[i].second = static_cast<uint8_t>(count);
            };

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
                        char_map[i].second = MAX_ESCAPE_LEN;
                    } else {
                        char_map[i].first[0] = static_cast<char>(i);
                        char_map[i].second = 1;
                    }
            }
        }
    }
};

static constexpr EscapeMaps escape_maps{};

/// Appends one character to out, escaped if needed. Writes up to MAX_LEN bytes
/// even for single byte characters, so the caller must provide that slack.
inline char* write_escaped_char(const char c, char* const out) noexcept
{
    const auto& [replacement, len] = escape_maps.char_map[static_cast<uint8_t>(c)];
    std::memcpy(out, replacement.data(), EscapeMaps::MAX_LEN);
    return out + len;
}

/// Writes the characters in [begin, end) into out, escaping as needed. The
/// caller must leave room for the worst case of six bytes per character plus
/// EscapeMaps::MAX_LEN bytes of slack.
inline char* write_escaped(const char* it, const char* const end, char* out) noexcept
{
#if defined(JSONWRITER_DETAIL_SSE2)
    const __m128i quote = _mm_set1_epi8('"');
    const __m128i backslash = _mm_set1_epi8('\\');
    const __m128i control_limit = _mm_set1_epi8(0x1f);
    const __m128i zero = _mm_setzero_si128();

    while (end - it >= 16) {
        __m128i chunk;
        std::memcpy(&chunk, it, 16);

        // Unsigned c < 0x20 without a signed compare, so bytes >= 0x80 are not
        // mistaken for control characters.
        const __m128i is_control = _mm_cmpeq_epi8(_mm_subs_epu8(chunk, control_limit), zero);
        const __m128i needs_escape = _mm_or_si128(
            _mm_or_si128(_mm_cmpeq_epi8(chunk, quote), _mm_cmpeq_epi8(chunk, backslash)),
            is_control);

        if (_mm_movemask_epi8(needs_escape) == 0) {
            std::memcpy(out, &chunk, 16);
            out += 16;
            it += 16;
        } else {
            const char* const block_end = it + 16;
            for (; it != block_end; ++it) {
                out = write_escaped_char(*it, out);
            }
        }
    }
#elif defined(JSONWRITER_DETAIL_NEON)
    const uint8x16_t quote = vdupq_n_u8('"');
    const uint8x16_t backslash = vdupq_n_u8('\\');
    const uint8x16_t control_limit = vdupq_n_u8(0x20);

    while (end - it >= 16) {
        const uint8x16_t chunk = vld1q_u8(reinterpret_cast<const uint8_t*>(it));
        const uint8x16_t needs_escape = vorrq_u8(
            vorrq_u8(vceqq_u8(chunk, quote), vceqq_u8(chunk, backslash)),
            vcltq_u8(chunk, control_limit));

        if (vmaxvq_u8(needs_escape) == 0) {
            vst1q_u8(reinterpret_cast<uint8_t*>(out), chunk);
            out += 16;
            it += 16;
        } else {
            const char* const block_end = it + 16;
            for (; it != block_end; ++it) {
                out = write_escaped_char(*it, out);
            }
        }
    }
#endif

    for (; it != end; ++it) {
        out = write_escaped_char(*it, out);
    }
    return out;
}

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

namespace detail {

/// True if the value is neither infinity nor NaN.
///
/// Inspecting the bits avoids std::isfinite(), whose result can be constant
/// folded away by -ffast-math, and works regardless of the rounding mode.
template<typename FloatType>
[[nodiscard]] constexpr bool is_finite(const FloatType value) noexcept
{
    static_assert(std::is_floating_point_v<FloatType>);
    static_assert(std::numeric_limits<FloatType>::is_iec559);
    static_assert(sizeof(FloatType) == sizeof(std::uint32_t)
                  || sizeof(FloatType) == sizeof(std::uint64_t));

    using UIntType = std::conditional_t<sizeof(FloatType) == sizeof(std::uint32_t), std::uint32_t,
                                        std::uint64_t>;
    constexpr auto mantissa_bits = std::numeric_limits<FloatType>::digits - 1;
    constexpr auto exponent_bits = sizeof(FloatType) * 8 - mantissa_bits - 1;
    constexpr auto exponent_mask = ((UIntType{1} << exponent_bits) - 1) << mantissa_bits;

    UIntType bits{};
    std::memcpy(&bits, &value, sizeof(value));
    return (bits & exponent_mask) != exponent_mask;
}

/// Writes a floating point value without checking the remaining room. The
/// caller must provide max_output_string_length<FloatType> bytes.
template<typename FloatType>
inline char* write_float_no_grow(char* const out, const FloatType value)
{
    // JSON has no representation for infinity or NaN, see
    // JSONWRITER_NONFINITE for the available behaviors
    // clang-format off
    if (!is_finite(value)) JSONWRITER_UNLIKELY {
#if JSONWRITER_NONFINITE == JSONWRITER_NONFINITE_NULL
        std::memcpy(out, "null", 4);
        return out + 4;
#elif JSONWRITER_NONFINITE == JSONWRITER_NONFINITE_ABORT
        std::abort();
#else
        throw std::domain_error{"JSON cannot represent infinity or NaN"};
#endif
    }
    // clang-format on

    return jkj::dragonbox::to_chars_n(value, out);
}

} // namespace detail

template<typename FloatType>
struct FormatterFloat
{
    static void write(Buffer& buffer, const FloatType value)
    {
        buffer.make_room(jkj::dragonbox::max_output_string_length<FloatType>);
        buffer.consume(detail::write_float_no_grow(buffer.working_end(), value));
    }
};

template<>
struct Formatter<float> : FormatterFloat<float>
{ };
template<>
struct Formatter<double> : FormatterFloat<double>
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
        static constexpr size_t BULK{64};

        // for two '"'
        buffer.make_room(2);
        buffer.append_no_grow('"');

        if (!value.empty()) {
            const char* it = value.data();
            const char* const end = it + value.size();
            while (it != end) {
                const size_t bulk_size = std::min(BULK, static_cast<size_t>(end - it));

                // Worst case output is EscapeMaps::MAX_ESCAPE_LEN characters
                // per input character. The extra EscapeMaps::MAX_LEN bytes are
                // needed because the escape sequences are copied as word-sized
                // chunks that can overwrite the logical end. They also
                // guarantee room for the terminating '"'.
                buffer.make_room(BULK * detail::EscapeMaps::MAX_ESCAPE_LEN
                                 + detail::EscapeMaps::MAX_LEN);

                // Write through a local pointer and commit it once per bulk so
                // the compiler can keep the pointer in a register: a char store
                // may alias the Buffer's pointer member.
                buffer.consume(detail::write_escaped(it, it + bulk_size, buffer.working_end()));
                it += bulk_size;
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
        if (!m_first) {
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

    List(Callback callback)
        : m_callback{std::move(callback)}
    {
    }

private:
    Callback m_callback;

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

namespace detail {

/// True for element types that have a native numeric formatter with a small
/// compile time output width. Enums are excluded: users may specialize
/// Formatter for them. char is excluded because it serializes as a string.
template<typename T>
inline constexpr bool is_scalar_list_element =
    (std::is_integral_v<T> || std::is_same_v<T, float> || std::is_same_v<T, double>)
    && !std::is_same_v<T, char>;

/// Maximum number of characters the JSON representation of a scalar list
/// element occupies.
template<typename T>
constexpr size_t scalar_list_width() noexcept
{
    if constexpr (std::is_same_v<T, bool>) {
        return 5; // false
    } else if constexpr (std::is_floating_point_v<T>) {
        return jkj::dragonbox::max_output_string_length<T>;
    } else {
        // int64_t range, including the sign of the most negative value
        return 20;
    }
}

/// Writes one scalar value without checking the remaining room. The caller
/// must provide scalar_list_width<T>() bytes.
template<typename T>
inline char* write_scalar_no_grow(char* const out, const T value)
{
    static_assert(is_scalar_list_element<T>);

    if constexpr (std::is_same_v<T, bool>) {
        if (value) {
            std::memcpy(out, "true", 4);
            return out + 4;
        }
        std::memcpy(out, "false", 5);
        return out + 5;
    } else if constexpr (std::is_floating_point_v<T>) {
        return write_float_no_grow(out, value);
    } else {
        FormatInt format_int{};
        const auto str = format_int.itostr(value);
        std::memcpy(out, str.data(), str.size());
        return out + str.size();
    }
}

} // namespace detail

struct FormatterList
{
    template<typename Container>
    static void write(Buffer& buffer, const Container& container)
    {
        using T = typename Container::value_type;

        if constexpr (detail::is_scalar_list_element<T>) {
            write_scalars(buffer, container);
        } else {
            write_generic(buffer, container);
        }
    }

private:
    static constexpr size_t SCALAR_CHUNK_ELEMENTS{256};

    template<typename Container>
    static void write_generic(Buffer& buffer, const Container& container)
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

    template<typename Container>
    static void write_scalars(Buffer& buffer, const Container& container)
    {
        using T = typename Container::value_type;

        // Worst case: a separator and a value for every element.
        constexpr size_t CHUNK_BYTES =
            SCALAR_CHUNK_ELEMENTS * (detail::scalar_list_width<T>() + 1);

        buffer.append('[');

        bool first{true};
        size_t chunk_count{0};
        char* out{nullptr};

        // Hoist end(): the char stores through `out` may alias the container,
        // so calling end() in the loop condition would reload it every element.
        const auto end = container.end();
        for (auto it = container.begin(); it != end; ++it) {
            if (chunk_count == 0) {
                buffer.make_room(CHUNK_BYTES);
                out = buffer.working_end();
            }

            if (!first) {
                *out++ = ',';
            }

            out = detail::write_scalar_no_grow(out, static_cast<T>(*it));
            first = false;

            if (++chunk_count == SCALAR_CHUNK_ELEMENTS) {
                buffer.consume(out);
                chunk_count = 0;
            }
        }

        if (chunk_count != 0) {
            buffer.consume(out);
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
        void operator=(T&& value) const
        {
            jsonwriter::write(m_buffer, std::forward<T>(value));
        }

        template<typename T>
        void operator=(const std::initializer_list<T> list) const
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
        if (!m_first) {
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

    Object(Callback callback)
        : m_callback{std::move(callback)}
    {
    }

private:
    Callback m_callback;

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

#undef JSONWRITER_UNLIKELY
#undef JSONWRITER_DETAIL_SSE2
#undef JSONWRITER_DETAIL_NEON

#endif /* include guard */
