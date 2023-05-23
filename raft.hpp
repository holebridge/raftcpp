#ifndef __HOLEBRIDGE_RAFT_HPP__
#define __HOLEBRIDGE_RAFT_HPP__
#include <vector>
#include <memory>
#include <fstream>

#include <grpcpp/grpcpp.h>
#include "pb/raftcpp.grpc.pb.h"
#include "random_interval_timer.hpp"


using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

using grpc::Status;

using raftcpp::AppendEntriesReply;
using raftcpp::AppendEntriesRequest;
using raftcpp::InstallSnapshotReply;
using raftcpp::IstallSnapshotRequest;
using raftcpp::RaftService;
using raftcpp::RequestVoteReply;
using raftcpp::RequestVoteRequest;

enum class RaftState
{
    Follower,
    Candidate,
    Leader
};

class RaftClient {
public:
	RaftClient(const std::string& member_addr);
    std::pair<int32_t, bool> RequestVote(const RequestVoteRequest& request);
    
    std::pair<int32_t, bool> AppendEntries(const AppendEntriesRequest& request);

    Status InstallSnapshot(ClientContext* context, const IstallSnapshotRequest& request, InstallSnapshotReply* response);
    
    const std::string& remoteAddr() const;
private:
	std::unique_ptr<RaftService::Stub> stub_;
    const std::string& remote_addr_;
};

static void raft_declare();

class RaftServer final : public RaftService::Service {
public:
    friend static void raft_declare();
    static void init(const std::string serve_addr, const std::vector<std::string> members_addr);
    void start();

    Status RequestVote(ServerContext *context, const RequestVoteRequest *request, RequestVoteReply *reply);
    Status AppendEntries(ServerContext *context, const AppendEntriesRequest *request, AppendEntriesReply *reply);

    const int currentTerm() const;
    void currentTerm(int32_t term);

    void voteFor(const std::string candidate_id);
    const std::string voteFor() const;

    static std::string id_;
    static RaftState state_;
    static std::vector<std::unique_ptr<RaftClient>> clients_to_others_;

    static RandomIntervalTimer election_timer_;
    static RandomIntervalTimer heartbeat_timer_;


};

#endif // !__HOLEBRIDGE_RAFT_HPP__