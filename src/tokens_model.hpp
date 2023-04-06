
#include "nlohmann/json.hpp"
#include <iostream>

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

typedef std::uint16_t TokenType;
typedef std::tuple<TokenType, TokenType> kmer;


ordered_json get_json(const std::map<TokenType, std::string>& tokens_str_map, const std::map<TokenType, kmer>& tokens) {
    
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

    for (auto const& [token_type, token_str] : tokens_str_map) {
        if (token_type < 11) {
            continue;
        }
        model["vocab"][token_str] = token_type;
        // std::cout << token_type << " " << token_str << std::endl;
        auto [left, right] = tokens.at(token_type);
        std::string left_str = tokens_str_map.at(left);
        std::string right_str = tokens_str_map.at(right);
        merges.push_back(left_str+" "+right_str);
    }
    model["merges"] = merges;
    config["model"] = model;
 
    return config;
}
