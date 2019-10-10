#include <lao_utils/test.hh>
#include <mvcc/table.hpp>

BEGIN_NAMESPACE(laomd)

BOOST_AUTO_TEST_CASE(test_serial)
{
  run_test([]() -> seastar::future<> {
    MVCCTable<int> table;
    auto&& res1 = co_await table.select(1, [](int a) { return true; });
    BOOST_REQUIRE(res1.empty());

    int total = 100;
    for (int i = 0; i < total; i++) {
      BOOST_REQUIRE(co_await table.insert(2, i));
    }
    auto&& res2 = co_await table.select(1, [](int a) { return true; });
    BOOST_REQUIRE(res2.empty());

    auto&& res3 = co_await table.select(3, [](int a) { return true; });
    BOOST_REQUIRE(res3.size() == total);

    BOOST_REQUIRE(co_await table.erase(3, [](int a) { return true; }) == total);
    auto&& res4 = co_await table.select(2, [](int a) { return true; });
    BOOST_REQUIRE(res4.size() == total);

    auto&& res5 = co_await table.select(3, [](int a) { return true; });
    BOOST_REQUIRE(res5.empty());
  });
}

END_NAMESPACE(laomd)