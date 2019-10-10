#pragma once

#define BOOST_TEST_MODULE Lao_TestSuite
#include <boost/test/included/unit_test.hpp>
#include <seastar/core/app-template.hh>
#include <seastar/core/coroutine.hh>
#include <seastar/core/fstream.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/sleep.hh>
#include "common.hh"

BEGIN_NAMESPACE(laomd)

void run_test(std::function<seastar::future<>()>&& func) {
    seastar::app_template app;
    char* argv[1];
    argv[0] = new char[1]{'a'};
    app.run(1, argv, std::move(func));
    delete argv[0];
}

END_NAMESPACE(laomd)