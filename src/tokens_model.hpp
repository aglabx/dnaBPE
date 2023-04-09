
#ifndef TOKENS_MODELS_FILE_H
#define TOKENS_MODELS_FILE_H

#include "../nlohmann/json.hpp"
#include <iostream>
#include "tokens.hpp"

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;


ordered_json get_json(const std::unordered_map<TokenType, std::string>& alphabet_map, const std::unordered_map<TokenType, size_t>& tokens, std::vector<Kmer>& merged_tokens,  std::unordered_map<Kmer, size_t, TupleHash> kmer2kmer_id, std::unordered_map<size_t, TokenType> rev_tokens) {
    
    // Create a json object and populate it with the data
    ordered_json config;

    config["version"] = "1.0";
    config["truncation"] = nullptr;
    config["padding"] = nullptr;

    ordered_json addedTokens = R"([
        {
        "id": 0,
        "content": "[UNK]",
        "single_word": false,
        "lstrip": false,
        "rstrip": false,
        "normalized": false,
        "special": true
        },
        {
        "id": 1,
        "content": "[CLS]",
        "single_word": false,
        "lstrip": false,
        "rstrip": false,
        "normalized": false,
        "special": true
        },
        {
        "id": 2,
        "content": "[SEP]",
        "single_word": false,
        "lstrip": false,
        "rstrip": false,
        "normalized": false,
        "special": true
        },
        {
        "id": 3,
        "content": "[PAD]",
        "single_word": false,
        "lstrip": false,
        "rstrip": false,
        "normalized": false,
        "special": true
        },
        {
        "id": 4,
        "content": "[MASK]",
        "single_word": false,
        "lstrip": false,
        "rstrip": false,
        "normalized": false,
        "special": true
        },
        {
        "id": 5,
        "content": "~",
        "single_word": false,
        "lstrip": false,
        "rstrip": false,
        "normalized": false,
        "special": true
        }
    ])"_json;
    

    config["added_tokens"] = addedTokens;

    config["normalizer"] = R"({
        "type": "Sequence",
        "normalizers": [
        {
            "type": "Strip",
            "strip_left": true,
            "strip_right": true
        }
        ]
    })"_json;

    config["pre_tokenizer"] = R"([{
        "type": "Split",
        "pattern": {
        "String": "~"
        },
        "behavior": "Isolated",
        "invert": false
    },
    {"type": "Split",
        "pattern": {
        "String": "\n"
        },
        "behavior": "Isolated",
        "invert": false
    }])"_json;

    config["post_processor"] = R"({
        "type": "BertProcessing",
        "sep": [
        "[SEP]",
        2
        ],
        "cls": [
        "[CLS]",
        1
        ]
    })"_json;

    config["decoder"] = R"({
        "type": "BPEDecoder",
        "suffix": " "
    })"_json;

    ordered_json model = {
        {"type", "BPE"},
        {"dropout", nullptr},
        {"unk_token", "[UNK]"},
        {"continuing_subword_prefix", nullptr},
        {"end_of_word_suffix", nullptr},
        {"fuse_unk", false}
    };
    ordered_json alphabet_ = {
            {"[UNK]", 0},
            {"[CLS]", 1},
            {"[SEP]", 2},
            {"[PAD]", 3},
            {"[MASK]", 4},
            {"~", 5},
            {"N", 6},
            {"A", 7},
            {"C", 8},
            {"G", 9},
            {"T", 10}
    };
    
    
    model["vocab"] = alphabet_;

    ordered_json merges = nlohmann::json::array();

    for (const Kmer& kmer_ : merged_tokens) {
        size_t kmer_id = kmer2kmer_id.at(kmer_);
        TokenType token_type = rev_tokens.at(kmer_id);
        std::string token_str = alphabet_map.at(token_type);
        if (token_type < 11) {
            continue;
        }
        model["vocab"][token_str] = token_type;
        // std::cout << token_type << " " << token_str << std::endl;
        auto [left, right] = kmer_;
        std::string left_str = alphabet_map.at(left);
        std::string right_str = alphabet_map.at(right);
        merges.push_back(left_str+" "+right_str);
    }
    model["merges"] = merges;
    config["model"] = model;
 
    return config;
}

#endif
