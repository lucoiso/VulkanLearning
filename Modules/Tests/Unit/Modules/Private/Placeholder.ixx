// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

export module Unit.Placeholder;

#define BOOST_TEST_MODULE PLACEHOLDER_TEST_MODULE
#include "boost/test/included/unit_test.hpp"

BOOST_AUTO_TEST_CASE(Placeholder)
{
    BOOST_TEST(true);
}

#undef BOOST_TEST_MODULE
