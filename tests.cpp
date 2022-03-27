#include "constexpr_string.h"

#include <cassert>

template<size_t N>
struct ConsexprTest {
    static constexpr size_t value{ N };
};

void print_string(const char* str) {
    std::cout << str << std::endl;
}

void Test_find() {
    using namespace sw_constexpr;
    constexpr auto source_string = CONSTEXPR_STRING("Interface1::Interface2::function(int arg, double arg2, somenamespace::type    arg3)()");
    constexpr auto bracket_index = source_string.find<'('>();
    using test_type = ConsexprTest<bracket_index>;
    static_assert(test_type::value == 32);

    constexpr auto bracket_index_last = source_string.find_last<'('>();
    using test_type_lats = ConsexprTest<bracket_index_last>;
    static_assert(test_type_lats::value == 83);
}

void Test_substr() {
    using namespace sw_constexpr;
    constexpr auto source_string = CONSTEXPR_STRING("Interface1::Interface2::function(int arg, double arg2, somenamespace::type    arg3)");
    constexpr auto no_interfaces = source_string.substr<source_string.rfind<':', source_string.find<'('>()>() + 1>();
    static_assert(no_interfaces.length() == sizeof("function(int arg, double arg2, somenamespace::type    arg3)")-1);
    static_assert(no_interfaces.capacity() == sizeof("function(int arg, double arg2, somenamespace::type    arg3)"));
    assert(strcmp(no_interfaces, "function(int arg, double arg2, somenamespace::type    arg3)") == 0);
}

void Test_filter() {
    using namespace sw_constexpr;
    constexpr auto source_string = CONSTEXPR_STRING("Interface1::Interface2::function(int arg, double arg2, \t somenamespace::type    arg3)");
    constexpr auto no_spaces = source_string.filter([](char sym) constexpr {return bool(sym != ' ' && sym != '\t'); });
    static_assert(no_spaces.length() == sizeof("Interface1::Interface2::function(intarg,doublearg2,somenamespace::typearg3)") - 1);
    assert(strcmp(no_spaces, "Interface1::Interface2::function(intarg,doublearg2,somenamespace::typearg3)") == 0);
}

void Test_concat() {
    using namespace sw_constexpr;
    constexpr auto string_1 = CONSTEXPR_STRING("Hello");
    constexpr auto string_2 = CONSTEXPR_STRING(", ");
    constexpr auto string_3 = CONSTEXPR_STRING("World");
    constexpr auto string_4 = CONSTEXPR_STRING("!");
    constexpr auto result_string = string_1.concat(string_2).concat(string_3).concat(string_4);
    static_assert(result_string.length() == 13);
    assert(strcmp(result_string, "Hello, World!") == 0);
}

void Test_split() {
    using namespace sw_constexpr;
    constexpr auto source_string = CONSTEXPR_STRING("Hello , \t World ! \t");
    constexpr auto strings_tuple = source_string.split([](char sym)constexpr {return sym == ' ' || sym == '\t'; });

    static_assert(std::tuple_size_v<decltype(strings_tuple)> == 4);
    assert(strcmp(std::get<0>(strings_tuple), "Hello") == 0);
    assert(strcmp(std::get<1>(strings_tuple), ",") == 0);
    assert(strcmp(std::get<2>(strings_tuple), "World") == 0);
    assert(strcmp(std::get<3>(strings_tuple), "!") == 0);

    //std::apply([](auto&&... args) {((std::cout << (const char*)args << '\n'), ...); }, strings_tuple);
}

int main() {
    Test_find();
    Test_substr();
    Test_filter();
    Test_concat();
    Test_split();
    return 0;
}