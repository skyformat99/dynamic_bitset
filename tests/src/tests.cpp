//
// Copyright (c) 2019 Maxime Pinard
//
// Distributed under the MIT license
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
//
#include "RandomIntGenerator.hpp"
#include "RandomDynamicBitsetGenerator.hpp"
#include "RandomChunkGenerator.hpp"
#include "MultiTakeGenerator.hpp"

#include <catch2/catch.hpp>
#include <dynamic_bitset.hpp>

#include <algorithm>
#include <vector>
#include <list>
#include <cstdint>

constexpr size_t RANDOM_VECTORS_TO_TEST = 100;
constexpr size_t RANDOM_VARIATIONS_TO_TEST = 10;
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
		CAPTURE(SEED);
		const std::tuple<unsigned long long, size_t> values =
		  GENERATE(multitake(RANDOM_VECTORS_TO_TEST,
		                     randomInt<unsigned long long>(SEED),
		                     randomInt<size_t>(1, bits_number<unsigned long long>, SEED + 1)));
		const unsigned long long value = std::get<0>(values);
		const size_t bits_to_take = std::get<1>(values);
		CAPTURE(value, bits_to_take);

		dynamic_bitset<TestType> bitset(bits_to_take, value);
		CAPTURE(bitset);

		// check init value bits
		for(size_t i = 0; i < bits_to_take; ++i)
		{
			CAPTURE(i);
			REQUIRE(bitset[i] == bit_value(value, i));
		}
	}

	SECTION("initializer_list constructor")
	{
		SECTION("one value")
		{
			const TestType init_value =
			  GENERATE(take(RANDOM_VECTORS_TO_TEST, randomInt<TestType>(SEED)));
			const dynamic_bitset<TestType> bitset = {init_value};
			CAPTURE(init_value, bitset);

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

TEMPLATE_TEST_CASE("resize", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	dynamic_bitset<TestType> bitset =
	  GENERATE(take(RANDOM_VECTORS_TO_TEST, randomDynamicBitset<TestType>(SEED)));
	CAPTURE(bitset);

	SECTION("resizing to 0")
	{
		bitset.resize(0);
		REQUIRE(bitset.empty());
		REQUIRE(bitset.size() == 0);
	}

	SECTION("resizing to the same size")
	{
		const size_t size_save = bitset.size();
		bitset.resize(bitset.size());
		REQUIRE(bitset.size() == size_save);
	}

	SECTION("changing size")
	{
		size_t size_change =
		  GENERATE(take(RANDOM_VARIATIONS_TO_TEST, randomInt<size_t>(0, 128, SEED + 1)));
		const bool new_values = GENERATE(true, false);
		CAPTURE(size_change, new_values);

		SECTION("incrementing size")
		{
			const dynamic_bitset<TestType> bitset_copy = bitset;
			const size_t new_size = bitset.size() + size_change;
			bitset.resize(new_size, new_values);
			REQUIRE(bitset.size() == new_size);

			for(size_t i = 0; i < bitset_copy.size(); ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
			for(size_t i = bitset_copy.size(); i < bitset.size(); ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == new_values);
			}
		}

		SECTION("decrementing size")
		{
			const dynamic_bitset<TestType> bitset_copy = bitset;
			const size_t new_size = size_change > bitset.size() ? 0 : size_change;
			bitset.resize(new_size, new_values);
			REQUIRE(bitset.size() == new_size);

			for(size_t i = 0; i < bitset.size(); ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
		}
	}
}

TEMPLATE_TEST_CASE("clear", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	dynamic_bitset<TestType> bitset =
	  GENERATE(take(RANDOM_VECTORS_TO_TEST, randomDynamicBitset<TestType>(SEED)));
	CAPTURE(bitset);

	bitset.clear();
	REQUIRE(bitset.empty());
	REQUIRE(bitset.size() == 0);
}

TEMPLATE_TEST_CASE("push_back", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	std::tuple<dynamic_bitset<TestType>, dynamic_bitset<TestType>> values =
	  GENERATE(multitake(RANDOM_VECTORS_TO_TEST,
	                     randomDynamicBitset<TestType>(SEED),
	                     randomDynamicBitset<TestType>(1, 2 * bits_number<TestType>, SEED + 1)));
	dynamic_bitset<TestType>& bitset = std::get<0>(values);
	dynamic_bitset<TestType>& to_push = std::get<1>(values);

	const dynamic_bitset<TestType> bitset_copy = bitset;
	size_t size = bitset.size();

	// new bits added
	for(size_t i = 0; i < to_push.size(); ++i)
	{
		CAPTURE(i, size);
		bitset.push_back(to_push[i]);
		REQUIRE(bitset[size] == to_push[i]);
		++size;
		REQUIRE(bitset.size() == size);
	}

	// initial bits not changed
	for(size_t i = 0; i < bitset_copy.size(); ++i)
	{
		CAPTURE(i);
		REQUIRE(bitset[i] == bitset_copy[i]);
	}
}

TEMPLATE_TEST_CASE("pop_back", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	std::tuple<dynamic_bitset<TestType>, size_t> values =
	  GENERATE(multitake(RANDOM_VECTORS_TO_TEST,
	                     randomDynamicBitset<TestType>(SEED),
	                     randomInt<TestType>(1, 2 * bits_number<TestType>, SEED + 1)));
	dynamic_bitset<TestType>& bitset = std::get<0>(values);
	size_t to_pop = std::min(bitset.size(), std::get<1>(values));

	const dynamic_bitset<TestType> bitset_copy = bitset;
	size_t size = bitset.size();

	// bits removed
	for(size_t i = 0; i < to_pop; ++i)
	{
		CAPTURE(i, size);
		bitset.pop_back();
		--size;
		REQUIRE(bitset.size() == size);
	}

	// initial bits not changed
	for(size_t i = 0; i < bitset.size(); ++i)
	{
		CAPTURE(i);
		REQUIRE(bitset[i] == bitset_copy[i]);
	}
}

TEMPLATE_TEST_CASE("append", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	dynamic_bitset<TestType> bitset =
	  GENERATE(take(RANDOM_VECTORS_TO_TEST, randomDynamicBitset<TestType>(SEED)));
	CAPTURE(bitset);
	const dynamic_bitset<TestType> bitset_copy = bitset;

	SECTION("one value")
	{
		const TestType value =
		  GENERATE(take(RANDOM_VARIATIONS_TO_TEST, randomInt<TestType>(SEED + 1)));
		CAPTURE(value);

		bitset.append(value);
		REQUIRE(bitset.size() == (bitset_copy.size() + bits_number<TestType>));

		// new bits added
		for(size_t i = 0; i < bits_number<TestType>; ++i)
		{
			CAPTURE(i);
			REQUIRE(bitset[bitset_copy.size() + i] == bit_value(value, i));
		}

		// initial bits not changed
		for(size_t i = 0; i < bitset_copy.size(); ++i)
		{
			CAPTURE(i);
			REQUIRE(bitset[i] == bitset_copy[i]);
		}
	}

	SECTION("two values initializer_list")
	{
		const std::tuple<TestType, TestType> init_values = GENERATE(multitake(
		  RANDOM_VARIATIONS_TO_TEST, randomInt<TestType>(SEED + 1), randomInt<TestType>(SEED + 2)));
		std::initializer_list<TestType> values = {std::get<0>(init_values),
		                                          std::get<1>(init_values)};
		CAPTURE(values);

		bitset.append(values);
		REQUIRE(bitset.size() == (bitset_copy.size() + values.size() * bits_number<TestType>));

		// new bits added
		size_t value_i = 0;
		for(const TestType& value: values)
		{
			for(size_t bit_i = 0; bit_i < bits_number<TestType>; ++bit_i)
			{
				CAPTURE(value_i, bit_i);
				REQUIRE(bitset[bitset_copy.size() + value_i * bits_number<TestType> + bit_i]
				        == bit_value(value, bit_i));
			}
			++value_i;
		}

		// initial bits not changed
		for(size_t i = 0; i < bitset_copy.size(); ++i)
		{
			CAPTURE(i);
			REQUIRE(bitset[i] == bitset_copy[i]);
		}
	}

	SECTION("random access iterators")
	{
		const std::vector<TestType> values =
		  GENERATE(take(RANDOM_VARIATIONS_TO_TEST,
		                randomChunk<TestType>(2, 5, randomInt<TestType>(SEED + 1), SEED + 2)));
		CAPTURE(values);

		bitset.append(std::cbegin(values), std::cend(values));
		REQUIRE(bitset.size() == (bitset_copy.size() + values.size() * bits_number<TestType>));

		// new bits added
		size_t value_i = 0;
		for(const TestType& value: values)
		{
			for(size_t bit_i = 0; bit_i < bits_number<TestType>; ++bit_i)
			{
				CAPTURE(value_i, bit_i);
				REQUIRE(bitset[bitset_copy.size() + value_i * bits_number<TestType> + bit_i]
				        == bit_value(value, bit_i));
			}
			++value_i;
		}

		// initial bits not changed
		for(size_t i = 0; i < bitset_copy.size(); ++i)
		{
			CAPTURE(i);
			REQUIRE(bitset[i] == bitset_copy[i]);
		}
	}

	SECTION("bidirectional iterators")
	{
		const std::vector<TestType> values =
		  GENERATE(take(RANDOM_VARIATIONS_TO_TEST,
		                randomChunk<TestType>(2, 5, randomInt<TestType>(SEED + 1), SEED + 2)));
		CAPTURE(values);

		const std::list<TestType> values_list(std::cbegin(values), std::cend(values));
		bitset.append(std::cbegin(values_list), std::cend(values_list));
		REQUIRE(bitset.size() == (bitset_copy.size() + values.size() * bits_number<TestType>));

		// new bits added
		size_t value_i = 0;
		for(const TestType& value: values)
		{
			for(size_t bit_i = 0; bit_i < bits_number<TestType>; ++bit_i)
			{
				CAPTURE(value_i, bit_i);
				REQUIRE(bitset[bitset_copy.size() + value_i * bits_number<TestType> + bit_i]
				        == bit_value(value, bit_i));
			}
			++value_i;
		}

		// initial bits not changed
		for(size_t i = 0; i < bitset_copy.size(); ++i)
		{
			CAPTURE(i);
			REQUIRE(bitset[i] == bitset_copy[i]);
		}
	}
}

TEMPLATE_TEST_CASE("bitwise operators", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	const std::tuple<unsigned long long, unsigned long long, size_t> values =
	  GENERATE(multitake(RANDOM_VECTORS_TO_TEST,
	                     randomInt<unsigned long long>(SEED),
	                     randomInt<unsigned long long>(SEED + 1),
	                     randomInt<size_t>(1, bits_number<unsigned long long>, SEED + 2)));
	unsigned long long value1 = std::get<0>(values);
	const unsigned long long value2 = std::get<1>(values);
	const size_t bits_to_take = std::get<2>(values);
	CAPTURE(value1, value2, bits_to_take);

	dynamic_bitset<TestType> bitset1(bits_to_take, value1);
	const dynamic_bitset<TestType> bitset2(bits_to_take, value2);
	CAPTURE(bitset1, bitset2);

	SECTION("assignement operators")
	{
		SECTION("operator&=")
		{
			bitset1 &= bitset2;
			value1 &= value2;

			// check bits
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset1[i] == bit_value(value1, i));
			}
		}

		SECTION("operator|=")
		{
			bitset1 |= bitset2;
			value1 |= value2;

			// check bits
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset1[i] == bit_value(value1, i));
			}
		}

		SECTION("operator^=")
		{
			bitset1 ^= bitset2;
			value1 ^= value2;

			// check bits
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset1[i] == bit_value(value1, i));
			}
		}

		SECTION("operator-=")
		{
			bitset1 -= bitset2;
			value1 &= ~value2;

			// check bits
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset1[i] == bit_value(value1, i));
			}
		}
	}

	SECTION("binary operators")
	{
		SECTION("operator&")
		{
			const dynamic_bitset<TestType> bitset = bitset1 & bitset2;
			const unsigned long long value = value1 & value2;

			// check bits
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bit_value(value, i));
			}
		}

		SECTION("operator|")
		{
			const dynamic_bitset<TestType> bitset = bitset1 | bitset2;
			const unsigned long long value = value1 | value2;

			// check bits
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bit_value(value, i));
			}
		}

		SECTION("operator^")
		{
			const dynamic_bitset<TestType> bitset = bitset1 ^ bitset2;
			const unsigned long long value = value1 ^ value2;

			// check bits
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bit_value(value, i));
			}
		}

		SECTION("operator-")
		{
			const dynamic_bitset<TestType> bitset = bitset1 - bitset2;
			const unsigned long long value = value1 & ~value2;

			// check bits
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bit_value(value, i));
			}
		}
	}
}

TEMPLATE_TEST_CASE("shift operators", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	const std::tuple<unsigned long long, size_t, size_t> values =
	  GENERATE(multitake(RANDOM_VECTORS_TO_TEST,
	                     randomInt<unsigned long long>(SEED),
	                     randomInt<size_t>(1, bits_number<unsigned long long>, SEED + 1),
	                     randomInt<size_t>(0, bits_number<unsigned long long> - 1, SEED + 2)));
	unsigned long long value = std::get<0>(values);
	const size_t bits_to_take = std::get<1>(values);
	const size_t shift = std::get<2>(values);
	CAPTURE(value, bits_to_take, shift);

	dynamic_bitset<TestType> bitset(bits_to_take, value);
	CAPTURE(bitset);

	SECTION("assignement operators")
	{
		SECTION("operator<<=")
		{
			bitset <<= shift;
			value <<= shift;

			// check bits
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bit_value(value, i));
			}
		}

		SECTION("operator>>=")
		{
			bitset >>= shift;
			value &=
			  ~static_cast<unsigned long long>(0)
			  >> (bits_number<unsigned long long> - bits_to_take); // set not taken left bits to 0
			value >>= shift;

			// check bits
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bit_value(value, i));
			}
		}
	}

	SECTION("binary operators")
	{
		SECTION("operator<<")
		{
			const dynamic_bitset<TestType> shifted_bitset = bitset << shift;
			const unsigned long long shifted_value = value << shift;

			// check bits
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				CAPTURE(i);
				REQUIRE(shifted_bitset[i] == bit_value(shifted_value, i));
			}
		}

		SECTION("operator>>")
		{
			const dynamic_bitset<TestType> shifted_bitset = bitset >> shift;
			const unsigned long long shifted_value =
			  (value
			   & (~static_cast<unsigned long long>(0)
			      >> (bits_number<unsigned long long> - bits_to_take)))
			  >> shift;

			// check bits
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				CAPTURE(i);
				REQUIRE(shifted_bitset[i] == bit_value(shifted_value, i));
			}
		}
	}
}

TEMPLATE_TEST_CASE("operator~", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	const std::tuple<unsigned long long, size_t> values =
	  GENERATE(multitake(RANDOM_VECTORS_TO_TEST,
	                     randomInt<unsigned long long>(SEED),
	                     randomInt<size_t>(1, bits_number<unsigned long long>, SEED + 1)));
	unsigned long long value = std::get<0>(values);
	const size_t bits_to_take = std::get<1>(values);
	CAPTURE(value, bits_to_take);

	dynamic_bitset<TestType> bitset(bits_to_take, value);
	CAPTURE(bitset);

	// operator~
	bitset = ~bitset;
	value = ~value;

	// check bits
	for(size_t i = 0; i < bits_to_take; ++i)
	{
		CAPTURE(i);
		REQUIRE(bitset[i] == bit_value(value, i));
	}
}

TEMPLATE_TEST_CASE("set reset flip", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	dynamic_bitset<TestType> bitset = GENERATE(take(
	  RANDOM_VECTORS_TO_TEST,
	  randomDynamicBitset<TestType>(3 * bits_number<TestType>, 8 * bits_number<TestType>, SEED)));
	CAPTURE(bitset);

	SECTION("range")
	{
		const std::tuple<size_t, size_t> values = GENERATE(multitake(
		  RANDOM_VARIATIONS_TO_TEST, randomInt<size_t>(SEED + 1), randomInt<size_t>(SEED + 2)));
		const size_t pos = std::get<0>(values) % bitset.size();
		const size_t len = std::get<1>(values) % (bitset.size() - pos);
		CAPTURE(pos, len);

		const dynamic_bitset<TestType> bitset_copy = bitset;

		SECTION("set")
		{
			const bool set_to = GENERATE(true, false);
			bitset.set(pos, len, set_to);

			// check bits
			for(size_t i = 0; i < pos; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
			for(size_t i = 0; i < len; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[pos + i] == set_to);
			}
			for(size_t i = pos + len; i < bitset.size(); ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
		}

		SECTION("reset")
		{
			bitset.reset(pos, len);

			// check bits
			for(size_t i = 0; i < pos; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
			for(size_t i = 0; i < len; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[pos + i] == false);
			}
			for(size_t i = pos + len; i < bitset.size(); ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
		}

		SECTION("flip")
		{
			bitset.flip(pos, len);

			// check bits
			for(size_t i = 0; i < pos; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
			for(size_t i = 0; i < len; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[pos + i] != bitset_copy[pos + i]);
			}
			for(size_t i = pos + len; i < bitset.size(); ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
		}
	}

	SECTION("sigle bit")
	{
		const size_t pos =
		  GENERATE(take(RANDOM_VARIATIONS_TO_TEST, randomInt<size_t>(SEED + 1))) % bitset.size();
		CAPTURE(pos);

		const dynamic_bitset<TestType> bitset_copy = bitset;

		SECTION("set")
		{
			const bool set_to = GENERATE(true, false);
			bitset.set(pos, set_to);

			// check bits
			for(size_t i = 0; i < pos; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
			REQUIRE(bitset[pos] == set_to);
			for(size_t i = pos + 1; i < bitset.size(); ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
		}

		SECTION("reset")
		{
			bitset.reset(pos);

			// check bits
			for(size_t i = 0; i < pos; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
			REQUIRE(bitset[pos] == false);
			for(size_t i = pos + 1; i < bitset.size(); ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
		}

		SECTION("flip")
		{
			bitset.flip(pos);

			// check bits
			for(size_t i = 0; i < pos; ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
			REQUIRE(bitset[pos] != bitset_copy[pos]);
			for(size_t i = pos + 1; i < bitset.size(); ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] == bitset_copy[i]);
			}
		}
	}

	SECTION("all bits")
	{
		SECTION("set")
		{
			bitset.set();

			// check bits
			REQUIRE(bitset.all());
		}

		SECTION("reset")
		{
			bitset.reset();

			// check bits
			REQUIRE(bitset.none());
		}

		SECTION("flip")
		{
			const dynamic_bitset<TestType> bitset_copy = bitset;
			bitset.flip();

			// check bits
			for(size_t i = 0; i < bitset.size(); ++i)
			{
				CAPTURE(i);
				REQUIRE(bitset[i] != bitset_copy[i]);
			}
		}
	}
}

TEMPLATE_TEST_CASE("test", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	const std::tuple<unsigned long long, size_t> values =
	  GENERATE(multitake(RANDOM_VECTORS_TO_TEST,
	                     randomInt<unsigned long long>(SEED),
	                     randomInt<size_t>(1, bits_number<unsigned long long>, SEED + 1)));
	unsigned long long value = std::get<0>(values);
	const size_t bits_to_take = std::get<1>(values);
	CAPTURE(value, bits_to_take);

	dynamic_bitset<TestType> bitset(bits_to_take, value);
	CAPTURE(bitset);

	// check
	for(size_t i = 0; i < bits_to_take; ++i)
	{
		CAPTURE(i);
		REQUIRE(bitset.test(i) == bit_value(value, i));
	}
}

TEMPLATE_TEST_CASE("test_set", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	const std::tuple<unsigned long long, size_t> values =
	  GENERATE(multitake(RANDOM_VECTORS_TO_TEST,
	                     randomInt<unsigned long long>(SEED),
	                     randomInt<size_t>(1, bits_number<unsigned long long>, SEED + 1)));
	unsigned long long value = std::get<0>(values);
	const size_t bits_to_take = std::get<1>(values);
	const bool set_to = GENERATE(true, false);
	CAPTURE(value, bits_to_take, set_to);

	dynamic_bitset<TestType> bitset(bits_to_take, value);
	CAPTURE(bitset);

	// check
	for(size_t i = 0; i < bits_to_take; ++i)
	{
		CAPTURE(i);
		REQUIRE(bitset.test_set(i, set_to) == bit_value(value, i));
		REQUIRE(bitset[i] == set_to);
	}
}

TEMPLATE_TEST_CASE("all any none", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	const size_t bitset_size =
	  GENERATE(take(RANDOM_VECTORS_TO_TEST,
	                randomInt<size_t>(3 * bits_number<TestType>, 8 * bits_number<TestType>, SEED)));
	CAPTURE(bitset_size);

	dynamic_bitset<TestType> bitset(bitset_size);
	CAPTURE(bitset);

	SECTION("all")
	{
		bitset.set();
		REQUIRE(bitset.all());
	}

	SECTION("any")
	{
		const size_t pos =
		  GENERATE(take(RANDOM_VARIATIONS_TO_TEST, randomInt<size_t>(SEED + 1))) % bitset_size;
		bitset.reset();
		bitset.set(pos);
		REQUIRE(bitset.any());
	}

	SECTION("all")
	{
		bitset.reset();
		REQUIRE(bitset.none());
	}
}

TEMPLATE_TEST_CASE("count", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	dynamic_bitset<TestType> bitset =
	  GENERATE(take(RANDOM_VECTORS_TO_TEST, randomDynamicBitset<TestType>(SEED)));
	CAPTURE(bitset);

	size_t count = 0;
	for(size_t i = 0; i < bitset.size(); ++i)
	{
		count += bitset[i];
	}

	REQUIRE(bitset.count() == count);
}

TEMPLATE_TEST_CASE("operator[]", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	const std::tuple<unsigned long long, size_t> values =
	  GENERATE(multitake(RANDOM_VECTORS_TO_TEST,
	                     randomInt<unsigned long long>(SEED),
	                     randomInt<size_t>(1, bits_number<unsigned long long>, SEED + 1)));
	unsigned long long value = std::get<0>(values);
	const size_t bits_to_take = std::get<1>(values);
	CAPTURE(value, bits_to_take);

	dynamic_bitset<TestType> bitset(bits_to_take, value);
	CAPTURE(bitset);

	SECTION("access")
	{
		for(size_t i = 0; i < bits_to_take; ++i)
		{
			CAPTURE(i);
			REQUIRE(bitset[i] == bit_value(value, i));
		}
	}

	SECTION("reference operator=")
	{
		const unsigned long long other_value =
		  GENERATE(take(1, randomInt<unsigned long long>(SEED + 2)));
		dynamic_bitset<TestType> other_bitset(bits_to_take, other_value);
		CAPTURE(other_bitset);

		SECTION("with bool")
		{
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				bitset[i] = other_bitset.test(i);
			}
			REQUIRE(bitset == other_bitset);
		}

		SECTION("with reference")
		{
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				bitset[i] = other_bitset[i];
			}
			REQUIRE(bitset == other_bitset);
		}
	}

	SECTION("reference binary operators")
	{
		const unsigned long long other_value =
		  GENERATE(take(1, randomInt<unsigned long long>(SEED + 2)));
		dynamic_bitset<TestType> other_bitset(bits_to_take, other_value);
		CAPTURE(other_bitset);

		dynamic_bitset<TestType> bitset_copy = bitset;

		SECTION("operator&=")
		{
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				bitset[i] &= other_bitset[i];
			}

			bitset_copy &= other_bitset;
			REQUIRE(bitset == bitset_copy);
		}

		SECTION("operator|=")
		{
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				bitset[i] |= other_bitset[i];
			}

			bitset_copy |= other_bitset;
			REQUIRE(bitset == bitset_copy);
		}

		SECTION("operator^=")
		{
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				bitset[i] ^= other_bitset[i];
			}

			bitset_copy ^= other_bitset;
			REQUIRE(bitset == bitset_copy);
		}

		SECTION("operator-=")
		{
			for(size_t i = 0; i < bits_to_take; ++i)
			{
				bitset[i] -= other_bitset[i];
			}

			bitset_copy -= other_bitset;
			REQUIRE(bitset == bitset_copy);
		}
	}

	SECTION("reference operator~")
	{
		for(size_t i = 0; i < bits_to_take; ++i)
		{
			CAPTURE(i);
			REQUIRE(~bitset[i] == !bit_value(value, i));
		}
	}

	SECTION("set reset flip assign")
	{
		const size_t pos =
		  GENERATE(take(RANDOM_VARIATIONS_TO_TEST, randomInt<size_t>(SEED + 2))) % bits_to_take;

		SECTION("set")
		{
			bitset[pos].set();
			REQUIRE(bitset[pos] == true);
		}

		SECTION("reset")
		{
			bitset[pos].reset();
			REQUIRE(bitset[pos] == false);
		}

		SECTION("flip")
		{
			bitset[pos].flip();
			REQUIRE(bitset[pos] == !bit_value(value, pos));
		}

		SECTION("assign")
		{
			const bool new_value = GENERATE(true, false);
			bitset[pos].assign(new_value);
			REQUIRE(bitset[pos] == new_value);
		}
	}
}

TEMPLATE_TEST_CASE("size num_blocks empty capacity",
                   "[dynamic_bitset]",
                   uint16_t,
                   uint32_t,
                   uint64_t)
{
	CAPTURE(SEED);
	const std::tuple<unsigned long long, size_t> values =
	  GENERATE(multitake(RANDOM_VECTORS_TO_TEST,
	                     randomInt<unsigned long long>(SEED),
	                     randomInt<size_t>(1, bits_number<unsigned long long>, SEED + 1)));
	unsigned long long value = std::get<0>(values);
	const size_t bits_to_take = std::get<1>(values);
	CAPTURE(value, bits_to_take);

	dynamic_bitset<TestType> bitset(bits_to_take, value);
	CAPTURE(bitset);

	// size
	REQUIRE(bitset.size() == bits_to_take);

	// num_blocks
	const size_t num_blocks =
	  bits_to_take
	    / bits_number<TestType> + static_cast<size_t>(bits_to_take % bits_number<TestType>> 0);
	REQUIRE(bitset.num_blocks() == num_blocks);

	const size_t old_capacity = bitset.capacity();
	bitset.clear();
	REQUIRE(bitset.empty());

	REQUIRE(bitset.capacity() == old_capacity);
}

TEMPLATE_TEST_CASE("reserve shrink_to_fit", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	const size_t size = GENERATE(
	  take(RANDOM_VARIATIONS_TO_TEST, randomInt<size_t>(1, 8 * bits_number<TestType>, SEED)));

	dynamic_bitset<TestType> bitset;
	bitset.reserve(size);

	REQUIRE(bitset.capacity() > 0);

	bitset.shrink_to_fit();
}

TEMPLATE_TEST_CASE("is_subset_of is_proper_subset_of",
                   "[dynamic_bitset]",
                   uint16_t,
                   uint32_t,
                   uint64_t)
{
	CAPTURE(SEED);
	dynamic_bitset<TestType> bitset =
	  GENERATE(take(RANDOM_VECTORS_TO_TEST, randomDynamicBitset<TestType>(SEED)));
	CAPTURE(bitset);

	const dynamic_bitset<TestType> bitset_copy = bitset;

	SECTION("subset")
	{
		REQUIRE(bitset.is_subset_of(bitset_copy) == true);
		REQUIRE(bitset.is_proper_subset_of(bitset_copy) == false);
	}

	SECTION("proper subset")
	{
		if(bitset.any())
		{
			bitset.flip(bitset.find_first());
			REQUIRE(bitset.is_subset_of(bitset_copy) == true);
			REQUIRE(bitset.is_proper_subset_of(bitset_copy) == true);
		}
	}

	SECTION("not a subset")
	{
		if(!bitset.all())
		{
			// flip first bit at 0
			for(size_t i = 0; i < bitset.size(); ++i)
			{
				if(bitset.test(i) == false)
				{
					bitset.flip(i);
					break;
				}
			}
			REQUIRE(bitset.is_subset_of(bitset_copy) == false);
			REQUIRE(bitset.is_proper_subset_of(bitset_copy) == false);
		}
	}
}

TEMPLATE_TEST_CASE("intersects", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	dynamic_bitset<TestType> bitset =
	  GENERATE(take(RANDOM_VECTORS_TO_TEST, randomDynamicBitset<TestType>(SEED)));
	CAPTURE(bitset);

	// require non 0-only bitset
	if(bitset.none())
	{
		bitset.push_back(true);
	}

	const dynamic_bitset<TestType> bitset_copy = bitset;

	REQUIRE(bitset.intersects(bitset_copy));

	if(!bitset.all())
	{
		// flip first bit at 0
		for(size_t i = 0; i < bitset.size(); ++i)
		{
			if(bitset.test(i) == false)
			{
				bitset.flip(i);
				break;
			}
		}
		REQUIRE(bitset.intersects(bitset_copy));
	}

	bitset.flip();
	REQUIRE_FALSE(bitset.intersects(bitset_copy));
	bitset.reset();
	REQUIRE_FALSE(bitset.intersects(bitset_copy));
}

TEMPLATE_TEST_CASE("find_first find_next", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	const std::tuple<size_t, size_t> values =
	  GENERATE(multitake(RANDOM_VECTORS_TO_TEST,
	                     randomInt<size_t>(0, 5 * bits_number<TestType>, SEED),
	                     randomInt<size_t>(0, 5 * bits_number<TestType>, SEED + 1)));
	const size_t first_bit_pos = std::get<0>(values);
	const size_t second_bit_pos = first_bit_pos + 1 + std::get<1>(values);
	CAPTURE(first_bit_pos, second_bit_pos);

	dynamic_bitset<TestType> bitset(second_bit_pos + 12);
	bitset.set(first_bit_pos);
	bitset.set(second_bit_pos);
	REQUIRE(bitset.find_first() == first_bit_pos);
	REQUIRE(bitset.find_next(first_bit_pos) == second_bit_pos);
	REQUIRE(bitset.find_next(second_bit_pos) == bitset.npos);
}

TEMPLATE_TEST_CASE("swap", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	CAPTURE(SEED);
	const std::tuple<dynamic_bitset<TestType>, dynamic_bitset<TestType>> bitsets =
	  GENERATE(multitake(RANDOM_VECTORS_TO_TEST,
	                     randomDynamicBitset<TestType>(SEED),
	                     randomDynamicBitset<TestType>(SEED + 1)));
	dynamic_bitset<TestType> bitset1 = std::get<0>(bitsets);
	dynamic_bitset<TestType> bitset2 = std::get<1>(bitsets);
	CAPTURE(bitset1, bitset2);

	SECTION("member function")
	{
		const dynamic_bitset<TestType> bitset1_copy = bitset1;
		const dynamic_bitset<TestType> bitset2_copy = bitset2;
		bitset1.swap(bitset2);
		REQUIRE(bitset1 == bitset2_copy);
		REQUIRE(bitset2 == bitset1_copy);
	}

	SECTION("external function")
	{
		const dynamic_bitset<TestType> bitset1_copy = bitset1;
		const dynamic_bitset<TestType> bitset2_copy = bitset2;
		swap(bitset1, bitset2);
		REQUIRE(bitset1 == bitset2_copy);
		REQUIRE(bitset2 == bitset1_copy);
	}
}

TEMPLATE_TEST_CASE("get_allocator", "[dynamic_bitset]", uint16_t, uint32_t, uint64_t)
{
	dynamic_bitset<TestType> bitset;
	static_cast<void>(bitset.get_allocator()); // avoid unused warnings
}
