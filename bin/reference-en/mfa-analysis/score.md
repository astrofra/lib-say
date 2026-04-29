# MFA Duration Similarity Score

- Generated: `2026-04-29T09:10:01+02:00`
- Metric: DSS = mean( min(r, 1/r) ) per phoneme pair, r = ours/mary duration
- 100% = perfect timing match with MaryTTS reference

## Per-sample results

| Sample | Text | DSS | Pairs | LLH ours/s | LLH mary/s |
|---|---|---:|---:|---:|---:|
| `01-demo` | Hello from lib-say. This is an English demo sentence. | **73.7%** | 37 | -13.01 | -15.92 |
| `02-hamlet` | To be or not to be, that is the question. | **62.8%** | 25 | -22.82 | -21.07 |
| `03-sibilants` | She sells seashells by the seashore. | **74.9%** | 21 | -23.35 | -23.79 |
| `04-dentals` | This thing is worth the effort. | **58.7%** | 16 | -32.80 | -35.09 |
| `05-voiced-dentals` | Those feathers gather there. | **49.6%** | 15 | -28.66 | -32.67 |
| `06-affricates` | Judge the changing church bells. | **63.4%** | 18 | -25.38 | -34.15 |
| `07-stress` | English fricatives shift sharply. | **61.8%** | 17 | -21.57 | -26.00 |
| `08-finals` | Leave these clothes and bags. | **58.5%** | 16 | -21.18 | -34.88 |

**Global DSS: 62.9%**

## Per-sample phoneme pairs

### `01-demo` — Hello from lib-say. This is an English demo sentence.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| HH | 80 | 30 | 2.67 | 38% ⚠ | Other |
| AH | 40 | 30 | 1.33 | 75% | Vowels |
| L | 70 | 70 | 1.00 | 100% | Liquids |
| OW | 170 | 130 | 1.31 | 76% | Vowels |
| F | 40 | 100 | 0.40 | 40% ⚠ | Other |
| R | 70 | 40 | 1.75 | 57% | Liquids |
| AH | 80 | 40 | 2.00 | 50% | Vowels |
| M | 80 | 60 | 1.33 | 75% | Nasals |
| L | 30 | 70 | 0.43 | 43% ⚠ | Liquids |
| IH | 130 | 60 | 2.17 | 46% ⚠ | Vowels |
| B | 100 | 60 | 1.67 | 60% | Stops |
| S | 100 | 130 | 0.77 | 77% | Sibilants |
| EJ | 230 | 330 | 0.70 | 70% | Other |
| DH | 30 | 30 | 1.00 | 100% | Dentals |
| IH | 80 | 80 | 1.00 | 100% | Vowels |
| S | 120 | 90 | 1.33 | 75% | Sibilants |
| IH | 80 | 90 | 0.89 | 89% | Vowels |
| Z | 60 | 60 | 1.00 | 100% | Sibilants |
| AH | 50 | 60 | 0.83 | 83% | Vowels |
| N | 60 | 60 | 1.00 | 100% | Nasals |
| IH | 100 | 110 | 0.91 | 91% | Vowels |
| N | 80 | 90 | 0.89 | 89% | Nasals |
| JH | 90 | 30 | 3.00 | 33% ⚠ | Affricates |
| L | 70 | 60 | 1.17 | 86% | Liquids |
| IH | 120 | 60 | 2.00 | 50% | Vowels |
| SH | 110 | 100 | 1.10 | 91% | Sibilants |
| D | 70 | 50 | 1.40 | 71% | Stops |
| EH | 110 | 80 | 1.37 | 73% | Vowels |
| M | 80 | 70 | 1.14 | 87% | Nasals |
| OW | 120 | 150 | 0.80 | 80% | Vowels |
| S | 80 | 110 | 0.73 | 73% | Sibilants |
| EH | 110 | 90 | 1.22 | 82% | Vowels |
| N | 110 | 60 | 1.83 | 55% | Nasals |
| T | 80 | 60 | 1.33 | 75% | Stops |
| AH | 60 | 70 | 0.86 | 86% | Vowels |
| N | 80 | 110 | 0.73 | 73% | Nasals |
| S | 120 | 150 | 0.80 | 80% | Sibilants |
| **DSS** | | | | **73.7%** | |

### `02-hamlet` — To be or not to be, that is the question.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| Tʰ | 50 | 90 | 0.56 | 56% | Other |
| Ʉː | 60 | 60 | 1.00 | 100% | Other |
| Bʲ | 60 | 60 | 1.00 | 100% | Other |
| IY | 70 | 120 | 0.58 | 58% | Vowels |
| Ɒ | 90 | 70 | 1.29 | 78% | Other |
| R | 60 | 60 | 1.00 | 100% | Liquids |
| N | 140 | 70 | 2.00 | 50% | Nasals |
| AA | 140 | 140 | 1.00 | 100% | Vowels |
| T | 40 | 50 | 0.80 | 80% | Stops |
| Tʰ | 90 | 90 | 1.00 | 100% | Other |
| Ʉː | 70 | 60 | 1.17 | 86% | Other |
| Bʲ | 60 | 50 | 1.20 | 83% | Other |
| IY | 70 | 290 | 0.24 | 24% ⚠ | Vowels |
| AE | 150 | 40 | 3.75 | 27% ⚠ | Vowels |
| T | 70 | 90 | 0.78 | 78% | Stops |
| IH | 30 | 50 | 0.60 | 60% | Vowels |
| Z | 30 | 80 | 0.38 | 38% ⚠ | Sibilants |
| DH | 80 | 40 | 2.00 | 50% ⚠ | Dentals |
| AH | 140 | 60 | 2.33 | 43% ⚠ | Vowels |
| Cʷ | 30 | 40 | 0.75 | 75% | Other |
| EH | 10 | 110 | 0.09 | 9% ⚠ | Vowels |
| SH | 30 | 110 | 0.27 | 27% ⚠ | Sibilants |
| CH | 50 | 90 | 0.56 | 56% | Affricates |
| AH | 40 | 130 | 0.31 | 31% ⚠ | Vowels |
| N | 110 | 70 | 1.57 | 64% | Nasals |
| **DSS** | | | | **62.8%** | |

### `03-sibilants` — She sells seashells by the seashore.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| SH | 110 | 140 | 0.79 | 79% | Sibilants |
| IY | 40 | 80 | 0.50 | 50% | Vowels |
| S | 100 | 120 | 0.83 | 83% | Sibilants |
| EH | 90 | 70 | 1.29 | 78% | Vowels |
| Ɫ | 100 | 90 | 1.11 | 90% | Other |
| Z | 40 | 40 | 1.00 | 100% | Sibilants |
| S | 190 | 90 | 2.11 | 47% ⚠ | Sibilants |
| IY | 120 | 120 | 1.00 | 100% | Vowels |
| SH | 130 | 100 | 1.30 | 77% | Sibilants |
| EH | 70 | 80 | 0.87 | 87% | Vowels |
| Ɫ | 90 | 80 | 1.12 | 89% | Other |
| Z | 110 | 60 | 1.83 | 55% | Sibilants |
| B | 80 | 100 | 0.80 | 80% | Stops |
| AY | 170 | 130 | 1.31 | 76% | Vowels |
| DH | 30 | 40 | 0.75 | 75% | Dentals |
| AH | 30 | 50 | 0.60 | 60% | Vowels |
| S | 120 | 110 | 1.09 | 92% | Sibilants |
| IY | 150 | 120 | 1.25 | 80% | Vowels |
| SH | 120 | 110 | 1.09 | 92% | Sibilants |
| Ɒ | 70 | 110 | 0.64 | 64% | Other |
| R | 40 | 210 | 0.19 | 19% ⚠ | Liquids |
| **DSS** | | | | **74.9%** | |

### `04-dentals` — This thing is worth the effort.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| DH | 30 | 50 | 0.60 | 60% | Dentals |
| IH | 50 | 80 | 0.62 | 62% | Vowels |
| S | 30 | 80 | 0.38 | 38% ⚠ | Sibilants |
| T̪ | 240 | 100 | 2.40 | 42% ⚠ | Other |
| IH | 30 | 70 | 0.43 | 43% ⚠ | Vowels |
| N | 210 | 90 | 2.33 | 43% ⚠ | Nasals |
| Z | 70 | 80 | 0.87 | 87% | Sibilants |
| W | 70 | 60 | 1.17 | 86% | Other |
| ɝ | 80 | 60 | 1.33 | 75% | Other |
| TH | 160 | 130 | 1.23 | 81% | Dentals |
| DH | 70 | 30 | 2.33 | 43% ⚠ | Dentals |
| AH | 130 | 40 | 3.25 | 31% ⚠ | Vowels |
| EH | 40 | 60 | 0.67 | 67% | Vowels |
| F | 120 | 150 | 0.80 | 80% | Other |
| ER | 30 | 110 | 0.27 | 27% ⚠ | Vowels |
| T | 30 | 40 | 0.75 | 75% | Stops |
| **DSS** | | | | **58.7%** | |

### `05-voiced-dentals` — Those feathers gather there.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| DH | 110 | 60 | 1.83 | 55% | Dentals |
| OW | 170 | 120 | 1.42 | 71% | Vowels |
| Z | 30 | 80 | 0.37 | 37% ⚠ | Sibilants |
| F | 150 | 110 | 1.36 | 73% | Other |
| EH | 100 | 110 | 0.91 | 91% | Vowels |
| DH | 140 | 50 | 2.80 | 36% ⚠ | Dentals |
| ER | 30 | 100 | 0.30 | 30% ⚠ | Vowels |
| Z | 140 | 90 | 1.56 | 64% | Sibilants |
| G | 30 | 70 | 0.43 | 43% ⚠ | Stops |
| AE | 30 | 120 | 0.25 | 25% ⚠ | Vowels |
| DH | 30 | 50 | 0.60 | 60% | Dentals |
| ER | 110 | 80 | 1.38 | 73% | Vowels |
| DH | 330 | 40 | 8.25 | 12% ⚠ | Dentals |
| EH | 80 | 140 | 0.57 | 57% | Vowels |
| R | 30 | 170 | 0.18 | 18% ⚠ | Liquids |
| **DSS** | | | | **49.6%** | |

### `06-affricates` — Judge the changing church bells.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| JH | 50 | 80 | 0.63 | 63% | Affricates |
| AH | 70 | 100 | 0.70 | 70% | Vowels |
| JH | 30 | 80 | 0.38 | 38% ⚠ | Affricates |
| DH | 170 | 80 | 2.12 | 47% ⚠ | Dentals |
| AH | 30 | 60 | 0.50 | 50% | Vowels |
| CH | 150 | 130 | 1.15 | 87% | Affricates |
| EJ | 40 | 120 | 0.33 | 33% ⚠ | Other |
| N | 140 | 80 | 1.75 | 57% | Nasals |
| JH | 30 | 60 | 0.50 | 50% ⚠ | Affricates |
| IH | 30 | 70 | 0.43 | 43% ⚠ | Vowels |
| NG | 30 | 80 | 0.38 | 38% ⚠ | Nasals |
| CH | 120 | 120 | 1.00 | 100% | Affricates |
| ɝ | 120 | 110 | 1.09 | 92% | Other |
| CH | 170 | 130 | 1.31 | 76% | Affricates |
| B | 40 | 40 | 1.00 | 100% | Stops |
| EH | 120 | 100 | 1.20 | 83% | Vowels |
| Ɫ | 60 | 160 | 0.38 | 38% ⚠ | Other |
| Z | 140 | 180 | 0.78 | 78% | Sibilants |
| **DSS** | | | | **63.4%** | |

### `07-stress` — English fricatives shift sharply.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| IH | 30 | 70 | 0.43 | 43% ⚠ | Vowels |
| N | 50 | 150 | 0.33 | 33% ⚠ | Nasals |
| JH | 30 | 30 | 1.00 | 100% | Affricates |
| L | 30 | 50 | 0.60 | 60% | Liquids |
| IH | 30 | 60 | 0.50 | 50% | Vowels |
| SH | 30 | 40 | 0.75 | 75% | Sibilants |
| SPN | 1410 | 600 | 2.35 | 43% ⚠ | Other |
| SH | 30 | 130 | 0.23 | 23% ⚠ | Sibilants |
| IH | 30 | 60 | 0.50 | 50% | Vowels |
| F | 30 | 50 | 0.60 | 60% | Other |
| T | 60 | 80 | 0.75 | 75% | Stops |
| SH | 110 | 130 | 0.85 | 85% | Sibilants |
| AA | 100 | 60 | 1.67 | 60% | Vowels |
| R | 70 | 90 | 0.78 | 78% | Liquids |
| Pʲ | 130 | 90 | 1.44 | 69% | Other |
| L | 70 | 60 | 1.17 | 86% | Liquids |
| IY | 110 | 180 | 0.61 | 61% | Vowels |
| **DSS** | | | | **61.8%** | |

### `08-finals` — Leave these clothes and bags.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| L | 150 | 70 | 2.14 | 47% ⚠ | Liquids |
| IY | 30 | 80 | 0.38 | 38% ⚠ | Vowels |
| V | 180 | 60 | 3.00 | 33% ⚠ | VoicedFric |
| DH | 30 | 40 | 0.75 | 75% | Dentals |
| IY | 80 | 130 | 0.62 | 62% | Vowels |
| Z | 140 | 70 | 2.00 | 50% | Sibilants |
| K | 30 | 110 | 0.27 | 27% ⚠ | Stops |
| L | 60 | 30 | 2.00 | 50% ⚠ | Liquids |
| OW | 210 | 150 | 1.40 | 71% | Vowels |
| DH | 170 | 40 | 4.25 | 24% ⚠ | Dentals |
| Z | 70 | 90 | 0.78 | 78% | Sibilants |
| AE | 100 | 80 | 1.25 | 80% | Vowels |
| N | 90 | 90 | 1.00 | 100% | Nasals |
| T | 90 | 210 | 0.43 | 43% ⚠ | Stops |
| B | 80 | 90 | 0.89 | 89% | Stops |
| AE | 150 | 215 | 0.70 | 70% | Vowels |
| **DSS** | | | | **58.5%** | |

## Family summary

| Family | N | Mean sim | Mean ratio |
|---|---:|---:|---:|
| Vowels | 53 | 61.7% | 1.01 |
| Sibilants | 26 | 69.2% | 0.96 |
| Stops | 13 | 68.9% | 0.87 |
| Nasals | 13 | 66.4% | 1.25 |
| Liquids | 12 | 61.9% | 1.03 |
| Dentals | 13 | 55.2% | 2.19 |
| VoicedFric | 1 | 33.3% | 3.00 |
| Affricates | 9 | 66.9% | 1.06 |
| Other | 25 | 70.3% | 1.11 |

## Improvement leads (sim < 60%)

| ARPA | Sample | ours (ms) | mary (ms) | ratio | sim | family |
|---|---|---:|---:|---:|---:|---|
| EH | `02-hamlet` | 10 | 110 | 0.09 | 9% | Vowels |
| | | | | | → too short | |
| DH | `05-voiced-dentals` | 330 | 40 | 8.25 | 12% | Dentals |
| | | | | | → too long | |
| R | `05-voiced-dentals` | 30 | 170 | 0.18 | 18% | Liquids |
| | | | | | → too short | |
| R | `03-sibilants` | 40 | 210 | 0.19 | 19% | Liquids |
| | | | | | → too short | |
| SH | `07-stress` | 30 | 130 | 0.23 | 23% | Sibilants |
| | | | | | → too short | |
| DH | `08-finals` | 170 | 40 | 4.25 | 24% | Dentals |
| | | | | | → too long | |
| IY | `02-hamlet` | 70 | 290 | 0.24 | 24% | Vowels |
| | | | | | → too short | |
| AE | `05-voiced-dentals` | 30 | 120 | 0.25 | 25% | Vowels |
| | | | | | → too short | |
| AE | `02-hamlet` | 150 | 40 | 3.75 | 27% | Vowels |
| | | | | | → too long | |
| SH | `02-hamlet` | 30 | 110 | 0.27 | 27% | Sibilants |
| | | | | | → too short | |
| K | `08-finals` | 30 | 110 | 0.27 | 27% | Stops |
| | | | | | → too short | |
| ER | `04-dentals` | 30 | 110 | 0.27 | 27% | Vowels |
| | | | | | → too short | |
| ER | `05-voiced-dentals` | 30 | 100 | 0.30 | 30% | Vowels |
| | | | | | → too short | |
| AH | `04-dentals` | 130 | 40 | 3.25 | 31% | Vowels |
| | | | | | → too long | |
| AH | `02-hamlet` | 40 | 130 | 0.31 | 31% | Vowels |
| | | | | | → too short | |
| N | `07-stress` | 50 | 150 | 0.33 | 33% | Nasals |
| | | | | | → too short | |
| V | `08-finals` | 180 | 60 | 3.00 | 33% | VoicedFric |
| | | | | | → too long | |
| EJ | `06-affricates` | 40 | 120 | 0.33 | 33% | Other |
| | | | | | → too short | |
| JH | `01-demo` | 90 | 30 | 3.00 | 33% | Affricates |
| | | | | | → too long | |
| DH | `05-voiced-dentals` | 140 | 50 | 2.80 | 36% | Dentals |
| | | | | | → too long | |
| Z | `05-voiced-dentals` | 30 | 80 | 0.37 | 37% | Sibilants |
| | | | | | → too short | |
| HH | `01-demo` | 80 | 30 | 2.67 | 38% | Other |
| | | | | | → too long | |
| Z | `02-hamlet` | 30 | 80 | 0.38 | 38% | Sibilants |
| | | | | | → too short | |
| S | `04-dentals` | 30 | 80 | 0.38 | 38% | Sibilants |
| | | | | | → too short | |
| IY | `08-finals` | 30 | 80 | 0.38 | 38% | Vowels |
| | | | | | → too short | |
| JH | `06-affricates` | 30 | 80 | 0.38 | 38% | Affricates |
| | | | | | → too short | |
| NG | `06-affricates` | 30 | 80 | 0.38 | 38% | Nasals |
| | | | | | → too short | |
| Ɫ | `06-affricates` | 60 | 160 | 0.38 | 38% | Other |
| | | | | | → too short | |
| F | `01-demo` | 40 | 100 | 0.40 | 40% | Other |
| | | | | | → too short | |
| T̪ | `04-dentals` | 240 | 100 | 2.40 | 42% | Other |
| | | | | | → too long | |
| SPN | `07-stress` | 1410 | 600 | 2.35 | 43% | Other |
| | | | | | → too long | |
| DH | `04-dentals` | 70 | 30 | 2.33 | 43% | Dentals |
| | | | | | → too long | |
| G | `05-voiced-dentals` | 30 | 70 | 0.43 | 43% | Stops |
| | | | | | → too short | |
| N | `04-dentals` | 210 | 90 | 2.33 | 43% | Nasals |
| | | | | | → too long | |
| IH | `07-stress` | 30 | 70 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| T | `08-finals` | 90 | 210 | 0.43 | 43% | Stops |
| | | | | | → too short | |
| AH | `02-hamlet` | 140 | 60 | 2.33 | 43% | Vowels |
| | | | | | → too long | |
| IH | `04-dentals` | 30 | 70 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| L | `01-demo` | 30 | 70 | 0.43 | 43% | Liquids |
| | | | | | → too short | |
| IH | `06-affricates` | 30 | 70 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| IH | `01-demo` | 130 | 60 | 2.17 | 46% | Vowels |
| | | | | | → too long | |
| L | `08-finals` | 150 | 70 | 2.14 | 47% | Liquids |
| | | | | | → too long | |
| DH | `06-affricates` | 170 | 80 | 2.12 | 47% | Dentals |
| | | | | | → too long | |
| S | `03-sibilants` | 190 | 90 | 2.11 | 47% | Sibilants |
| | | | | | → too long | |
| JH | `06-affricates` | 30 | 60 | 0.50 | 50% | Affricates |
| | | | | | → too short | |
| L | `08-finals` | 60 | 30 | 2.00 | 50% | Liquids |
| | | | | | → too long | |
| DH | `02-hamlet` | 80 | 40 | 2.00 | 50% | Dentals |
| | | | | | → too long | |
| AH | `01-demo` | 80 | 40 | 2.00 | 50% | Vowels |
| | | | | | → too long | |
| IH | `01-demo` | 120 | 60 | 2.00 | 50% | Vowels |
| | | | | | → too long | |
| N | `02-hamlet` | 140 | 70 | 2.00 | 50% | Nasals |
| | | | | | → too long | |
| IY | `03-sibilants` | 40 | 80 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| IH | `07-stress` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| IH | `07-stress` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| Z | `08-finals` | 140 | 70 | 2.00 | 50% | Sibilants |
| | | | | | → too long | |
| AH | `06-affricates` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| N | `01-demo` | 110 | 60 | 1.83 | 55% | Nasals |
| | | | | | → too long | |
| Z | `03-sibilants` | 110 | 60 | 1.83 | 55% | Sibilants |
| | | | | | → too long | |
| DH | `05-voiced-dentals` | 110 | 60 | 1.83 | 55% | Dentals |
| | | | | | → too long | |
| Tʰ | `02-hamlet` | 50 | 90 | 0.56 | 56% | Other |
| | | | | | → too short | |
| CH | `02-hamlet` | 50 | 90 | 0.56 | 56% | Affricates |
| | | | | | → too short | |
| N | `06-affricates` | 140 | 80 | 1.75 | 57% | Nasals |
| | | | | | → too long | |
| R | `01-demo` | 70 | 40 | 1.75 | 57% | Liquids |
| | | | | | → too long | |
| EH | `05-voiced-dentals` | 80 | 140 | 0.57 | 57% | Vowels |
| | | | | | → too short | |
| IY | `02-hamlet` | 70 | 120 | 0.58 | 58% | Vowels |
| | | | | | → too short | |
| B | `01-demo` | 100 | 60 | 1.67 | 60% | Stops |
| | | | | | → too long | |
