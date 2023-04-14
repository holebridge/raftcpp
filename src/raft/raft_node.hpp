#ifndef __RAFTCPP_RAFTNODE_HPP__
#define __RAFTCPP_RAFTNODE_HPP__

#include <string>
#include <iostream>
#include <vector>
#include <grpcpp/grpcpp.h>

#include "../pb/raftcpp.grpc.pb.h"
#include "raft_timer.hpp"
#include "raft_store.hpp"
#include "raft_client.hpp"

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

class RaftNode : public RaftService::Service
{
public:
    RaftNode(std::vector<std::string> member_addr_list, const std::string &serve_addr)
        : _member_addr_list(member_addr_list), _serve_addr(serve_addr),
        _election_timer(150, 300), _heartbeat_timer(100, 100), _store(serve_addr), _state(RaftState::Follower)
    {
        for (auto &member_addr : member_addr_list)
        {
            if (member_addr != serve_addr)
            {
                _clients.emplace_back(std::make_unique<RaftClient>(member_addr));
            }
        }

    }

    void Start() {

        auto heartbeat = [&, this]()
        {
            if (_state == RaftState::Leader)
            {
                AppendEntriesRequest request;
                request.set_term(_store.currentTerm());
                request.set_leaderid(_serve_addr);
                for (auto& client : _clients)
                {
                    auto [term, success] = client->AppendEntries(request);
                    if (term > _store.currentTerm())
                    {
                        _store.voteFor(client->_RemoteAddr());
                        _store.currentTerm(term);
                        _state = RaftState::Follower;
                        _election_timer.Reset();
                        break;
                    }
                }
            }
        };

        auto start_election = [&, this]()
        {
            if (_state == RaftState::Leader) {
                return;
            }
            if (_state == RaftState::Follower)
            {
                _state = RaftState::Candidate;
                
            }
            _store.voteFor("");
            _store.increaseTerm();
            int32_t votes = 0;
            RequestVoteRequest request;

            request.set_term(_store.currentTerm());
            request.set_candidateid(_serve_addr);

            for (auto& client : _clients)
            {
                auto [term, granted] = client->RequestVote(request);
                if (term > _store.currentTerm())
                {
                    _store.voteFor(client->_RemoteAddr());
                    _state = RaftState::Follower;
                    _store.currentTerm(term);
                }
                else if (granted)
                {
                    if (++votes > _member_addr_list.size() / 2)
                    {
                        break;
                    }
                }
            }
            if (votes > _member_addr_list.size() / 2)
            {
                _state = RaftState::Leader;
                std::cerr << "I win!!! " << _serve_addr << std::endl;
                heartbeat();
            }
            std::cerr << "term:" << _store.currentTerm() << " voteFor:" << _store.voteFor() << " votes:" << votes << std::endl;
        };

        _heartbeat_timer.Start(heartbeat);
        _election_timer.Start(start_election);
    }

    Status RequestVote(ServerContext *context, const RequestVoteRequest *request, RequestVoteReply *reply) override
    {
        if (_state == RaftState::Follower)
        {
            if (request->term() < _store.currentTerm() || _store.voteFor() != "")
            {
                reply->set_term(_store.currentTerm());
                reply->set_votegranted(false);
                return Status::OK;
            }

            _store.currentTerm(request->term());
            _store.voteFor(request->candidateid());

            reply->set_term(request->term());
            reply->set_votegranted(true);

            _election_timer.Reset();

            return Status::OK;
        }
        else if (_state == RaftState::Candidate)
        {
            if (request->term() > _store.currentTerm())
            {

                _store.voteFor(request->candidateid());
                _store.currentTerm(request->term());
                _state = RaftState::Follower;

                reply->set_term(request->term());
                reply->set_votegranted(true);

                _election_timer.Reset();
                return Status::OK;
            }

            reply->set_term(_store.currentTerm());
            reply->set_votegranted(false);

            return Status::OK;
        }
        else
        {

            reply->set_term(_store.currentTerm());
            reply->set_votegranted(false);

            return Status::OK;
        }
    }

    Status AppendEntries(ServerContext *context, const AppendEntriesRequest *request, AppendEntriesReply *reply) override
    {
        if (request->entries_size() == 0)
        {
            if (_state == RaftState::Follower || _state == RaftState::Candidate)
            {
                if (request->term() < _store.currentTerm())
                {
                    reply->set_term(_store.currentTerm());
                    reply->set_success(false);
                    return Status::OK;
                }

                _store.voteFor(request->leaderid());
                _store.currentTerm(request->term());
                _state = RaftState::Follower;

                reply->set_term(request->term());
                reply->set_success(true);

                _election_timer.Reset();
                return Status::OK;
            }
        }

        reply->set_term(request->term());
        reply->set_success(false);
        return Status::OK;
    }

    Status InstallSnapshot(ServerContext *context, const IstallSnapshotRequest *request, InstallSnapshotReply *response) override
    {
        return Status::OK;
    }

private:
    std::vector<std::string> _member_addr_list;
    std::string _serve_addr;
    RaftTimer _election_timer;
    RaftTimer _heartbeat_timer;
    RaftStore _store;
    std::vector<std::unique_ptr<RaftClient>> _clients;
    RaftState _state;
};

#endif