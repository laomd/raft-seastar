#include "common/test_case.hh"
using namespace laomd::raft;

RAFT_TEST_CASE(InitialElection2A) {
  return env->checkOneLeader().discard_result();
}

RAFT_TEST_CASE(ReElection2A) {
  return env->checkOneLeader()
      .then([this](laomd::raft::id_t leader) {
        return env->kill(leader).then([this, leader] {
          // if the leader disconnects, a new one should be elected.
          return env->checkOneLeader().then([leader](auto new_leader) {
            BOOST_REQUIRE_NE(leader, new_leader);
            return leader;
          });
        });
      })
      .then([this](laomd::raft::id_t leader) {
        // if the old leader rejoins, that shouldn't
        // disturb the old leader.
        return env->restart(leader).then(
            [this] { return env->checkOneLeader(); });
      })
      .then([this](auto leader) {
        // if there's no quorum, no leader should
        // be elected.
        auto leader2 = (leader + 1) % num_servers;
        return seastar::when_all_succeed(env->kill(leader), env->kill(leader2))
            .then([this] { return env->checkNoLeader(); })
            .then([leader, leader2] {
              return seastar::make_ready_future<laomd::raft::id_t,
                                                laomd::raft::id_t>(leader,
                                                                   leader2);
            });
      })
      .then([this](auto last_killed1, auto last_killed2) {
        return seastar::when_all_succeed(env->restart(last_killed1),
                                         env->restart(last_killed2))
            .then([this] { return env->checkOneLeader().discard_result(); });
      });
}

RAFT_TEST_CASE(BasicAgree2B) {
  auto range = boost::irange(1, 4);
  return seastar::do_for_each(range.begin(), range.end(), [this](int i) {
    return env->nCommitted(i).then([this, i](int nd, seastar::sstring cmd1) {
      BOOST_REQUIRE_LE(nd, 0);
      return env->Commit("entry" + seastar::to_sstring(i), num_servers);
    });
  });
}

RAFT_TEST_CASE(FailAgree2B) {
  return env->Commit("101", num_servers).then([this] {
    return env->checkOneLeader().then([this](auto leader) {
      // follower network disconnection
      auto to_stop = (leader + 1) % num_servers;
      return env->kill(to_stop)
          .then([this] {
            auto range = boost::irange(102, 106);
            return seastar::do_for_each(
                range.begin(), range.end(), [this](int i) {
                  return env->Commit(seastar::to_sstring(i), num_servers - 1);
                });
          })
          .then([this, to_stop] {
            return env->restart(to_stop).then(
                [this] { return env->Commit("106", num_servers); });
          });
    });
  });
}

RAFT_TEST_CASE(FailNoAgree2B) {
  return env->Commit("10", num_servers).then([this] {
    return env->checkOneLeader().then([this](auto leader) {
      return seastar::parallel_for_each(boost::irange(1, 4),
                                        [this, leader](int i) {
                                          return env->kill((i + leader) %
                                                           num_servers);
                                        })
          .then([this, leader] {
            return env->AppendLog(leader, "20")
                .then([this](auto index, bool ok) {
                  BOOST_REQUIRE(ok);
                  BOOST_REQUIRE_EQUAL(index, 2);
                  return seastar::sleep(2 * electionTimeout)
                      .then([this, index] {
                        return env->nCommitted(index).then(
                            [this](int n, auto) { BOOST_REQUIRE_EQUAL(n, 0); });
                      });
                });
          })
          .then([this, leader] {
            auto range = boost::irange(1, 4);
            return seastar::do_for_each(range, [this, leader](int i) {
              return env->restart((i + leader) % num_servers);
            });
          })
          .then([this] { return env->checkOneLeader(); })
          .then([this](auto new_leader) {
            return env->AppendLog(new_leader, "30")
                .then([this](auto index, bool ok) {
                  BOOST_REQUIRE(ok);
                  // if new_leader != leader, index = 2, else index =
                  // 3s
                  BOOST_REQUIRE(index == 2 || index == 3);
                  return seastar::make_ready_future();
                })
                .then([this] { return env->Commit("1000", num_servers); });
          });
    });
  });
}

RAFT_TEST_CASE(disable_ConcurrentAppend2B) {
  return env->checkOneLeader().then([this](auto leader) {
    auto range = boost::irange(0, 5);
    return seastar::do_for_each(
               range,
               [this, leader](auto i) {
                 return env->AppendLog(leader, seastar::to_sstring(100 + i))
                     .then([](auto index, bool ok) { BOOST_REQUIRE(ok); });
               })
        .then([this] {
          return seastar::do_with(true, [this](bool &success) {
            return seastar::repeat([this, &success] {
              return seastar::parallel_for_each(
                         boost::irange(0, 5),
                         [this, &success](auto i) {
                           return env->nCommitted(i + 1).then(
                               [this, &success, i](auto n, auto cmd) {
                                 if (n != num_servers) {
                                   std::cout << "log entry at index " << i + 1
                                             << " has been committed on " << n
                                             << "(!=" << num_servers
                                             << ") servers" << std::endl;
                                   success = false;
                                 }
                               });
                         })
                  .then([this, &success] {
                    if (success) {
                      return seastar::make_ready_future<
                          seastar::stop_iteration>(
                          seastar::stop_iteration::yes);
                    } else {
                      success = true;
                      return seastar::sleep(2 * electionTimeout).then([] {
                        return seastar::stop_iteration::no;
                      });
                    }
                  });
            });
          });
        });
  });
}