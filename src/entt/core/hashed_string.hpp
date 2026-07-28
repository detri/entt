#ifndef RAPIDHASH_IMPL
#define RAPIDHASH_IMPL

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string_view>
#include <type_traits>

#if defined(_MSC_VER)
#  include <intrin.h>
#  if defined(_M_X64) && !defined(_M_ARM64EC)
#    pragma intrinsic(_umul128)
#  endif
#endif

#if defined(_MSC_VER)
#  define RAPIDHASH_ALWAYS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#  define RAPIDHASH_ALWAYS_INLINE inline __attribute__((__always_inline__))
#else
#  define RAPIDHASH_ALWAYS_INLINE inline
#endif

#ifndef RAPIDHASH_UNROLLED
#  define RAPIDHASH_COMPACT
#elif defined(RAPIDHASH_COMPACT)
#  error "RAPIDHASH_COMPACT and RAPIDHASH_UNROLLED cannot both be defined"
#endif

#ifndef RAPIDHASH_PROTECTED
#  define RAPIDHASH_FAST
#elif defined(RAPIDHASH_FAST)
#  error "RAPIDHASH_FAST and RAPIDHASH_PROTECTED cannot both be defined"
#endif

namespace rapidhash
{
    inline constexpr std::uint64_t secret[8] = {
        0x2d358dccaa6c78a5ULL,
        0x8bb84b93962eacc9ULL,
        0x4b33a62ed433d4a3ULL,
        0x4d5a2da51de1aa47ULL,
        0xa0761d6478bd642fULL,
        0xe7037ed1a0b428dbULL,
        0x90ed1765281c388cULL,
        0xaaaaaaaaaaaaaaaaULL
    };

    namespace detail
    {
        template<class T>
        concept byte_like =
                std::same_as<std::remove_cv_t<T>, char> ||
                std::same_as<std::remove_cv_t<T>, signed char> ||
                std::same_as<std::remove_cv_t<T>, unsigned char> ||
                std::same_as<std::remove_cv_t<T>, std::byte>;

        template<byte_like Byte>
        [[nodiscard]] constexpr std::uint8_t byte_value(Byte value) noexcept
        {
            if constexpr (std::same_as<std::remove_cv_t<Byte>, std::byte>)
                return std::to_integer<std::uint8_t>(value);
            else
                return static_cast<std::uint8_t>(value);
        }

        template<byte_like Byte>
        [[nodiscard]] constexpr std::uint64_t read32_constexpr(
            const Byte* p) noexcept
        {
            return
                    std::uint64_t{byte_value(p[0])} |
                    (std::uint64_t{byte_value(p[1])} << 8) |
                    (std::uint64_t{byte_value(p[2])} << 16) |
                    (std::uint64_t{byte_value(p[3])} << 24);
        }

        template<byte_like Byte>
        [[nodiscard]] constexpr std::uint64_t read64_constexpr(
            const Byte* p) noexcept
        {
            return
                    std::uint64_t{byte_value(p[0])} |
                    (std::uint64_t{byte_value(p[1])} << 8) |
                    (std::uint64_t{byte_value(p[2])} << 16) |
                    (std::uint64_t{byte_value(p[3])} << 24) |
                    (std::uint64_t{byte_value(p[4])} << 32) |
                    (std::uint64_t{byte_value(p[5])} << 40) |
                    (std::uint64_t{byte_value(p[6])} << 48) |
                    (std::uint64_t{byte_value(p[7])} << 56);
        }

        template<byte_like Byte>
        [[nodiscard]] RAPIDHASH_ALWAYS_INLINE constexpr std::uint64_t read32(
            const Byte* p) noexcept
        {
            if (std::is_constant_evaluated())
                return read32_constexpr(p);

            if constexpr (std::endian::native == std::endian::little)
            {
                std::uint32_t value;
                std::memcpy(&value, p, sizeof(value));
                return value;
            } else
            {
                return read32_constexpr(p);
            }
        }

        template<byte_like Byte>
        [[nodiscard]] RAPIDHASH_ALWAYS_INLINE constexpr std::uint64_t read64(
            const Byte* p) noexcept
        {
            if (std::is_constant_evaluated())
                return read64_constexpr(p);

            if constexpr (std::endian::native == std::endian::little)
            {
                std::uint64_t value;
                std::memcpy(&value, p, sizeof(value));
                return value;
            } else
            {
                return read64_constexpr(p);
            }
        }

        struct uint128_parts
        {
            std::uint64_t low;
            std::uint64_t high;
        };

        [[nodiscard]] constexpr uint128_parts multiply_portable(
            const std::uint64_t lhs,
            const std::uint64_t rhs) noexcept
        {
            const std::uint64_t lhs_high = lhs >> 32;
            const std::uint64_t rhs_high = rhs >> 32;
            const std::uint64_t lhs_low =
                    static_cast<std::uint32_t>(lhs);
            const std::uint64_t rhs_low =
                    static_cast<std::uint32_t>(rhs);

            const std::uint64_t high_high = lhs_high * rhs_high;
            const std::uint64_t high_low = lhs_high * rhs_low;
            const std::uint64_t low_high = rhs_high * lhs_low;
            const std::uint64_t low_low = lhs_low * rhs_low;

            const std::uint64_t middle0 =
                    low_low + (high_low << 32);

            std::uint64_t carry = middle0 < low_low;

            const std::uint64_t low =
                    middle0 + (low_high << 32);

            carry += low < middle0;

            const std::uint64_t high =
                    high_high +
                    (high_low >> 32) +
                    (low_high >> 32) +
                    carry;

            return {low, high};
        }

        RAPIDHASH_ALWAYS_INLINE constexpr void apply_product(
            std::uint64_t* lhs,
            std::uint64_t* rhs,
            const uint128_parts product) noexcept
        {
#if defined(RAPIDHASH_PROTECTED)
            *lhs ^= product.low;
            *rhs ^= product.high;
#else
            *lhs = product.low;
            *rhs = product.high;
#endif
        }

        RAPIDHASH_ALWAYS_INLINE constexpr void mum(
            std::uint64_t* lhs,
            std::uint64_t* rhs) noexcept
        {
            if (std::is_constant_evaluated())
            {
                apply_product(
                    lhs,
                    rhs,
                    multiply_portable(*lhs, *rhs));

                return;
            }

#if defined(__SIZEOF_INT128__)
            const __uint128_t product =
                    static_cast<__uint128_t>(*lhs) *
                    static_cast<__uint128_t>(*rhs);

            apply_product(lhs, rhs, {
                              static_cast<std::uint64_t>(product),
                              static_cast<std::uint64_t>(product >> 64)
                          });
#elif defined(_MSC_VER) && \
      (defined(_WIN64) || defined(_M_HYBRID_CHPE_ARM64))
#  if defined(_M_X64)
            std::uint64_t high;
            const std::uint64_t low =
                    _umul128(*lhs, *rhs, &high);

            apply_product(lhs, rhs, {low, high});
#  else
            const std::uint64_t low = *lhs * *rhs;
            const std::uint64_t high = __umulh(*lhs, *rhs);

            apply_product(lhs, rhs, {low, high});
#  endif
#else
            apply_product(
                lhs,
                rhs,
                multiply_portable(*lhs, *rhs));
#endif
        }

        [[nodiscard]] RAPIDHASH_ALWAYS_INLINE constexpr std::uint64_t mix(
            std::uint64_t lhs,
            std::uint64_t rhs) noexcept
        {
            mum(&lhs, &rhs);
            return lhs ^ rhs;
        }

        enum class variant
        {
            full,
            micro,
            nano
        };

        template<variant Variant, byte_like Byte>
        [[nodiscard]] constexpr std::uint64_t hash_internal(
            const Byte* p,
            std::size_t len,
            std::uint64_t seed_value,
            const std::uint64_t* secret_value) noexcept
        {
            seed_value ^=
                    mix(seed_value ^ secret_value[2], secret_value[1]);

            std::uint64_t a = 0;
            std::uint64_t b = 0;
            std::size_t i = len;

            if (len <= 16)
            {
                if (len >= 4)
                {
                    seed_value ^= len;

                    if (len >= 8)
                    {
                        a = read64(p);
                        b = read64(p + len - 8);
                    } else
                    {
                        a = read32(p);
                        b = read32(p + len - 4);
                    }
                } else if (len > 0)
                {
                    a =
                            (static_cast<std::uint64_t>(
                                 byte_value(p[0])) << 45) |
                            byte_value(p[len - 1]);

                    b = byte_value(p[len >> 1]);
                }
            } else
            {
                if constexpr (Variant == variant::full)
                {
                    if (len > 112)
                    {
                        std::uint64_t see1 = seed_value;
                        std::uint64_t see2 = seed_value;
                        std::uint64_t see3 = seed_value;
                        std::uint64_t see4 = seed_value;
                        std::uint64_t see5 = seed_value;
                        std::uint64_t see6 = seed_value;

#if defined(RAPIDHASH_COMPACT)
                        do
                        {
                            seed_value = mix(
                                read64(p) ^ secret_value[0],
                                read64(p + 8) ^ seed_value);

                            see1 = mix(
                                read64(p + 16) ^ secret_value[1],
                                read64(p + 24) ^ see1);

                            see2 = mix(
                                read64(p + 32) ^ secret_value[2],
                                read64(p + 40) ^ see2);

                            see3 = mix(
                                read64(p + 48) ^ secret_value[3],
                                read64(p + 56) ^ see3);

                            see4 = mix(
                                read64(p + 64) ^ secret_value[4],
                                read64(p + 72) ^ see4);

                            see5 = mix(
                                read64(p + 80) ^ secret_value[5],
                                read64(p + 88) ^ see5);

                            see6 = mix(
                                read64(p + 96) ^ secret_value[6],
                                read64(p + 104) ^ see6);

                            p += 112;
                            i -= 112;
                        } while (i > 112);
#else
                        while (i > 224)
                        {
                            seed_value = mix(
                                read64(p) ^ secret_value[0],
                                read64(p + 8) ^ seed_value);

                            see1 = mix(
                                read64(p + 16) ^ secret_value[1],
                                read64(p + 24) ^ see1);

                            see2 = mix(
                                read64(p + 32) ^ secret_value[2],
                                read64(p + 40) ^ see2);

                            see3 = mix(
                                read64(p + 48) ^ secret_value[3],
                                read64(p + 56) ^ see3);

                            see4 = mix(
                                read64(p + 64) ^ secret_value[4],
                                read64(p + 72) ^ see4);

                            see5 = mix(
                                read64(p + 80) ^ secret_value[5],
                                read64(p + 88) ^ see5);

                            see6 = mix(
                                read64(p + 96) ^ secret_value[6],
                                read64(p + 104) ^ see6);

                            seed_value = mix(
                                read64(p + 112) ^ secret_value[0],
                                read64(p + 120) ^ seed_value);

                            see1 = mix(
                                read64(p + 128) ^ secret_value[1],
                                read64(p + 136) ^ see1);

                            see2 = mix(
                                read64(p + 144) ^ secret_value[2],
                                read64(p + 152) ^ see2);

                            see3 = mix(
                                read64(p + 160) ^ secret_value[3],
                                read64(p + 168) ^ see3);

                            see4 = mix(
                                read64(p + 176) ^ secret_value[4],
                                read64(p + 184) ^ see4);

                            see5 = mix(
                                read64(p + 192) ^ secret_value[5],
                                read64(p + 200) ^ see5);

                            see6 = mix(
                                read64(p + 208) ^ secret_value[6],
                                read64(p + 216) ^ see6);

                            p += 224;
                            i -= 224;
                        }

                        if (i > 112)
                        {
                            seed_value = mix(
                                read64(p) ^ secret_value[0],
                                read64(p + 8) ^ seed_value);

                            see1 = mix(
                                read64(p + 16) ^ secret_value[1],
                                read64(p + 24) ^ see1);

                            see2 = mix(
                                read64(p + 32) ^ secret_value[2],
                                read64(p + 40) ^ see2);

                            see3 = mix(
                                read64(p + 48) ^ secret_value[3],
                                read64(p + 56) ^ see3);

                            see4 = mix(
                                read64(p + 64) ^ secret_value[4],
                                read64(p + 72) ^ see4);

                            see5 = mix(
                                read64(p + 80) ^ secret_value[5],
                                read64(p + 88) ^ see5);

                            see6 = mix(
                                read64(p + 96) ^ secret_value[6],
                                read64(p + 104) ^ see6);

                            p += 112;
                            i -= 112;
                        }
#endif

                        seed_value ^= see1;
                        see2 ^= see3;
                        see4 ^= see5;
                        seed_value ^= see6;
                        see2 ^= see4;
                        seed_value ^= see2;
                    }
                } else if constexpr (Variant == variant::micro)
                {
                    if (i > 80)
                    {
                        std::uint64_t see1 = seed_value;
                        std::uint64_t see2 = seed_value;
                        std::uint64_t see3 = seed_value;
                        std::uint64_t see4 = seed_value;

                        do
                        {
                            seed_value = mix(
                                read64(p) ^ secret_value[0],
                                read64(p + 8) ^ seed_value);

                            see1 = mix(
                                read64(p + 16) ^ secret_value[1],
                                read64(p + 24) ^ see1);

                            see2 = mix(
                                read64(p + 32) ^ secret_value[2],
                                read64(p + 40) ^ see2);

                            see3 = mix(
                                read64(p + 48) ^ secret_value[3],
                                read64(p + 56) ^ see3);

                            see4 = mix(
                                read64(p + 64) ^ secret_value[4],
                                read64(p + 72) ^ see4);

                            p += 80;
                            i -= 80;
                        } while (i > 80);

                        seed_value ^= see1;
                        see2 ^= see3;
                        seed_value ^= see4;
                        seed_value ^= see2;
                    }
                } else
                {
                    if (i > 48)
                    {
                        std::uint64_t see1 = seed_value;
                        std::uint64_t see2 = seed_value;

                        do
                        {
                            seed_value = mix(
                                read64(p) ^ secret_value[0],
                                read64(p + 8) ^ seed_value);

                            see1 = mix(
                                read64(p + 16) ^ secret_value[1],
                                read64(p + 24) ^ see1);

                            see2 = mix(
                                read64(p + 32) ^ secret_value[2],
                                read64(p + 40) ^ see2);

                            p += 48;
                            i -= 48;
                        } while (i > 48);

                        seed_value ^= see1;
                        seed_value ^= see2;
                    }
                }

                if (i > 16)
                {
                    seed_value = mix(
                        read64(p) ^ secret_value[2],
                        read64(p + 8) ^ seed_value);

                    if (i > 32)
                    {
                        seed_value = mix(
                            read64(p + 16) ^ secret_value[2],
                            read64(p + 24) ^ seed_value);

                        if constexpr (Variant != variant::nano)
                        {
                            if (i > 48)
                            {
                                seed_value = mix(
                                    read64(p + 32) ^ secret_value[1],
                                    read64(p + 40) ^ seed_value);

                                if (i > 64)
                                {
                                    seed_value = mix(
                                        read64(p + 48) ^ secret_value[1],
                                        read64(p + 56) ^ seed_value);
                                }
                            }
                        }

                        if constexpr (Variant == variant::full)
                        {
                            if (i > 80)
                            {
                                seed_value = mix(
                                    read64(p + 64) ^ secret_value[2],
                                    read64(p + 72) ^ seed_value);

                                if (i > 96)
                                {
                                    seed_value = mix(
                                        read64(p + 80) ^ secret_value[1],
                                        read64(p + 88) ^ seed_value);
                                }
                            }
                        }
                    }
                }

                a = read64(p + i - 16) ^ i;
                b = read64(p + i - 8);
            }

            a ^= secret_value[1];
            b ^= seed_value;

            mum(&a, &b);

            return mix(
                a ^ secret_value[7],
                b ^ secret_value[1] ^ i);
        }
    }

    template<detail::byte_like Byte>
    [[nodiscard]] constexpr std::uint64_t rapidhash_with_seed(
        const Byte* key,
        std::size_t len,
        std::uint64_t seed_value) noexcept
    {
        return detail::hash_internal<detail::variant::full>(
            key,
            len,
            seed_value,
            secret);
    }

    template<detail::byte_like Byte>
    [[nodiscard]] constexpr std::uint64_t rapidhash_micro_with_seed(
        const Byte* key,
        std::size_t len,
        std::uint64_t seed_value) noexcept
    {
        return detail::hash_internal<detail::variant::micro>(
            key,
            len,
            seed_value,
            secret);
    }

    template<detail::byte_like Byte>
    [[nodiscard]] constexpr std::uint64_t rapidhash_nano_with_seed(
        const Byte* key,
        std::size_t len,
        std::uint64_t seed_value) noexcept
    {
        return detail::hash_internal<detail::variant::nano>(
            key,
            len,
            seed_value,
            secret);
    }

    [[nodiscard]] inline std::uint64_t rapidhash_with_seed(
        const void* key,
        const std::size_t len,
        const std::uint64_t seed_value) noexcept
    {
        return detail::hash_internal<detail::variant::full>(
            static_cast<const unsigned char*>(key),
            len,
            seed_value,
            secret);
    }

    [[nodiscard]] inline std::uint64_t rapidhash_micro_with_seed(
        const void* key,
        const std::size_t len,
        const std::uint64_t seed_value) noexcept
    {
        return detail::hash_internal<detail::variant::micro>(
            static_cast<const unsigned char*>(key),
            len,
            seed_value,
            secret);
    }

    [[nodiscard]] inline std::uint64_t rapidhash_nano_with_seed(
        const void* key,
        const std::size_t len,
        const std::uint64_t seed_value) noexcept
    {
        return detail::hash_internal<detail::variant::nano>(
            static_cast<const unsigned char*>(key),
            len,
            seed_value,
            secret);
    }

    template<detail::byte_like Byte>
    [[nodiscard]] constexpr std::uint64_t rapidhash(
        const Byte* key,
        const std::size_t len) noexcept
    {
        return rapidhash_with_seed(key, len, 0);
    }

    template<detail::byte_like Byte>
    [[nodiscard]] constexpr std::uint64_t rapidhash_micro(
        const Byte* key,
        const std::size_t len) noexcept
    {
        return rapidhash_micro_with_seed(key, len, 0);
    }

    template<detail::byte_like Byte>
    [[nodiscard]] constexpr std::uint64_t rapidhash_nano(
        const Byte* key,
        const std::size_t len) noexcept
    {
        return rapidhash_nano_with_seed(key, len, 0);
    }

    [[nodiscard]] inline std::uint64_t rapidhash(
        const void* key,
        const std::size_t len) noexcept
    {
        return rapidhash_with_seed(key, len, 0);
    }

    [[nodiscard]] inline std::uint64_t rapidhash_micro(
        const void* key,
        const std::size_t len) noexcept
    {
        return rapidhash_micro_with_seed(key, len, 0);
    }

    [[nodiscard]] inline std::uint64_t rapidhash_nano(
        const void* key,
        const std::size_t len) noexcept
    {
        return rapidhash_nano_with_seed(key, len, 0);
    }

    [[nodiscard]] constexpr std::uint64_t rapidhash(
        const std::string_view value) noexcept
    {
        return rapidhash(value.data(), value.size());
    }

    [[nodiscard]] constexpr std::uint64_t rapidhash_micro(
        const std::string_view value) noexcept
    {
        return rapidhash_micro(value.data(), value.size());
    }

    [[nodiscard]] constexpr std::uint64_t rapidhash_nano(
        const std::string_view value) noexcept
    {
        return rapidhash_nano(value.data(), value.size());
    }

    template<std::size_t N>
    [[nodiscard]] constexpr std::uint64_t rapidhash(
        const char (&value)[N]) noexcept
    {
        static_assert(N > 0);
        return rapidhash(value, N - 1);
    }

    template<std::size_t N>
    [[nodiscard]] constexpr std::uint64_t rapidhash_micro(
        const char (&value)[N]) noexcept
    {
        static_assert(N > 0);
        return rapidhash_micro(value, N - 1);
    }

    template<std::size_t N>
    [[nodiscard]] constexpr std::uint64_t rapidhash_nano(
        const char (&value)[N]) noexcept
    {
        static_assert(N > 0);
        return rapidhash_nano(value, N - 1);
    }

    template<class T>
        requires (
            std::is_trivially_copyable_v<T> &&
            std::has_unique_object_representations_v<T>)
    struct rapidhash_t
    {
        [[nodiscard]] constexpr std::size_t operator()(
            const T& value) const noexcept
        {
            if (std::is_constant_evaluated())
            {
                const auto bytes =
                        std::bit_cast<
                            std::array<std::byte, sizeof(T)> >(value);

                return rapidhash(bytes.data(), bytes.size());
            }

            return rapidhash(static_cast<const void*>(std::addressof(value)), sizeof(value));
        }
    };

    struct string_hash
    {
        using is_avalanching = void;
        using is_transparent = void;

        [[nodiscard]] constexpr std::size_t operator()(
            const std::string_view value) const noexcept
        {
            return rapidhash(value);
        }
    };
}

#undef RAPIDHASH_ALWAYS_INLINE
#endif // RAPIDHASH_IMPL

#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP

#include "../stl/cstddef.hpp"
#include "../stl/cstdint.hpp"
#include "fwd.hpp"

namespace entt {

/*! @cond ENTT_INTERNAL */
namespace internal {

template<typename = id_type>
struct fnv_1a_params;

template<>
struct fnv_1a_params<stl::uint32_t> {
    static constexpr auto offset = 2166136261;
    static constexpr auto prime = 16777619;
};

template<>
struct fnv_1a_params<stl::uint64_t> {
    static constexpr auto offset = 14695981039346656037ull;
    static constexpr auto prime = 1099511628211ull;
};

template<typename Char>
struct basic_hashed_string {
    using value_type = Char;
    using size_type = stl::size_t;
    using hash_type = id_type;

    const value_type *repr{};
    hash_type hash{fnv_1a_params<>::offset};
    size_type length{};
};

} // namespace internal
/*! @endcond */

/**
 * @brief Zero overhead unique identifier.
 *
 * A hashed string is a compile-time tool that allows users to use
 * human-readable identifiers in the codebase while using their numeric
 * counterparts at runtime.<br/>
 * Because of that, a hashed string can also be used in constant expressions if
 * required.
 *
 * @warning
 * This class doesn't take ownership of user-supplied strings nor does it make a
 * copy of them.
 *
 * @tparam Char Character type.
 */
template<typename Char>
class basic_hashed_string: internal::basic_hashed_string<Char> {
    using base_type = internal::basic_hashed_string<Char>;
    using params = internal::fnv_1a_params<>;

    struct const_wrapper {
        // non-explicit constructor on purpose
        constexpr const_wrapper(const base_type::value_type *str) noexcept
            : repr{str} {}

        const base_type::value_type *repr;
    };

public:
    /*! @brief Character type. */
    using value_type = base_type::value_type;
    /*! @brief Unsigned integer type. */
    using size_type = base_type::size_type;
    /*! @brief Unsigned integer type. */
    using hash_type = base_type::hash_type;

    /**
     * @brief Returns directly the numeric representation of a string view.
     * @param str Human-readable identifier.
     * @param len Length of the string to hash.
     * @return The numeric representation of the string.
     */
    [[nodiscard]] static constexpr hash_type value(const value_type *str, const size_type len) noexcept {
        return basic_hashed_string{str, len};
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifier.
     * @return The numeric representation of the string.
     */
    template<stl::size_t N>
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
    [[nodiscard]] static ENTT_CONSTEVAL hash_type value(const value_type (&str)[N]) noexcept {
        return basic_hashed_string{str};
    }

    /**
     * @brief Returns directly the numeric representation of a string.
     * @param wrapper Helps achieving the purpose by relying on overloading.
     * @return The numeric representation of the string.
     */
    [[nodiscard]] static constexpr hash_type value(const_wrapper wrapper) noexcept {
        return basic_hashed_string{wrapper};
    }

    /*! @brief Constructs an empty hashed string. */
    constexpr basic_hashed_string() noexcept
        : basic_hashed_string{nullptr, 0u} {}

    /**
     * @brief Constructs a hashed string from a string view.
     * @param str Human-readable identifier.
     * @param len Length of the string to hash.
     */
    constexpr basic_hashed_string(const value_type *str, const size_type len) noexcept
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        : base_type{str} {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for(; base_type::length < len; ++base_type::length) {
            base_type::hash = (base_type::hash ^ static_cast<id_type>(str[base_type::length])) * params::prime;
        }
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    /**
     * @brief Constructs a hashed string from an array of const characters.
     * @tparam N Number of characters of the identifier.
     * @param str Human-readable identifier.
     */
    template<stl::size_t N>
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
    ENTT_CONSTEVAL basic_hashed_string(const value_type (&str)[N]) noexcept
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        : base_type{str} {
        for(; str[base_type::length]; ++base_type::length) {
            base_type::hash = (base_type::hash ^ static_cast<id_type>(str[base_type::length])) * params::prime;
        }
    }

    /**
     * @brief Explicit constructor on purpose to avoid constructing a hashed
     * string directly from a `const value_type *`.
     *
     * @warning
     * The lifetime of the string is not extended nor is it copied.
     *
     * @param wrapper Helps achieving the purpose by relying on overloading.
     */
    explicit constexpr basic_hashed_string(const_wrapper wrapper) noexcept
        : base_type{wrapper.repr} {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for(; wrapper.repr[base_type::length]; ++base_type::length) {
            base_type::hash = (base_type::hash ^ static_cast<id_type>(wrapper.repr[base_type::length])) * params::prime;
        }
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    /**
     * @brief Returns the size of a hashed string.
     * @return The size of the hashed string.
     */
    [[nodiscard]] constexpr size_type size() const noexcept {
        return base_type::length;
    }

    /**
     * @brief Returns the human-readable representation of a hashed string.
     * @return The string used to initialize the hashed string.
     */
    [[nodiscard]] constexpr const value_type *data() const noexcept {
        return base_type::repr;
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the hashed string.
     */
    [[nodiscard]] constexpr hash_type value() const noexcept {
        return base_type::hash;
    }

    /*! @copydoc data */
    [[nodiscard]] explicit constexpr operator const value_type *() const noexcept {
        return data();
    }

    /**
     * @brief Returns the numeric representation of a hashed string.
     * @return The numeric representation of the hashed string.
     */
    [[nodiscard]] constexpr operator hash_type() const noexcept {
        return value();
    }

    /**
     * @brief Compares two hashed strings.
     * @param other A valid hashed string.
     * @return True if the two hashed strings are identical, false otherwise.
     */
    [[nodiscard]] constexpr bool operator==(const basic_hashed_string &other) const noexcept {
        return value() == other.value();
    }

    /**
     * @brief Lexicographically compares two hashed strings.
     * @param other A valid hashed string.
     * @return The relative order between the two hashed strings.
     */
    [[nodiscard]] constexpr auto operator<=>(const basic_hashed_string &other) const noexcept {
        return value() <=> other.value();
    }
};

/**
 * @brief Deduction guide.
 * @tparam Char Character type.
 * @param str Human-readable identifier.
 * @param len Length of the string to hash.
 */
template<typename Char>
basic_hashed_string(const Char *str, stl::size_t len) -> basic_hashed_string<Char>;

/**
 * @brief Deduction guide.
 * @tparam Char Character type.
 * @tparam N Number of characters of the identifier.
 * @param str Human-readable identifier.
 */
template<typename Char, stl::size_t N>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
basic_hashed_string(const Char (&str)[N]) -> basic_hashed_string<Char>;

inline namespace literals {

/**
 * @brief User defined literal for hashed strings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed string.
 */
[[nodiscard]] ENTT_CONSTEVAL hashed_string operator""_hs(const char *str, stl::size_t) noexcept {
    return hashed_string{str};
}

/**
 * @brief User defined literal for hashed wstrings.
 * @param str The literal without its suffix.
 * @return A properly initialized hashed wstring.
 */
[[nodiscard]] ENTT_CONSTEVAL hashed_wstring operator""_hws(const wchar_t *str, stl::size_t) noexcept {
    return hashed_wstring{str};
}

} // namespace literals

} // namespace entt

#endif
