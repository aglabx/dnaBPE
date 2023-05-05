#ifndef BPE_FILE_FILE_H
#define BPE_FILE_FILE_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <map>

struct Data {
    size_t kmerid;
    std::string kmer;
    std::string merge;
    int tf1;
    int tf2;
    std::vector<std::pair<int, int>> cid_pos;
};

vod reader(const std::string& bpe_file, const std::string& pos_file, const std::vector<TokenType>& seq, std::unordered_map<Kmer, size_t, TupleHash>& kmer2kmer_id, std::unordered_map<size_t, Kmer>& kmer_id2kmer) {

        // read pos file (kmer_id, kmer, merge pair, _, tf, poses)
        std::ifstream pos_stream(pos_file);
        std::string line;
        std::getline(pos_stream, line); // skip header
        

    }


#endif