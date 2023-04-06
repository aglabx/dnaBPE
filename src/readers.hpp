#ifndef READERS_FILE_H
#define READERS_FILE_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>

void get_sequences_fasta(const std::string& file_name, std::vector<std::string>& seqs) {
  std::ifstream file(file_name);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open file " << file_name << std::endl;
    return;
  }

  std::string line;
  std::string seq;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '>') {
      if (!seq.empty()) {
        seqs.push_back(seq);
        seq.clear();
      }
    } else {
      seq += line;
    }
  }

  if (!seq.empty()) {
    std::transform(seq.begin(), seq.end(), seq.begin(),
               [](unsigned char c){ return std::toupper(c); });
    seqs.push_back(seq);
  }

  file.close();
}

void get_sequences_trf(const std::string& trf_file_name, std::vector<std::string>& seqs) {
    std::ifstream fh(trf_file_name);
    if (fh.is_open()) {
        std::string line;
        while (std::getline(fh, line)) {
            std::vector<std::string> data;
            size_t pos = 0;
            while ((pos = line.find('\t')) != std::string::npos) {
                data.push_back(line.substr(0, pos));
                line.erase(0, pos + 1);
            }
            seqs.push_back(data[14]);
        }
        fh.close();
    }
}

void get_sequences_reads(const std::string& reads_file_name, std::vector<std::string>& seqs) {
    std::ifstream fh(reads_file_name);
    if (fh.is_open()) {
        std::string line;
        while (std::getline(fh, line)) {
            std::transform(line.begin(), line.end(), line.begin(),
               [](unsigned char c){ return std::toupper(c); });
            seqs.push_back(line);
        }
        fh.close();
    }
}

#endif