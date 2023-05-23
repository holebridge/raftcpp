#include <ranges>
#include <string>
#include <vector>
#include <string>
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
    std::size_t start = 0;
    std::size_t end = addr_list_str.find_first_of(',');
    while(end !=  std::string::npos) {
        member_addrlist.emplace_back(addr_list_str.substr(start, end - start));
        start = end + 1;
        end =  addr_list_str.find_first_of(',',  start);
    }

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