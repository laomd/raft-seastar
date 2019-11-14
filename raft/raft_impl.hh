#include "service.smf.fb.h"
#include <lao_utils/common.hh>
#include <seastar/core/timer.hh>
#include <smf/log.h>
#include <smf/rpc_filter.h>
#include <smf/rpc_server.h>
#include <vector>

BEGIN_NAMESPACE(laomd)
BEGIN_NAMESPACE(raft)

#define null 0ul
#define HEART_BEAT_TIMEOUT (2000ms)
#define ELECTION_TIMEOUT (200ms)

class RaftImpl : public Raft {
public:
  RaftImpl(uint16_t server_id, const std::vector<seastar::ipv4_addr>& other_servers);
  virtual seastar::future<smf::rpc_typed_envelope<VoteResponse>>
  RequestVote(smf::rpc_recv_typed_context<VoteRequest> &&rec) override;
private:
  void Persist() const;
  seastar::future<> OnTimer();
  seastar::future<> StartElection();

  ServerState state_;
  const uint16_t server_id_;
  using clock_type = seastar::lowres_clock;
  seastar::timer<clock_type> timer_;
  clock_type::time_point lastHeartbeat_;
  std::vector<seastar::shared_ptr<RaftClient>> other_servers_;
  std::atomic<uint64_t> voted_count_;
  // Persistent state on all servers: Updated on stable storage before responding to RPCs)
  uint64_t currentTerm_;
  uint64_t votedFor_;
  std::vector<std::pair<uint64_t, std::string>> log_;
  // Volatile state on all servers:
  uint64_t commitIndex_;
  uint64_t lastApplied_;
  // Volatile state on leaders: Reinitialized after election)
  std::vector<uint64_t> nextIndex_, matchIndex_;
};

END_NAMESPACE(raft)
END_NAMESPACE(laomd)