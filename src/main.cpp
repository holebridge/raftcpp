#include <ranges>
#include <string>
#include <vector>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "raft/raft_node.hpp"

using grpc::ServerBuilder;
using grpc::Server;

int main(int argc, char **argv)
{
    if(argc != 3) {
        std::abort();
    }
    std::string addr_list_str(argv[1]);
    std::string serve_addr(argv[2]);
    std::vector<std::string> member_addrlist;
    std::size_t start = 0;
    std::size_t end = addr_list_str.find_first_of(',');
    while(end !=  std::string::npos) {
        member_addrlist.emplace_back(addr_list_str.substr(start, end - start));
        start = end + 1;
        end =  addr_list_str.find_first_of(',',  start);
    }

    ServerBuilder builder;
    builder.AddListeningPort(serve_addr,  grpc::InsecureServerCredentials());
    
    RaftNode node(member_addrlist, serve_addr);
    builder.RegisterService(&node);

    std::unique_ptr<Server> server(builder.BuildAndStart());

    node.Start();
    server->Wait();

    return 0;
}