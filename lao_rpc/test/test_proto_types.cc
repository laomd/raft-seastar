#define BOOST_TEST_MODULE First_TestSuite
#include <boost/test/included/unit_test.hpp>
#include <lao_rpc/rpc.hh>
#include <lao_rpc/test/proto/test_types.pb.h>
#include <lao_rpc/test/proto/test_types.pb.cc>

BOOST_AUTO_TEST_CASE(TestString)
{
    PbBasicTypesReq req;
}