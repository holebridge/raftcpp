#include <string>
#include <vector>
#include <ranges>
#include <sstream>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "raft.hpp"

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
    std::stringstream ss(addr_list_str);
    std::string addr;
    while (std::getline(ss, addr, ','))
        member_addrlist.push_back(addr);

    ServerBuilder builder;
    builder.AddListeningPort(serve_addr,  grpc::InsecureServerCredentials());
    
    RaftServer srv;
    srv.init(serve_addr, member_addrlist);
    srv.start();
    
    builder.RegisterService(&srv);

    std::unique_ptr<Server> server(builder.BuildAndStart());

    server->Wait();

    return 0;
}