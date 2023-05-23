#include "raft.hpp"

std::string RaftServer::id_;
RaftState RaftServer::state_;
std::vector<std::unique_ptr<RaftClient>> RaftServer::clients_to_others_;
RandomIntervalTimer RaftServer::election_timer_(150, 350);
RandomIntervalTimer RaftServer::heartbeat_timer_(100, 100);

RaftClient::RaftClient(const std::string& member_addr)
: remote_addr_(member_addr), stub_(RaftService::NewStub(grpc::CreateChannel(member_addr, grpc::InsecureChannelCredentials()))) {}

std::pair<int32_t, bool> RaftClient::RequestVote(const RequestVoteRequest& request) {
    ClientContext context;
    RequestVoteReply response;
    auto status = stub_->RequestVote(&context, request, &response);
    if(!status.ok()) {
        return std::pair<int32_t, bool>{-1, false};
    }
    return std::pair<int32_t, bool>{response.term(), response.votegranted()};
}

std::pair<int32_t, bool> RaftClient::AppendEntries(const AppendEntriesRequest& request) {

    ClientContext context;
    AppendEntriesReply response;
    auto status = stub_->AppendEntries(&context, request, &response);
    if(!status.ok()) {
        return std::pair<int32_t, bool>{-1, false}; 
    }
    return std::pair<int32_t,  bool>{response.term(), response.success()};
}

Status RaftClient::InstallSnapshot(ClientContext* context, const IstallSnapshotRequest& request, InstallSnapshotReply* response) {
    return stub_->InstallSnapshot(context, request, response);
}

const std::string& RaftClient::remoteAddr() const {
    return remote_addr_;
}

void RaftServer::init(const std::string serve_addr, const std::vector<std::string> members_addr) {
    RaftServer::id_ = serve_addr;
    RaftServer::state_ = RaftState::Follower;
    for(auto& member_addr : members_addr) {
        if(member_addr != serve_addr) {
            RaftServer::clients_to_others_.emplace_back(std::make_unique<RaftClient>(member_addr));
        }
    }
}

const int RaftServer::currentTerm() const {
    std::ifstream term_file(this->id_+".term");
    std::string termstr;
    std::getline(term_file, termstr);
    term_file.close();
    if(termstr.empty()) {
        return 0;
    }
    int32_t term = std::stoi(termstr);
    return term;
}

void RaftServer::currentTerm(int32_t term) {
    std::ofstream term_file(this->id_+".term");
    std::string termstr = std::to_string(term);
    term_file.write(termstr.c_str(), termstr.size());
    term_file.close();
}

void RaftServer::voteFor(const std::string candidate_id) {
    std::ofstream votefor_file(this->id_+".votefor");
    votefor_file.write(candidate_id.c_str(), candidate_id.size());
    return;
}

const std::string RaftServer::voteFor() const {
    std::ifstream votefor_file(this->id_+".votefor");
    std::string candidate_id;
    std::getline(votefor_file, candidate_id);
    votefor_file.close();
    return candidate_id;
}

void RaftServer::start() {
    auto heartbeat = [this]()
    {
        if (this->state_ == RaftState::Leader)
        {
            AppendEntriesRequest request;
            request.set_term(this->currentTerm());
            request.set_leaderid(this->id_);
            for (auto& client : this->clients_to_others_)
            {
                auto [term, success] = client->AppendEntries(request);
                if (term > this->currentTerm())
                {
                    this->voteFor(client->remoteAddr());
                    this->currentTerm(term);
                    this->state_ = RaftState::Follower;
                    break;
                }
            }
        }
    };

    auto start_election = [this]()
    {
        if (this->state_ == RaftState::Leader) {
            return;
        }
        if (this->state_ == RaftState::Follower)
        {
            this->state_ = RaftState::Candidate;
        }
        this->voteFor("");
        this->currentTerm(this->currentTerm()+1);

        int32_t votes = 0;
        RequestVoteRequest request;
        request.set_term(this->currentTerm());
        request.set_candidateid(this->id_);
        for (auto& client : this->clients_to_others_)
        {
            auto [term, granted] = client->RequestVote(request);
            if (term > this->currentTerm())
            {
                this->voteFor(client->remoteAddr());
                this->state_ = RaftState::Follower;
                this->currentTerm(term);
            }
            else if (granted)
            {
                if (++votes > (this->clients_to_others_.size() +1) / 2)
                {
                    break;
                }
            }
        }
        if (votes >  (this->clients_to_others_.size() +1) / 2)
        {
            this->state_ = RaftState::Leader;
        }
    };

    election_timer_.Start(start_election);
    heartbeat_timer_.Start(heartbeat);
}

Status RaftServer::RequestVote(ServerContext *context, const RequestVoteRequest *request, RequestVoteReply *reply) {
    if (this->state_ == RaftState::Follower)
    {
        if (request->term() < this->currentTerm() || this->voteFor() != "")
        {
            reply->set_term(this->currentTerm());
            reply->set_votegranted(false);
            return Status::OK;
        }

        this->currentTerm(request->term());
        this->voteFor(request->candidateid());

        reply->set_term(request->term());
        reply->set_votegranted(true);

        this->election_timer_.Reset();

        return Status::OK;
    }
    else if (this->state_ == RaftState::Candidate)
    {
        if (request->term() > this->currentTerm())
        {

            this->voteFor(request->candidateid());
            this->currentTerm(request->term());
            this->state_ = RaftState::Follower;

            reply->set_term(request->term());
            reply->set_votegranted(true);

            this->election_timer_.Reset();
            return Status::OK;
        }

        reply->set_term(this->currentTerm());
        reply->set_votegranted(false);

        return Status::OK;
    }
    else
    {

        reply->set_term(this->currentTerm());
        reply->set_votegranted(false);

        return Status::OK;
    }
}
Status RaftServer::AppendEntries(ServerContext *context, const AppendEntriesRequest *request, AppendEntriesReply *reply) {
    if (request->entries_size() == 0)
    {
        if (this->state_ == RaftState::Follower || this->state_ == RaftState::Candidate)
        {
            if (request->term() < this->currentTerm())
            {
                reply->set_term(this->currentTerm());
                reply->set_success(false);
                return Status::OK;
            }

            this->voteFor(request->leaderid());
            this->currentTerm(request->term());
            this->state_ = RaftState::Follower;

            reply->set_term(request->term());
            reply->set_success(true);

            this->election_timer_.Reset();
            return Status::OK;
        }
    }

    reply->set_term(request->term());
    reply->set_success(false);
    return Status::OK;
}