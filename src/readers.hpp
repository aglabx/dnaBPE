#ifndef READERS_FILE_H
#define READERS_FILE_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>


// reader for BPE tokenized files
void get_sequences_bpe(const std::string& bpe_file_name, const std::string& pos_file_name, std::vector<std::string>& seqs) {
  std::ifstream bpe_file(bpe_file_name);
  std::ifstream pos_file(pos_file_name);
  if (!bpe_file.is_open()) {
    std::cerr << "Error: Could not open file " << bpe_file_name << std::endl;
    return;
  }
  if (!pos_file.is_open()) {
    std::cerr << "Error: Could not open file " << pos_file_name << std::endl;
    return;
  }
  // reads index kmerid (first columin) to kmer (second column) from tab delimited bpe_file
  std::string line;
  std::string seq;
  while (std::getline(bpe_file, line)) {
    size_t kmerid = std::stoi(line.substr(0, line.find('\t')));
    std::string kmer = line.substr(line.find('\t') + 1);
  }
  // read bpe file where space delimited tokens are replaced by their index
  while (std::getline(pos_file, line)) {
    std::string seq;
    size_t pos = 0;
    while ((pos = line.find(' ')) != std::string::npos) {
      seq += line.substr(0, pos);
      line.erase(0, pos + 1);
    }
    seqs.push_back(seq);
  }
}

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

void get_sequences_fastq(const std::string& file_name, std::vector<std::string>& seqs) {
  std::ifstream file(file_name);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open file " << file_name << std::endl;
    return;
  }
  std::string line;
  std::string seq;
  int i = 0;
  while (std::getline(file, line)) {
    if (i % 4 == 1) {
      seq += line;
    } else if (i % 4 == 3) {
      seqs.push_back(seq);
      seq.clear();
    }
    ++i;
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