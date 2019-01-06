#include <vector>
#include <cstdio>
#include "catch.hpp"
#include <cedar/runes.h>

TEST_CASE("runes can be created and modified", "[runes]") {
  GIVEN("An empty rune structure") {
		cedar::runes subject;

    REQUIRE(subject.size() == 0);
    REQUIRE(subject.capacity() >= 0);


		WHEN("A string is appended") {
			subject += "test";
			THEN("the size and capacity will change") {
				REQUIRE(subject.size() == 4);
				REQUIRE(subject.capacity() >= 4);
			}
		}

		WHEN("The string is cleared") {
			subject.clear();
			THEN("the size will be 0 and the capacity will be valid") {
				REQUIRE(subject.size() == 0);
				REQUIRE(subject.capacity() >= 0);
			}
		}


		WHEN("A char is appended") {
			subject.push_back('A');
			THEN("The size will be 1") {
				REQUIRE(subject.size() == 1);
				REQUIRE(subject.capacity() >= 1);
			}
		}


		WHEN("300 chars are appended") {
			subject.clear();
			for (int i = 0; i < 300; i++) {
				subject.push_back('A');
			}
			THEN("the size will be 300") {
				REQUIRE(subject.size() == 300);
				REQUIRE(subject.capacity() >= 300);
			}
		}

		WHEN("Rune is constructed from a `const char*`") {
			cedar::runes test = "test";
			THEN("the size will be correct") {
				REQUIRE(test.size() == 4);
				REQUIRE(test.capacity() >= 4);
			}
		}

		WHEN("Rune is constructed from a `const char32_t*`") {
			cedar::runes test = U"test";
			THEN("the size will be correct") {
				REQUIRE(test.size() == 4);
				REQUIRE(test.capacity() >= 4);
			}
		}


		WHEN("Rune is constructed from a `const char*` with unicode") {
			cedar::runes test = "ᚳᚹᚫᚦ";
			THEN("the size will be correct") {
				REQUIRE(test.size() == 4);
				REQUIRE(test.capacity() >= 4);
			}
		}


		WHEN("Rune is constructed from a `const char32_t*` with unicode") {
			cedar::runes test = U"ᚳᚹᚫᚦ";
			THEN("the size will be correct") {
				REQUIRE(test.size() == 4);
				REQUIRE(test.capacity() >= 4);
			}
		}

		WHEN("Rune is constructed with an emoji") {
			cedar::runes test = U"👩";
			THEN("the size will be correct") {
				REQUIRE(test.size() == 1);
				REQUIRE(test.capacity() >= 1);
			}
		}

  }
}
