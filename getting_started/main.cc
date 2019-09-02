#include "seastar/core/app-template.hh"
#include "seastar/core/reactor.hh"
#include <iostream>

using namespace seastar;

int main(int argc, char** argv) {
    app_template app;
    app.run(argc, argv, [] {
            std::cout << "Hello world\n";
            return make_ready_future<>(); 
    });
}