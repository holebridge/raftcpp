#ifndef __RAFTCPP_RAFTCLIENT_HPP__
#define __RAFTCPP_RAFTCLIENT_HPP__

#include <tuple>
#include <memory>

#include <grpcpp/grpcpp.h>
#include "../pb/raftcpp.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using raftcpp::RequestVoteRequest;
using raftcpp::RequestVoteReply;
using raftcpp::AppendEntriesRequest;
using raftcpp::AppendEntriesReply;
using raftcpp::IstallSnapshotRequest;
using raftcpp::InstallSnapshotReply;
using raftcpp::RaftService;


class RaftClient {
public:
	RaftClient(const std::string& member_addr)
    : stub(RaftService::NewStub(grpc::CreateChannel(member_addr, grpc::InsecureChannelCredentials())))
    {
        remote_addr = member_addr;
    }
    std::pair<int32_t, bool> RequestVote(const RequestVoteRequest& request) {
        ClientContext context;
        RequestVoteReply response;
        auto status = stub->RequestVote(&context, request, &response);
        if(!status.ok()) {
            return std::pair<int32_t, bool>{-1, false};
        }
        return std::pair<int32_t, bool>{response.term(), response.votegranted()};
    }
    
    std::pair<int32_t, bool> AppendEntries(const AppendEntriesRequest& request) {

        ClientContext context;
        AppendEntriesReply response;
        auto status = stub->AppendEntries(&context, request, &response);
        if(!status.ok()) {
            return std::pair<int32_t, bool>{-1, false}; 
        }
        return std::pair<int32_t,  bool>{response.term(), response.success()};
    }

    Status InstallSnapshot(ClientContext* context, const IstallSnapshotRequest& request, InstallSnapshotReply* response) {
        return stub->InstallSnapshot(context, request, response);
    }
    
    const std::string& _RemoteAddr() const {
        return remote_addr;
    }
private:
	std::unique_ptr<RaftService::Stub> stub;
    std::string remote_addr;
};

#endif // !__RAFTCPP_RAFTCLIENT_HPP__