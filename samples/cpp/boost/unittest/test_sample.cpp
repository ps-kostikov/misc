#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

BOOST_AUTO_TEST_CASE(test)
{
    BOOST_CHECK_EQUAL(1, 1);
}
