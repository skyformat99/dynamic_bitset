//
// Copyright (c) 2019 Maxime Pinard
//
// Distributed under the MIT license
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
//
#include "RandomIntGenerator.hpp"
#include "MultiTakeGenerator.hpp"

#include <catch2/catch.hpp>
#include <dynamic_bitset.hpp>

#include <algorithm>
#include <cstdint>

constexpr size_t RANDOM_VECTORS_TO_TEST = 24;
constexpr std::minstd_rand::result_type SEED = 314159;
//const std::minstd_rand::result_type SEED = std::random_device{}();

template<typename T>
constexpr size_t bits_number = sizeof(T) * CHAR_BIT;

template<typename T>
constexpr bool bit_value(T value, size_t bit_pos)
{
	static_assert(std::is_unsigned<T>::value, "T is not an unsigned integral type");
	assert(bit_pos < bits_number<T>);
	return (value & (T(1) << bit_pos)) != T(0);
}

TEMPLATE_TEST_CASE("constructors", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);

	SECTION("default constructor")
	{
		dynamic_bitset<TestType> bitset;

		REQUIRE(bitset.empty());
		REQUIRE(bitset.size() == 0);
		REQUIRE(bitset.capacity() == 0);
	}

	SECTION("nbits and init_val constructor")
	{
		const unsigned long long init_value =
		  GENERATE(take(RANDOM_VECTORS_TO_TEST, randomInt<unsigned long long>(SEED)));
		const size_t bits_to_take =
		  GENERATE(take(1, randomInt<size_t>(0, bits_number<unsigned long long>, SEED)));
		CAPTURE(init_value, bits_to_take);

		// call dynamic_bitset constructor
		const dynamic_bitset<TestType> bitset(bits_to_take, init_value);
		CAPTURE(bitset);

		// check init value bits
		for(size_t i = 0; i < bits_to_take; ++i)
		{
			CAPTURE(i);
			REQUIRE(bitset[i] == bit_value(init_value, i));
		}
	}

	SECTION("initializer_list constructor")
	{
		SECTION("one value")
		{
			const TestType init_value =
			  GENERATE(take(RANDOM_VECTORS_TO_TEST, randomInt<TestType>(SEED)));
			const dynamic_bitset<TestType> bitset = {init_value};

			// check bits
			for(size_t i = 0; i < bits_number<TestType>; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bit_value(init_value, i));
			}
		}

		SECTION("two values")
		{
			const std::tuple<TestType, TestType> init_values = GENERATE(multitake(
			  RANDOM_VECTORS_TO_TEST, randomInt<TestType>(SEED), randomInt<TestType>(SEED + 1)));
			std::initializer_list<TestType> init_values_list = {std::get<0>(init_values),
			                                                    std::get<1>(init_values)};
			const dynamic_bitset<TestType> bitset = init_values_list;
			CAPTURE(bitset, init_values_list);

			// check bits
			size_t value_i = 0;
			for(const TestType& value: init_values_list)
			{
				for(size_t bit_i = 0; bit_i < bits_number<TestType>; ++bit_i)
				{
					CAPTURE(value_i, bit_i);
					REQUIRE(bitset[value_i * bits_number<TestType> + bit_i]
					        == bit_value(value, bit_i));
				}
				++value_i;
			}
		}
	}
}