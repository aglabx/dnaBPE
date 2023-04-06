
# Usage

For GENA_LM tokens:

```python
from transformers import AutoTokenizer
tokenizer = AutoTokenizer.from_pretrained('aglabx/dna_tokens', force_download=True, use_fast=True)

print(tokenizer.vocab_size)

tokenizer.tokenize(dna_data.upper())
```

For 16S tokens:

```python
from transformers import AutoTokenizer
tokenizer = AutoTokenizer.from_pretrained('aglabx/16S_1024_bpe_tokens', force_download=True, use_fast=True)

print(tokenizer.vocab_size)

tokenizer.tokenize(dna_data.upper())
```


```
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

```
id: A unique identifier for the token.
content: The actual content of the token. For example, [UNK] represents an unknown or out-of-vocabulary token, [CLS] represents the start of a sequence, [SEP] represents the separator between two sequences, [PAD] represents padding, and [MASK] represents a masked token that is used for pretraining or fine-tuning.
single_word: A boolean indicating whether the token represents a single word or a sequence of words. For example, [UNK] and [PAD] are single words, while [CLS], [SEP], and [MASK] are not.
lstrip: A boolean indicating whether the token needs to be left-stripped before use. For example, a token like "..." might need to be left-stripped to remove leading whitespace.
rstrip: A boolean indicating whether the token needs to be right-stripped before use. For example, a token like "Mr." might need to be right-stripped to remove trailing punctuation.
normalized: A boolean indicating whether the token has been normalized in some way, such as lowercasing or stemming.
special: A boolean indicating whether the token is a special token that has a specific purpose in the model, as opposed to a regular word in the vocabulary.
{"[UNK]": 0}, // represents an unknown or out-of-vocabulary token
{"[CLS]": 1}, // represents the start of a sequence
{"[SEP]": 2}, // represents the separator between two sequences
{"[PAD]": 3}, // represents padding
{"[MASK]": 4}, // represents a masked token that is used for pretraining or fine-tuning
{"~", 5}, // for gaps unkwown length between sequences
{"N", 6}, // for gaps length 1bp between sequences
```
