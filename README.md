
# dnaBPE: a tokenizer for DNA Sequences

This repository contains a C++ program that applies Byte Pair Encoding (BPE) to tokenize DNA sequences for various downstream applications, including transformer-based language models.

## Features

- Supports multiple input formats for DNA sequences: reads, fasta, trf.
- Efficient BPE computation using parallel processing.
- Outputs a JSON model in file containing BPE tokens and their corresponding DNA sub-sequences.
- JSON file is compatible with the HuggingFace Transformers library.
- Transforms the input DNA sequences using the computed BPE tokens.
- Ouput file with information about token frequencies and positions in the input sequences.

## Memory requirements

For the fast version (default or bpe.fast.exe):

The forward and reverse T2T human genomes takes about 200 GB of RAM once and then lower it during computation. Memory requirements are linear with the size of the input sequences.

For the slow version (bpe.lowmem.exe):

The forward and reverse T2T human genomes takes about 20 GB of RAM. Memory requirements are linear with the size of the input sequences.

## Requirements

- A C++ compiler with C++17 support.
- The nlohmann/json library (included in the repository).

## Installation

```sh
git clone https://github.com/aglabx/dnaBPE.git
cd dnaBPE
make
```

If you have a problem with the gcc version, you can try to install required version with conda and provided environment.yml file:

```sh
conda env create -f environment.yml
```

## Usage

Compile and run the program with the following command-line arguments:

```sh
./bin/bpe.exe <input_file> <output_file_prefix> <format: reads, fasta, trf> <max_tokens> <threads>
```

This command will generate a JSON model file with the BPE tokens and their corresponding DNA sub-sequences, as well as the transformed sequences using BPE. The output can be further used in various downstream applications, including transformer-based language models for DNA sequence analysis and prediction.

### Reads format

It is the simple one sequence per line format.

### Output files

- prefix.json - JSON file for hugging face transformers
- prefix.bpe - transformed sequences
- prefix.poses - token frequencies and positions in the input sequences. Tab-separated file with the following columns: token, frequency, space-separated positions in the sequence. Each position like sequd:pos, where sequd is the sequence position in the input file and pos and pos is the zero-base position in the sequence.

```txt
AAAC	62504	14:161 15:160 16:161 17:161 18:161 20:161 ...
AAACAGG	6679	105:775 106:774 158:774 159:774 160:774 ...
AAACAGGATTAGATACCCTGGTAGTCCAC	5346	82:777 273:789 274:789 ...
```

# Usage for HuggingFace Transformers

You can simply upload to HuggingFace and use it in your code.

For 16S tokens:

```python
from transformers import AutoTokenizer
tokenizer = AutoTokenizer.from_pretrained('aglabx/16S_1024_bpe_tokens', force_download=True, use_fast=True)

print(tokenizer.vocab_size)

tokenizer.tokenize(dna_data.upper())
```

### Some information about the haggling face special symbols:

```json
 {
    "id": 1,
    "content": "[CLS]",
    "single_word": false,
    "lstrip": false,
    "rstrip": false,
    "normalized": false,
    "special": true
},
```

- id: A unique identifier for the token.
- content: The actual content of the token. For example, 
  * [UNK] represents an unknown or out-of-vocabulary token, 
  * [CLS] represents the start of a sequence, 
  * [SEP] represents the separator between two sequences, 
  * [PAD] represents padding, and 
  * [MASK] represents a masked token that is used for pretraining or fine-tuning.
- single_word: A boolean indicating whether the token represents a single word or a sequence of words. For example, [UNK] and [PAD] are single words, while [CLS], [SEP], and [MASK] are not.
- lstrip: A boolean indicating whether the token needs to be left-stripped before use. For example, a token like "..." might need to be left-stripped to remove leading whitespace.
- rstrip: A boolean indicating whether the token needs to be right-stripped before use. For example, a token like "Mr." might need to be right-stripped to remove trailing punctuation.
- normalized: A boolean indicating whether the token has been normalized in some way, such as lowercasing or stemming.
- special: A boolean indicating whether the token is a special token that has a specific purpose in the model, as opposed to a regular word in the vocabulary.

I've added two other special symbols:

```json
    {"~", 5}, // for gaps unkwown length between sequences
    {"N", 6}, // for gaps length 1bp between sequences
```
