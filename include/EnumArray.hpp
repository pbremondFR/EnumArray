#pragma once

#include <type_traits>
#include <string_view>
#include <array>
#include <concepts>
#include <limits>
#include <algorithm>

namespace pb {

template <typename E>
concept is_enum = std::is_enum_v<E>;

namespace detail {
	template <is_enum auto Min, decltype(Min) Max>
	consteval size_t	EnumRangeLength()
	{
		using E = decltype(Min);
		constexpr auto cast_min = std::underlying_type_t<E>(Min);
		constexpr auto cast_max = std::underlying_type_t<E>(Max);
		static_assert(cast_max >= cast_min, "Invalid enum range! Max should be >= Min");
		return static_cast<size_t>(cast_max - cast_min) + 1;
	}

	/*
	* When this function gets templated with a valid number casted to the enum, its signature looks something
	* like "bool is_enum_variant_valid<enumType, enumMemberName>()". But when it's invocated with an
	* invalid number casted to the enum, it looks like "bool is_enum_variant_valid<enumType, (enumType)42>()".
	* If there's a parenthesis, your enum does not contain this integer!
	*
	* So, calling this function with an arbitrary int, like is_enum_variant_valid<MyEnum, (MyEnum)42>() tells
	* you whether your enum has a variant that equals 42 or not.
	*/
	template <is_enum E, E EnumVariant>
	consteval bool	is_enum_variant_valid()
	{
		constexpr std::string_view funcsig = __FUNCSIG__;
		constexpr size_t start = funcsig.find(',', funcsig.rfind('<')) + 1;
		static_assert(start != funcsig.npos);
		return funcsig[start] != '(';
	}

	/*
	* Recursive parameter-pack expansion black magic to make an integer sequence with range [Min, Max].
	*/
	template <std::integral T, T Min, T Max, T... integers>
	struct make_integer_sequence_helper
	{
		using type = typename make_integer_sequence_helper<T, Min + 1, Max, integers..., Min>::type;
	};

	/*
	* Recursion base case of the helper struct above.
	*/
	template <std::integral T, T Max, T... integers>
	struct make_integer_sequence_helper<T, Max, Max, integers...>
	{
		using type = std::integer_sequence<T, integers..., Max>;
	};

	template <is_enum E, std::underlying_type_t<E>... Is>
	consteval auto	get_enum_variants_validity(std::integer_sequence<std::underlying_type_t<E>, Is...> sequence)
	{
		return std::array<bool, sequence.size()>{
			is_integer_valid_enum_variant<E, Is>()...
		};
	}

	template <std::integral auto Size, typename T>
	concept smaller_than_max_value = requires {
		requires Size < std::numeric_limits<T>::max();
	};

	// This fucking sucks
	template <std::integral auto Size>
	struct smallest_possible_index {
		using type = std::conditional_t<
			(Size <= std::numeric_limits<uint8_t>::max()), uint8_t,
			std::conditional_t<
				(Size <= std::numeric_limits<uint16_t>::max()), uint16_t,
				std::conditional_t<
					(Size <= std::numeric_limits<uint32_t>::max()), uint32_t,
					size_t
				>
			>
		>;
	};

	template <std::integral auto Size>
	using smallest_possible_index_t = typename smallest_possible_index<Size>::type;
}

template <std::integral T, T Min, T Max>
using make_ranged_integer_sequence = detail::make_integer_sequence_helper<T, Min, Max>::type;

template <is_enum auto Min, decltype(Min) Max>
using make_enum_sequence = make_ranged_integer_sequence<
	std::underlying_type_t<decltype(Min)>,
	std::underlying_type_t<decltype(Min)>(Min),
	std::underlying_type_t<decltype(Min)>(Max)
>;


template <is_enum E, std::underlying_type_t<E> ToTest>
consteval bool	is_integer_valid_enum_variant()
{
	return detail::is_enum_variant_valid<E, (E)ToTest>();
}

template <is_enum auto Min, decltype(Min) Max>
consteval size_t	count_valid_enum_variants()
{
	using E = decltype(Min);

	constexpr auto enum_sequence = make_enum_sequence<Min, Max>{};
	constexpr auto valid_variants = detail::get_enum_variants_validity<E>(enum_sequence);
	return static_cast<size_t>(std::count(valid_variants.begin(), valid_variants.end(), true));
}

template <is_enum auto Min, decltype(Min) Max>
consteval auto	is_enum_range_contiguous()
{
	using E = decltype(Min);

	constexpr auto enum_sequence = make_enum_sequence<Min, Max>{};
	constexpr auto valid_variants = detail::get_enum_variants_validity<E>(enum_sequence);
	constexpr auto valid_count = std::count(valid_variants.begin(), valid_variants.end(), true);
	return valid_count == enum_sequence.size();
}

template <class T, is_enum auto Min, decltype(Min) Max>
struct EnumArrayContiguous : public std::array<T, detail::EnumRangeLength<Min, Max>()>
{
private:
	using super = std::array<T, detail::EnumRangeLength<Min, Max>()>;

public:
	using enum_type			= decltype(Min);
	using underlying_type	= std::underlying_type_t<enum_type>;
	
	static constexpr inline enum_type	MIN = Min;
	static constexpr inline enum_type	MAX = Max;

	super::reference		at(super::size_type)				= delete;
	super::const_reference	at(super::size_type) const			= delete;
	super::reference		operator[](super::size_type)		= delete;
	super::const_reference	operator[](super::size_type) const	= delete;

	constexpr super::reference			at(enum_type idx)		{
		return super::at(static_cast<super::size_type>(underlying_type(idx) - underlying_type(Min)));
	}
	constexpr super::const_reference	at(enum_type idx) const	{
		return super::at(static_cast<super::size_type>(underlying_type(idx) - underlying_type(Min)));
	}
	constexpr super::reference			operator[](enum_type idx)		{
		return super::operator[](static_cast<super::size_type>(underlying_type(idx) - underlying_type(Min)));
	}
	constexpr super::const_reference	operator[](enum_type idx) const	{
		return super::operator[](static_cast<super::size_type>(underlying_type(idx) - underlying_type(Min)));
	}
};

template <is_enum auto Min, decltype(Min) Max>
struct EnumArrayIndexMap
{
	static constexpr size_t array_size = detail::EnumRangeLength<Min, Max>();

	using enum_type		= decltype(Min);
	using index_type	= detail::smallest_possible_index_t<array_size>;
	using array_type	= std::array<index_type, array_size>;

	static constexpr array_type	make_index_map()
	{
		using E = decltype(Min);

		constexpr auto enum_sequence = make_enum_sequence<Min, Max>{};
		constexpr auto valid_variants = detail::get_enum_variants_validity<E>(enum_sequence);
		constexpr auto build_array = [&]() {
			array_type index_array = {};
			index_type j = 0;
			for (size_t i = 0; i < enum_sequence.size(); ++i) {
				if (valid_variants[i])
					index_array[i] = j++;
			}
			return index_array;
		};
		return build_array();
	}
	static constexpr inline array_type	index_map = make_index_map();

	static constexpr index_type	lookup(size_t idx)	{ return index_map[idx]; }
};

template <class T, is_enum auto Min, decltype(Min) Max>
class EnumArraySparse : public std::array<T, count_valid_enum_variants<Min, Max>()>
{
private:
	using super			= std::array<T, count_valid_enum_variants<Min, Max>()>;
	using index_type	= EnumArrayIndexMap<Min, Max>::index_type;
	
	static constexpr inline EnumArrayIndexMap<Min, Max>	_index_map;

public:
	using enum_type			= decltype(Min);
	using underlying_type	= std::underlying_type_t<enum_type>;
	
	static constexpr inline enum_type	MIN = Min;
	static constexpr inline enum_type	MAX = Max;

	super::reference		at(super::size_type)				= delete;
	super::const_reference	at(super::size_type) const			= delete;
	super::reference		operator[](super::size_type)		= delete;
	super::const_reference	operator[](super::size_type) const	= delete;

	constexpr super::reference			at(enum_type idx)
	{
		auto lookup_index = static_cast<index_type>(underlying_type(idx) - underlying_type(Min));
		return super::at(_index_map.lookup(lookup_index));
	}
	constexpr super::const_reference	at(enum_type idx) const
	{
		auto lookup_index = static_cast<index_type>(underlying_type(idx) - underlying_type(Min));
		return super::at(_index_map.lookup(lookup_index));
	}
	constexpr super::reference			operator[](enum_type idx)
	{
		auto lookup_index = static_cast<index_type>(underlying_type(idx) - underlying_type(Min));
		return super::operator[](_index_map.lookup(lookup_index));
	}
	constexpr super::const_reference	operator[](enum_type idx) const
	{
		auto lookup_index = static_cast<index_type>(underlying_type(idx) - underlying_type(Min));
		return super::operator[](_index_map.lookup(lookup_index));
	}
};

} // namespace pb
