#include <seastar/core/app-template.hh>
#include <seastar/core/coroutine.hh>
#include <seastar/core/fstream.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/sleep.hh>

int main(int argc, char** argv) {
    seastar::app_template app;
    app.run(argc, argv, [] () -> seastar::future<> {
        std::cout << "this is a completely useless program\nplease stand by...\n";
        auto f = seastar::parallel_for_each(std::vector<int> { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, [] (int i) -> seastar::future<> {
            co_await seastar::sleep(std::chrono::seconds(i));
            std::cout << i << "\n";
        });

        auto file = co_await seastar::open_file_dma("useless_file.txt", seastar::open_flags::create | seastar::open_flags::wo);
        auto out = seastar::make_file_output_stream(file);
        seastar::sstring str = "nothing to see here, move along now\n";
        co_await out.write(str);
        co_await out.flush();
        co_await out.close();

        co_await std::move(f);
        std::cout << "done\n";
    });
}