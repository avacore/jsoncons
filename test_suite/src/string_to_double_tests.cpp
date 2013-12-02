// Copyright 2013 Daniel Parker
// Distributed under Boost license

#include <boost/test/unit_test.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include "jsoncons/json.hpp"
#include "jsoncons/json_serializer.hpp"
#include "my_custom_data.hpp"
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>

using jsoncons::json_serializer;
using jsoncons::output_format;
using jsoncons::json;
using jsoncons::wjson;
using jsoncons::basic_json_reader;
using std::string;
using boost::numeric::ublas::matrix;

BOOST_AUTO_TEST_CASE(test_string_to_double)
{
    char* begin = "1.15507e-173";
    char* endptr = begin + strlen(begin);
    double value1 = 1.15507e-173;
    double value2 = strtod( begin, &endptr );
    double value3 = jsoncons::string_to_double(begin);

    BOOST_CHECK(value1 == value2);
    BOOST_CHECK(value2 == value3);
}

BOOST_AUTO_TEST_CASE(test_wstring_to_double)
{
    wchar_t* begin = L"1.15507e-173";
    wchar_t* endptr = begin + wcslen(begin);
    double value1 = 1.15507e-173;
    double value2 = wcstod( begin, &endptr );
    double value3 = jsoncons::string_to_double(begin);

    BOOST_CHECK(value1 == value2);
    BOOST_CHECK(value2 == value3);
}

