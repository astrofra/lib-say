# MFA Duration Similarity Score

- Generated: `2026-04-29T15:32:45+02:00`
- Metric: DSS = mean( min(r, 1/r) ) per phoneme pair, r = ours/mary duration
- 100% = perfect timing match with MaryTTS reference

## Per-sample results

| Sample | Text | DSS | Pairs | LLH ours/s | LLH mary/s |
|---|---|---:|---:|---:|---:|
| `01-demo` | Hello from lib-say. This is an English demo sentence. | **72.2%** | 37 | -13.27 | -15.83 |
| `02-hamlet` | To be or not to be, that is the question. | **61.7%** | 25 | -22.83 | -20.84 |
| `03-sibilants` | She sells seashells by the seashore. | **77.8%** | 21 | -24.63 | -24.56 |
| `04-dentals` | This thing is worth the effort. | **58.1%** | 16 | -34.84 | -34.68 |
| `05-voiced-dentals` | Those feathers gather there. | **52.2%** | 15 | -29.42 | -28.36 |
| `06-affricates` | Judge the changing church bells. | **66.6%** | 18 | -25.82 | -33.82 |
| `07-stress` | English fricatives shift sharply. | **56.1%** | 17 | -21.51 | -25.88 |
| `08-finals` | Leave these clothes and bags. | **59.7%** | 17 | -22.45 | -35.79 |

**Global DSS: 63.1%**

## Per-sample phoneme pairs

### `01-demo` — Hello from lib-say. This is an English demo sentence.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| HH | 80 | 50 | 1.60 | 62% | Other |
| AH | 30 | 40 | 0.75 | 75% | Vowels |
| L | 80 | 60 | 1.33 | 75% | Liquids |
| OW | 150 | 130 | 1.15 | 87% | Vowels |
| F | 50 | 100 | 0.50 | 50% | Other |
| R | 40 | 40 | 1.00 | 100% | Liquids |
| AH | 110 | 40 | 2.75 | 36% ⚠ | Vowels |
| M | 90 | 80 | 1.13 | 89% | Nasals |
| L | 30 | 50 | 0.60 | 60% | Liquids |
| IH | 110 | 60 | 1.83 | 55% | Vowels |
| B | 100 | 60 | 1.67 | 60% | Stops |
| S | 100 | 130 | 0.77 | 77% | Sibilants |
| EJ | 220 | 330 | 0.67 | 67% | Other |
| DH | 30 | 30 | 1.00 | 100% | Dentals |
| IH | 70 | 80 | 0.87 | 87% | Vowels |
| S | 120 | 90 | 1.33 | 75% | Sibilants |
| IH | 40 | 90 | 0.44 | 44% ⚠ | Vowels |
| Z | 120 | 60 | 2.00 | 50% | Sibilants |
| AH | 50 | 60 | 0.83 | 83% | Vowels |
| N | 60 | 70 | 0.86 | 86% | Nasals |
| IH | 100 | 100 | 1.00 | 100% | Vowels |
| N | 70 | 90 | 0.78 | 78% | Nasals |
| JH | 90 | 30 | 3.00 | 33% ⚠ | Affricates |
| L | 70 | 60 | 1.17 | 86% | Liquids |
| IH | 120 | 60 | 2.00 | 50% | Vowels |
| SH | 120 | 60 | 2.00 | 50% ⚠ | Sibilants |
| D | 60 | 90 | 0.67 | 67% | Stops |
| EH | 90 | 80 | 1.12 | 89% | Vowels |
| M | 90 | 70 | 1.29 | 78% | Nasals |
| OW | 120 | 150 | 0.80 | 80% | Vowels |
| S | 80 | 110 | 0.73 | 73% | Sibilants |
| EH | 90 | 90 | 1.00 | 100% | Vowels |
| N | 100 | 70 | 1.43 | 70% | Nasals |
| T | 80 | 50 | 1.60 | 63% | Stops |
| AH | 60 | 70 | 0.86 | 86% | Vowels |
| N | 80 | 90 | 0.89 | 89% | Nasals |
| S | 110 | 170 | 0.65 | 65% | Sibilants |
| **DSS** | | | | **72.2%** | |

### `02-hamlet` — To be or not to be, that is the question.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| Tʰ | 50 | 90 | 0.56 | 56% | Other |
| Ʉː | 70 | 60 | 1.17 | 86% | Other |
| Bʲ | 60 | 60 | 1.00 | 100% | Other |
| IY | 70 | 120 | 0.58 | 58% | Vowels |
| Ɒ | 80 | 70 | 1.14 | 87% | Other |
| R | 60 | 60 | 1.00 | 100% | Liquids |
| N | 140 | 70 | 2.00 | 50% | Nasals |
| AA | 120 | 140 | 0.86 | 86% | Vowels |
| T | 40 | 50 | 0.80 | 80% | Stops |
| Tʰ | 100 | 90 | 1.11 | 90% | Other |
| Ʉː | 60 | 60 | 1.00 | 100% | Other |
| Bʲ | 70 | 50 | 1.40 | 71% | Other |
| IY | 90 | 290 | 0.31 | 31% ⚠ | Vowels |
| AE | 140 | 40 | 3.50 | 29% ⚠ | Vowels |
| T | 80 | 100 | 0.80 | 80% | Stops |
| IH | 30 | 40 | 0.75 | 75% | Vowels |
| Z | 30 | 80 | 0.38 | 38% ⚠ | Sibilants |
| DH | 120 | 40 | 3.00 | 33% ⚠ | Dentals |
| AH | 110 | 60 | 1.83 | 55% | Vowels |
| Cʷ | 30 | 40 | 0.75 | 75% | Other |
| EH | 10 | 110 | 0.09 | 9% ⚠ | Vowels |
| S | 30 | 110 | 0.27 | 27% ⚠ | Sibilants |
| CH | 40 | 90 | 0.44 | 44% ⚠ | Affricates |
| AH | 30 | 130 | 0.23 | 23% ⚠ | Vowels |
| N | 120 | 70 | 1.71 | 58% | Nasals |
| **DSS** | | | | **61.7%** | |

### `03-sibilants` — She sells seashells by the seashore.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| SH | 100 | 140 | 0.71 | 71% | Sibilants |
| IY | 40 | 80 | 0.50 | 50% | Vowels |
| S | 100 | 110 | 0.91 | 91% | Sibilants |
| EH | 80 | 80 | 1.00 | 100% | Vowels |
| Ɫ | 90 | 90 | 1.00 | 100% | Other |
| Z | 40 | 40 | 1.00 | 100% | Sibilants |
| S | 180 | 100 | 1.80 | 56% | Sibilants |
| IY | 110 | 110 | 1.00 | 100% | Vowels |
| SH | 120 | 100 | 1.20 | 83% | Sibilants |
| EH | 70 | 80 | 0.87 | 87% | Vowels |
| Ɫ | 90 | 80 | 1.13 | 89% | Other |
| Z | 140 | 60 | 2.33 | 43% ⚠ | Sibilants |
| B | 50 | 100 | 0.50 | 50% | Stops |
| AY | 180 | 130 | 1.38 | 72% | Vowels |
| DH | 30 | 40 | 0.75 | 75% | Dentals |
| AH | 30 | 50 | 0.60 | 60% | Vowels |
| S | 110 | 110 | 1.00 | 100% | Sibilants |
| IY | 130 | 120 | 1.08 | 92% | Vowels |
| SH | 110 | 110 | 1.00 | 100% | Sibilants |
| Ɒ | 70 | 110 | 0.64 | 64% | Other |
| R | 110 | 220 | 0.50 | 50% ⚠ | Liquids |
| **DSS** | | | | **77.8%** | |

### `04-dentals` — This thing is worth the effort.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| DH | 30 | 50 | 0.60 | 60% | Dentals |
| IH | 40 | 80 | 0.50 | 50% | Vowels |
| S | 30 | 80 | 0.38 | 38% ⚠ | Sibilants |
| T̪ | 230 | 100 | 2.30 | 43% ⚠ | Other |
| IH | 30 | 70 | 0.43 | 43% ⚠ | Vowels |
| N | 210 | 90 | 2.33 | 43% ⚠ | Nasals |
| Z | 70 | 80 | 0.87 | 87% | Sibilants |
| W | 70 | 60 | 1.17 | 86% | Other |
| ɝ | 80 | 50 | 1.60 | 63% | Other |
| TH | 150 | 140 | 1.07 | 93% | Dentals |
| DH | 70 | 30 | 2.33 | 43% ⚠ | Dentals |
| AH | 110 | 50 | 2.20 | 45% ⚠ | Vowels |
| EH | 30 | 50 | 0.60 | 60% | Vowels |
| F | 120 | 140 | 0.86 | 86% | Other |
| ER | 40 | 120 | 0.33 | 33% ⚠ | Vowels |
| T | 70 | 40 | 1.75 | 57% | Stops |
| **DSS** | | | | **58.1%** | |

### `05-voiced-dentals` — Those feathers gather there.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| DH | 100 | 60 | 1.67 | 60% | Dentals |
| OW | 180 | 120 | 1.50 | 67% | Vowels |
| Z | 30 | 80 | 0.37 | 37% ⚠ | Sibilants |
| F | 140 | 110 | 1.27 | 79% | Other |
| EH | 100 | 110 | 0.91 | 91% | Vowels |
| DH | 130 | 50 | 2.60 | 38% ⚠ | Dentals |
| ER | 30 | 100 | 0.30 | 30% ⚠ | Vowels |
| Z | 160 | 80 | 2.00 | 50% ⚠ | Sibilants |
| G | 60 | 80 | 0.75 | 75% | Stops |
| AE | 130 | 120 | 1.08 | 92% | Vowels |
| DH | 120 | 50 | 2.40 | 42% ⚠ | Dentals |
| ER | 30 | 80 | 0.38 | 38% ⚠ | Vowels |
| DH | 160 | 40 | 4.00 | 25% ⚠ | Dentals |
| EH | 60 | 140 | 0.43 | 43% ⚠ | Vowels |
| R | 30 | 180 | 0.17 | 17% ⚠ | Liquids |
| **DSS** | | | | **52.2%** | |

### `06-affricates` — Judge the changing church bells.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| JH | 40 | 80 | 0.50 | 50% | Affricates |
| AH | 110 | 100 | 1.10 | 91% | Vowels |
| JH | 30 | 40 | 0.75 | 75% | Affricates |
| DH | 160 | 120 | 1.33 | 75% | Dentals |
| AH | 30 | 60 | 0.50 | 50% ⚠ | Vowels |
| CH | 130 | 130 | 1.00 | 100% | Affricates |
| EJ | 40 | 120 | 0.33 | 33% ⚠ | Other |
| N | 230 | 80 | 2.88 | 35% ⚠ | Nasals |
| JH | 120 | 60 | 2.00 | 50% | Affricates |
| IH | 70 | 70 | 1.00 | 100% | Vowels |
| NG | 60 | 80 | 0.75 | 75% | Nasals |
| CH | 110 | 120 | 0.92 | 92% | Affricates |
| ɝ | 30 | 110 | 0.27 | 27% ⚠ | Other |
| CH | 200 | 110 | 1.82 | 55% | Affricates |
| B | 40 | 30 | 1.33 | 75% | Stops |
| EH | 100 | 100 | 1.00 | 100% | Vowels |
| Ɫ | 80 | 150 | 0.53 | 53% | Other |
| Z | 120 | 190 | 0.63 | 63% | Sibilants |
| **DSS** | | | | **66.6%** | |

### `07-stress` — English fricatives shift sharply.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| IH | 30 | 60 | 0.50 | 50% | Vowels |
| N | 40 | 160 | 0.25 | 25% ⚠ | Nasals |
| JH | 30 | 30 | 1.00 | 100% | Affricates |
| L | 30 | 50 | 0.60 | 60% | Liquids |
| IH | 30 | 60 | 0.50 | 50% | Vowels |
| SH | 30 | 100 | 0.30 | 30% ⚠ | Sibilants |
| SPN | 1390 | 460 | 3.02 | 33% ⚠ | Other |
| SH | 50 | 130 | 0.38 | 38% ⚠ | Sibilants |
| IH | 30 | 60 | 0.50 | 50% | Vowels |
| F | 30 | 50 | 0.60 | 60% | Other |
| T | 30 | 80 | 0.38 | 38% ⚠ | Stops |
| SH | 100 | 130 | 0.77 | 77% | Sibilants |
| AA | 80 | 60 | 1.33 | 75% | Vowels |
| R | 60 | 90 | 0.67 | 67% | Liquids |
| Pʲ | 130 | 90 | 1.44 | 69% | Other |
| L | 80 | 60 | 1.33 | 75% | Liquids |
| IY | 120 | 210 | 0.57 | 57% | Vowels |
| **DSS** | | | | **56.1%** | |

### `08-finals` — Leave these clothes and bags.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| L | 160 | 70 | 2.29 | 44% ⚠ | Liquids |
| IY | 30 | 90 | 0.33 | 33% ⚠ | Vowels |
| V | 170 | 50 | 3.40 | 29% ⚠ | VoicedFric |
| DH | 30 | 40 | 0.75 | 75% | Dentals |
| IY | 80 | 130 | 0.62 | 62% | Vowels |
| Z | 140 | 70 | 2.00 | 50% | Sibilants |
| K | 30 | 110 | 0.27 | 27% ⚠ | Stops |
| L | 80 | 30 | 2.67 | 37% ⚠ | Liquids |
| OW | 230 | 140 | 1.64 | 61% | Vowels |
| DH | 80 | 60 | 1.33 | 75% | Dentals |
| Z | 140 | 80 | 1.75 | 57% | Sibilants |
| AE | 90 | 110 | 0.82 | 82% | Vowels |
| N | 80 | 80 | 1.00 | 100% | Nasals |
| B | 100 | 90 | 1.11 | 90% | Stops |
| AE | 130 | 220 | 0.59 | 59% | Vowels |
| G | 80 | 90 | 0.89 | 89% | Stops |
| Z | 90 | 205 | 0.44 | 44% ⚠ | Sibilants |
| **DSS** | | | | **59.7%** | |

## Family summary

| Family | N | Mean sim | Mean ratio |
|---|---:|---:|---:|
| Vowels | 53 | 64.4% | 0.94 |
| Sibilants | 27 | 61.9% | 1.04 |
| Stops | 13 | 65.4% | 0.96 |
| Nasals | 13 | 67.3% | 1.33 |
| Liquids | 12 | 64.2% | 1.11 |
| Dentals | 13 | 61.1% | 1.76 |
| VoicedFric | 1 | 29.4% | 3.40 |
| Affricates | 9 | 66.6% | 1.27 |
| Other | 25 | 69.2% | 1.08 |

## Improvement leads (sim < 60%)

| ARPA | Sample | ours (ms) | mary (ms) | ratio | sim | family |
|---|---|---:|---:|---:|---:|---|
| EH | `02-hamlet` | 10 | 110 | 0.09 | 9% | Vowels |
| | | | | | → too short | |
| R | `05-voiced-dentals` | 30 | 180 | 0.17 | 17% | Liquids |
| | | | | | → too short | |
| AH | `02-hamlet` | 30 | 130 | 0.23 | 23% | Vowels |
| | | | | | → too short | |
| N | `07-stress` | 40 | 160 | 0.25 | 25% | Nasals |
| | | | | | → too short | |
| DH | `05-voiced-dentals` | 160 | 40 | 4.00 | 25% | Dentals |
| | | | | | → too long | |
| S | `02-hamlet` | 30 | 110 | 0.27 | 27% | Sibilants |
| | | | | | → too short | |
| K | `08-finals` | 30 | 110 | 0.27 | 27% | Stops |
| | | | | | → too short | |
| ɝ | `06-affricates` | 30 | 110 | 0.27 | 27% | Other |
| | | | | | → too short | |
| AE | `02-hamlet` | 140 | 40 | 3.50 | 29% | Vowels |
| | | | | | → too long | |
| V | `08-finals` | 170 | 50 | 3.40 | 29% | VoicedFric |
| | | | | | → too long | |
| SH | `07-stress` | 30 | 100 | 0.30 | 30% | Sibilants |
| | | | | | → too short | |
| ER | `05-voiced-dentals` | 30 | 100 | 0.30 | 30% | Vowels |
| | | | | | → too short | |
| IY | `02-hamlet` | 90 | 290 | 0.31 | 31% | Vowels |
| | | | | | → too short | |
| SPN | `07-stress` | 1390 | 460 | 3.02 | 33% | Other |
| | | | | | → too long | |
| ER | `04-dentals` | 40 | 120 | 0.33 | 33% | Vowels |
| | | | | | → too short | |
| DH | `02-hamlet` | 120 | 40 | 3.00 | 33% | Dentals |
| | | | | | → too long | |
| EJ | `06-affricates` | 40 | 120 | 0.33 | 33% | Other |
| | | | | | → too short | |
| IY | `08-finals` | 30 | 90 | 0.33 | 33% | Vowels |
| | | | | | → too short | |
| JH | `01-demo` | 90 | 30 | 3.00 | 33% | Affricates |
| | | | | | → too long | |
| N | `06-affricates` | 230 | 80 | 2.88 | 35% | Nasals |
| | | | | | → too long | |
| AH | `01-demo` | 110 | 40 | 2.75 | 36% | Vowels |
| | | | | | → too long | |
| L | `08-finals` | 80 | 30 | 2.67 | 37% | Liquids |
| | | | | | → too long | |
| Z | `05-voiced-dentals` | 30 | 80 | 0.37 | 37% | Sibilants |
| | | | | | → too short | |
| Z | `02-hamlet` | 30 | 80 | 0.38 | 38% | Sibilants |
| | | | | | → too short | |
| ER | `05-voiced-dentals` | 30 | 80 | 0.38 | 38% | Vowels |
| | | | | | → too short | |
| T | `07-stress` | 30 | 80 | 0.38 | 38% | Stops |
| | | | | | → too short | |
| S | `04-dentals` | 30 | 80 | 0.38 | 38% | Sibilants |
| | | | | | → too short | |
| SH | `07-stress` | 50 | 130 | 0.38 | 38% | Sibilants |
| | | | | | → too short | |
| DH | `05-voiced-dentals` | 130 | 50 | 2.60 | 38% | Dentals |
| | | | | | → too long | |
| DH | `05-voiced-dentals` | 120 | 50 | 2.40 | 42% | Dentals |
| | | | | | → too long | |
| DH | `04-dentals` | 70 | 30 | 2.33 | 43% | Dentals |
| | | | | | → too long | |
| IH | `04-dentals` | 30 | 70 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| N | `04-dentals` | 210 | 90 | 2.33 | 43% | Nasals |
| | | | | | → too long | |
| EH | `05-voiced-dentals` | 60 | 140 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| Z | `03-sibilants` | 140 | 60 | 2.33 | 43% | Sibilants |
| | | | | | → too long | |
| T̪ | `04-dentals` | 230 | 100 | 2.30 | 43% | Other |
| | | | | | → too long | |
| L | `08-finals` | 160 | 70 | 2.29 | 44% | Liquids |
| | | | | | → too long | |
| Z | `08-finals` | 90 | 205 | 0.44 | 44% | Sibilants |
| | | | | | → too short | |
| IH | `01-demo` | 40 | 90 | 0.44 | 44% | Vowels |
| | | | | | → too short | |
| CH | `02-hamlet` | 40 | 90 | 0.44 | 44% | Affricates |
| | | | | | → too short | |
| AH | `04-dentals` | 110 | 50 | 2.20 | 45% | Vowels |
| | | | | | → too long | |
| SH | `01-demo` | 120 | 60 | 2.00 | 50% | Sibilants |
| | | | | | → too long | |
| AH | `06-affricates` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| R | `03-sibilants` | 110 | 220 | 0.50 | 50% | Liquids |
| | | | | | → too short | |
| Z | `05-voiced-dentals` | 160 | 80 | 2.00 | 50% | Sibilants |
| | | | | | → too long | |
| F | `01-demo` | 50 | 100 | 0.50 | 50% | Other |
| | | | | | → too short | |
| IH | `01-demo` | 120 | 60 | 2.00 | 50% | Vowels |
| | | | | | → too long | |
| Z | `01-demo` | 120 | 60 | 2.00 | 50% | Sibilants |
| | | | | | → too long | |
| N | `02-hamlet` | 140 | 70 | 2.00 | 50% | Nasals |
| | | | | | → too long | |
| IH | `07-stress` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| IH | `07-stress` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| IH | `07-stress` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| Z | `08-finals` | 140 | 70 | 2.00 | 50% | Sibilants |
| | | | | | → too long | |
| IH | `04-dentals` | 40 | 80 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| JH | `06-affricates` | 40 | 80 | 0.50 | 50% | Affricates |
| | | | | | → too short | |
| IY | `03-sibilants` | 40 | 80 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| JH | `06-affricates` | 120 | 60 | 2.00 | 50% | Affricates |
| | | | | | → too long | |
| B | `03-sibilants` | 50 | 100 | 0.50 | 50% | Stops |
| | | | | | → too short | |
| Ɫ | `06-affricates` | 80 | 150 | 0.53 | 53% | Other |
| | | | | | → too short | |
| IH | `01-demo` | 110 | 60 | 1.83 | 55% | Vowels |
| | | | | | → too long | |
| AH | `02-hamlet` | 110 | 60 | 1.83 | 55% | Vowels |
| | | | | | → too long | |
| CH | `06-affricates` | 200 | 110 | 1.82 | 55% | Affricates |
| | | | | | → too long | |
| Tʰ | `02-hamlet` | 50 | 90 | 0.56 | 56% | Other |
| | | | | | → too short | |
| S | `03-sibilants` | 180 | 100 | 1.80 | 56% | Sibilants |
| | | | | | → too long | |
| T | `04-dentals` | 70 | 40 | 1.75 | 57% | Stops |
| | | | | | → too long | |
| Z | `08-finals` | 140 | 80 | 1.75 | 57% | Sibilants |
| | | | | | → too long | |
| IY | `07-stress` | 120 | 210 | 0.57 | 57% | Vowels |
| | | | | | → too short | |
| IY | `02-hamlet` | 70 | 120 | 0.58 | 58% | Vowels |
| | | | | | → too short | |
| N | `02-hamlet` | 120 | 70 | 1.71 | 58% | Nasals |
| | | | | | → too long | |
| AE | `08-finals` | 130 | 220 | 0.59 | 59% | Vowels |
| | | | | | → too short | |
| L | `01-demo` | 30 | 50 | 0.60 | 60% | Liquids |
| | | | | | → too short | |
| B | `01-demo` | 100 | 60 | 1.67 | 60% | Stops |
| | | | | | → too long | |
