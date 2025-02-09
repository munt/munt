/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2024 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MT32EMU_TESTING_H
#define MT32EMU_TESTING_H

#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING

#include <cstddef>
#include <doctest/doctest.h>

#define MT32EMU_NEED_WRAPPING_TEMPLATE_TEST_TYPES DOCTEST_VERSION_MAJOR < 2
#define MT32EMU_WRAP_TEMPLATE_TEST_TYPES(list, types) typedef doctest::Types<types> list;

#define MT32EMU_STRINGIFY_ENUM(E) \
String toString(E e) { \
	return String(#E"{") + ::MT32Emu::Test::toString(int(e)) + "}"; \
}

#define MT32EMU_CHECK_STRING_CONTAINS(haystack, needle) \
do { \
	bool subStringFound = strstr(haystack, needle) != NULL; \
	const char *descr = subStringFound ? " CONTAINS " : " DOES NOT CONTAIN "; \
	INFO(#haystack << descr << #needle "!"); \
	CAPTURE(haystack); \
	CAPTURE(needle); \
	CHECK(subStringFound); \
} while(false)

#define MT32EMU_CHECK_MEMORY_EQUAL(data1, data2, size) \
do { \
	if (+(data1) == data2) { \
		CHECK(data1 == data2); \
		break; \
	} \
	CAPTURE(data1); \
	CAPTURE(data2); \
	CAPTURE(size); \
	CHECK(memcmp(data1, data2, size) == 0); \
} while(false)

namespace MT32Emu {

namespace Test {

using doctest::Approx;
using doctest::String;
using doctest::toString;

// A workaround for problems with checking pointers against NULL in doctest versions before 2.4.6.
void * const NULL_PTR = NULL;

template<class A>
static inline size_t calcArraySize(A &a) {
	return sizeof a / sizeof *a;
}

template<class T>
struct Array {
	T * const data;
	const size_t size;

	Array() : data(), size() {}

	Array(T *d, size_t s) : data(d), size(s) {}

	template<class A>
	Array(A &a) : data(a), size(calcArraySize(a)) {}

	T &operator[](size_t ix) const {
		REQUIRE(ix < size);
		return data[ix];
	}
};

} // namespace Test

} // namespace MT32Emu

#endif // #ifndef MT32EMU_TESTING_H
