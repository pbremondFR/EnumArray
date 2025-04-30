#include "include/EnumArray.hpp"
#include <cstdio>
#include <cstdlib>
#include <ctime>

enum Foo {
	One = 1,
	Two = 2,
	Three = 3,
	Five = 5,
	Hundred = 100,
};

int	main()
{
	std::srand((unsigned int)std::time(nullptr));
	{
		constexpr pb::EnumArrayContiguous<int, Foo::One, Foo::Three> foo = {1, 2, 3};
		for (auto i : foo)
			printf("%d ", i);
		printf("\n");
		for (Foo i = Foo::One; i <= Foo::Three; i = Foo(i+1))
			printf("%d ", foo.at(i));
		printf("\n");

	}
	{
		constexpr pb::EnumArraySparse<int, Foo::One, Foo::Hundred> foo = { 1, 2, 3, 4, 5 };
		for (auto i : foo)
			printf("%d ", i);
		printf("\n");
		using enum Foo;
		for (auto e : {One, Two, Three, Five, Hundred})
			printf("%d ", foo.at(e));
		printf("\n");
	}
	{
		pb::EnumArraySparse<int, Foo::One, Foo::Three> foo = { 1, 2, 3 };
		int random = (std::rand() % 3) + 1;
		printf("%d\n", foo[(Foo)random]);
	}
	{
		constexpr pb::EnumArrayContiguous<int, Foo::One, Foo::Three> foo = { 1, 2, 3 };
		int result = foo[Foo::Two];
		printf("Result: %d\n", result);
	}
	{
		constexpr pb::EnumArraySparse<int, Foo::One, Foo::Hundred> foo = { 1, 2, 3, 4, 5 };
		int result = foo[Foo::Five];
		printf("Result: %d\n", result);
	}
}
