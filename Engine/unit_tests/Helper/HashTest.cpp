#include <catch2/catch.hpp>
#include <EtCore/Hashing/Hash.h>


TEST_CASE("String Hash", "[hash]")
{
	using namespace et;

	constexpr T_Hash check = "0123456789ABCDEF"_hash;
	REQUIRE(check == 141695047u);
	REQUIRE(check == GetHash("0123456789ABCDEF"));
}