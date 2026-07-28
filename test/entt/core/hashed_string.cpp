#include <cstdint>
#include <string_view>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>

template<typename Type>
[[nodiscard]] static consteval Type adapt_hash(
    const std::uint64_t hash) noexcept
{
    if constexpr(sizeof(Type) == sizeof(std::uint32_t)) {
        return static_cast<Type>(hash ^ (hash >> 32u));
    } else {
        return static_cast<Type>(hash);
    }
}

struct BasicHashedString: ::testing::Test {

    using hash_type = entt::hashed_string::hash_type;

    static constexpr hash_type expected =
        adapt_hash<hash_type>(
            rapidhash::rapidhash_micro("foobar", 6u));

    static constexpr hash_type expected_empty =
        adapt_hash<hash_type>(
            rapidhash::rapidhash_micro("", 0u));

    using wide_hash_type = entt::hashed_wstring::hash_type;

    static constexpr wide_hash_type expected_wide =
        adapt_hash<wide_hash_type>(
            rapidhash::rapidhash_micro(L"foobar", 6u));
};

using HashedString = BasicHashedString;
using HashedWString = BasicHashedString;

TEST_F(HashedString, DeductionGuide) {
    testing::StaticAssertTypeEq<decltype(entt::basic_hashed_string{"foo"}), entt::hashed_string>();
    testing::StaticAssertTypeEq<decltype(entt::basic_hashed_string{L"foo"}), entt::hashed_wstring>();
}

TEST_F(HashedString, Functionalities) {
    using namespace entt::literals;
    using hash_type = entt::hashed_string::hash_type;

    const char *bar = "bar";

    auto foo_hs = entt::hashed_string{"foo"};
    auto bar_hs = entt::hashed_string{bar};

    ASSERT_NE(static_cast<hash_type>(foo_hs), static_cast<hash_type>(bar_hs));
    ASSERT_STREQ(static_cast<const char *>(foo_hs), "foo");
    ASSERT_STREQ(static_cast<const char *>(bar_hs), bar);
    ASSERT_STREQ(foo_hs.data(), "foo");
    ASSERT_STREQ(bar_hs.data(), bar);
    ASSERT_EQ(foo_hs.size(), 3u);
    ASSERT_EQ(bar_hs.size(), 3u);

    ASSERT_EQ(foo_hs, foo_hs);
    ASSERT_NE(foo_hs, bar_hs);

    const entt::hashed_string hs{"foobar"};

    ASSERT_EQ(static_cast<hash_type>(hs), expected);
    ASSERT_EQ(hs.value(), expected);

    ASSERT_EQ(foo_hs, "foo"_hs);
    ASSERT_NE(bar_hs, "foo"_hs);

    entt::hashed_string empty_hs{};

    ASSERT_EQ(empty_hs, entt::hashed_string{});
    ASSERT_NE(empty_hs, foo_hs);

    empty_hs = foo_hs;

    ASSERT_NE(empty_hs, entt::hashed_string{});
    ASSERT_EQ(empty_hs, foo_hs);
}

TEST_F(HashedString, Empty) {
    using hash_type = entt::hashed_string::hash_type;

    const entt::hashed_string hs{};

    ASSERT_EQ(hs.size(), 0u);
    ASSERT_EQ(static_cast<hash_type>(hs), expected_empty);
    ASSERT_EQ(static_cast<const char *>(hs), nullptr);
}

TEST_F(HashedString, Correctness) {
    const char *foobar = "foobar";
    const std::string_view view{"foobar__", 6};

    ASSERT_EQ(entt::hashed_string{foobar}, expected);
    ASSERT_EQ((entt::hashed_string{view.data(), view.size()}), expected);
    ASSERT_EQ(entt::hashed_string{"foobar"}, expected);

    ASSERT_EQ(entt::hashed_string::value(foobar), expected);
    ASSERT_EQ(entt::hashed_string::value(view.data(), view.size()), expected);
    ASSERT_EQ(entt::hashed_string::value("foobar"), expected);

    ASSERT_EQ(entt::hashed_string{foobar}.size(), 6u);
    ASSERT_EQ((entt::hashed_string{view.data(), view.size()}).size(), 6u);
    ASSERT_EQ(entt::hashed_string{"foobar"}.size(), 6u);
}

TEST_F(HashedString, Order) {
    using namespace entt::literals;

    const entt::hashed_string lhs = "foo"_hs;
    const entt::hashed_string rhs = "bar"_hs;

    ASSERT_EQ(lhs < rhs, lhs.value() < rhs.value());
    ASSERT_EQ(lhs <= rhs, lhs.value() <= rhs.value());
    ASSERT_EQ(lhs > rhs, lhs.value() > rhs.value());
    ASSERT_EQ(lhs >= rhs, lhs.value() >= rhs.value());

    ASSERT_FALSE(lhs < lhs);
    ASSERT_TRUE(lhs <= lhs);
    ASSERT_FALSE(lhs > lhs);
    ASSERT_TRUE(lhs >= lhs);
}

TEST_F(HashedString, Constexprness) {
    using namespace entt::literals;
    constexpr entt::hashed_string lhs = "foo"_hs;
    constexpr entt::hashed_string rhs = "bar"_hs;

    static_assert((lhs < rhs) == (lhs.value() < rhs.value()));
    static_assert((lhs <= rhs) == (lhs.value() <= rhs.value()));
    static_assert((lhs > rhs) == (lhs.value() > rhs.value()));
    static_assert((lhs >= rhs) == (lhs.value() >= rhs.value()));
}

TEST_F(HashedWString, DeductionGuide) {
    testing::StaticAssertTypeEq<decltype(entt::basic_hashed_string{"foo"}), entt::hashed_string>();
    testing::StaticAssertTypeEq<decltype(entt::basic_hashed_string{L"foo"}), entt::hashed_wstring>();
}

TEST_F(HashedWString, Functionalities) {
    using namespace entt::literals;
    using hash_type = entt::hashed_wstring::hash_type;

    const wchar_t *bar = L"bar";

    auto foo_hws = entt::hashed_wstring{L"foo"};
    auto bar_hws = entt::hashed_wstring{bar};

    ASSERT_NE(static_cast<hash_type>(foo_hws), static_cast<hash_type>(bar_hws));
    ASSERT_STREQ(static_cast<const wchar_t *>(foo_hws), L"foo");
    ASSERT_STREQ(static_cast<const wchar_t *>(bar_hws), bar);
    ASSERT_STREQ(foo_hws.data(), L"foo");
    ASSERT_STREQ(bar_hws.data(), bar);
    ASSERT_EQ(foo_hws.size(), 3u);
    ASSERT_EQ(bar_hws.size(), 3u);

    ASSERT_EQ(foo_hws, foo_hws);
    ASSERT_NE(foo_hws, bar_hws);

    const entt::hashed_wstring hws{L"foobar"};

    ASSERT_EQ(static_cast<hash_type>(hws), expected_wide);
    ASSERT_EQ(hws.value(), expected_wide);

    ASSERT_EQ(foo_hws, L"foo"_hws);
    ASSERT_NE(bar_hws, L"foo"_hws);
}

TEST_F(HashedWString, Empty) {
    using hash_type = entt::hashed_wstring::hash_type;

    const entt::hashed_wstring hws{};

    ASSERT_EQ(hws.size(), 0u);
    ASSERT_EQ(static_cast<hash_type>(hws), expected_empty);
    ASSERT_EQ(static_cast<const wchar_t *>(hws), nullptr);
}

TEST_F(HashedWString, Correctness) {
    const wchar_t *foobar = L"foobar";
    const std::wstring_view view{L"foobar__", 6};

    ASSERT_EQ(entt::hashed_wstring{foobar}, expected_wide);
    ASSERT_EQ((entt::hashed_wstring{view.data(), view.size()}), expected_wide);
    ASSERT_EQ(entt::hashed_wstring{L"foobar"}, expected_wide);

    ASSERT_EQ(entt::hashed_wstring::value(foobar), expected_wide);
    ASSERT_EQ(entt::hashed_wstring::value(view.data(), view.size()), expected_wide);
    ASSERT_EQ(entt::hashed_wstring::value(L"foobar"), expected_wide);

    ASSERT_EQ(entt::hashed_wstring{foobar}.size(), 6u);
    ASSERT_EQ((entt::hashed_wstring{view.data(), view.size()}).size(), 6u);
    ASSERT_EQ(entt::hashed_wstring{L"foobar"}.size(), 6u);
}

TEST_F(HashedWString, Order) {
    using namespace entt::literals;

    const entt::hashed_wstring lhs = L"foo"_hws;
    const entt::hashed_wstring rhs = L"bar"_hws;

    ASSERT_EQ(lhs < rhs, lhs.value() < rhs.value());
    ASSERT_EQ(lhs <= rhs, lhs.value() <= rhs.value());
    ASSERT_EQ(lhs > rhs, lhs.value() > rhs.value());
    ASSERT_EQ(lhs >= rhs, lhs.value() >= rhs.value());

    ASSERT_FALSE(lhs < lhs);
    ASSERT_TRUE(lhs <= lhs);
    ASSERT_FALSE(lhs > lhs);
    ASSERT_TRUE(lhs >= lhs);
}

TEST_F(HashedWString, Constexprness) {
    using namespace entt::literals;

    constexpr entt::hashed_wstring lhs = L"foo"_hws;
    constexpr entt::hashed_wstring rhs = L"bar"_hws;

    static_assert((lhs < rhs) == (lhs.value() < rhs.value()));
    static_assert((lhs <= rhs) == (lhs.value() <= rhs.value()));
    static_assert((lhs > rhs) == (lhs.value() > rhs.value()));
    static_assert((lhs >= rhs) == (lhs.value() >= rhs.value()));
}
