#ifndef __RAFTCPP_RAFTSTORE_HPP__
#define __RAFTCPP_RAFTSTORE_HPP__

#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string_view>

enum class RaftLogType
{
    Normal,
    Configuration,
};

struct RaftLog
{
    int32_t index;
    int32_t term;
    std::string_view data;
};

class RaftStore
{
public:
    RaftStore(const std::string &id)
    {
        term_filepath = id + ".term";
        if (!std::filesystem::is_regular_file(term_filepath))
        {
            std::ofstream term_file = std::ofstream(term_filepath);
            term_file << "0";
            term_file.close();
        }
        votefor_filepath = id + ".votefor";
        if (!std::filesystem::is_regular_file(votefor_filepath))
        {
            std::ofstream votefor_file = std::ofstream(votefor_filepath);
            votefor_file.close();
        }
    }
    int32_t currentTerm() const
    {
        std::ifstream term_file = std::ifstream(term_filepath);
        std::string term_str;
        std::getline(term_file, term_str);
        term_file.close();
        return int32_t(std::stoi(term_str));
    }

    bool currentTerm(int32_t term)
    {
        std::ofstream term_file = std::ofstream(term_filepath);
        std::string term_str = std::to_string(term);
        term_file.write(term_str.c_str(), term_str.size());
        term_file.close();
        return true;
    }

    bool increaseTerm()
    {
        int32_t term = currentTerm();
        return currentTerm(term + 1);
    }
    const std::string voteFor() const
    {
        std::ifstream votefor_file = std::ifstream(votefor_filepath);
        std::string candidate;
        std::getline(votefor_file, candidate);
        votefor_file.close();
        return candidate;
    }

    bool voteFor(const std::string &candidate)
    {
        std::ofstream votefor_file = std::ofstream(votefor_filepath);
        votefor_file.write(candidate.c_str(), candidate.size());
        votefor_file.close();
        return true;
    }

private:
    std::string term_filepath;
    std::string votefor_filepath;
};

#endif __RAFTCPP_RAFTSTORE_HPP__