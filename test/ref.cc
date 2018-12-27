#include "catch.hpp"
#include <cedar/ref.hpp>


TEST_CASE("ref size is 16 bytes") {
	REQUIRE(sizeof(cedar::ref) == 16);
}
