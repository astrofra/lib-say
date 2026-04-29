# MFA Duration Similarity Score

- Generated: `2026-04-29T16:09:38+02:00`
- Metric: DSS = mean( min(r, 1/r) ) per phoneme pair, r = ours/mary duration
- 100% = perfect timing match with MaryTTS reference

## Per-sample results

| Sample | Text | DSS | Pairs | LLH ours/s | LLH mary/s |
|---|---|---:|---:|---:|---:|
| `01-demo` | Hello from lib-say. This is an English demo sentence. | **71.3%** | 37 | -13.27 | -15.94 |
| `02-hamlet` | To be or not to be, that is the question. | **60.4%** | 25 | -23.21 | -20.80 |
| `03-sibilants` | She sells seashells by the seashore. | **76.9%** | 21 | -24.63 | -23.59 |
| `04-dentals` | This thing is worth the effort. | **60.9%** | 16 | -34.84 | -38.09 |
| `05-voiced-dentals` | Those feathers gather there. | **52.4%** | 15 | -29.42 | -27.95 |
| `06-affricates` | Judge the changing church bells. | **71.2%** | 18 | -27.20 | -33.67 |
| `07-stress` | English fricatives shift sharply. | **57.0%** | 17 | -21.51 | -25.53 |
| `08-finals` | Leave these clothes and bags. | **53.1%** | 16 | -22.45 | -35.45 |

**Global DSS: 62.9%**

## Per-sample phoneme pairs

### `01-demo` — Hello from lib-say. This is an English demo sentence.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| HH | 80 | 50 | 1.60 | 62% | Other |
| AH | 30 | 30 | 1.00 | 100% | Vowels |
| L | 80 | 70 | 1.14 | 87% | Liquids |
| OW | 150 | 120 | 1.25 | 80% | Vowels |
| F | 50 | 120 | 0.42 | 42% ⚠ | Other |
| R | 40 | 30 | 1.33 | 75% | Liquids |
| AH | 110 | 40 | 2.75 | 36% ⚠ | Vowels |
| M | 90 | 60 | 1.50 | 67% | Nasals |
| L | 30 | 70 | 0.43 | 43% ⚠ | Liquids |
| IH | 110 | 60 | 1.83 | 55% | Vowels |
| B | 100 | 60 | 1.67 | 60% | Stops |
| S | 100 | 130 | 0.77 | 77% | Sibilants |
| EJ | 220 | 330 | 0.67 | 67% | Other |
| DH | 30 | 40 | 0.75 | 75% | Dentals |
| IH | 70 | 70 | 1.00 | 100% | Vowels |
| S | 120 | 80 | 1.50 | 67% | Sibilants |
| IH | 40 | 100 | 0.40 | 40% ⚠ | Vowels |
| Z | 120 | 60 | 2.00 | 50% | Sibilants |
| AH | 50 | 60 | 0.83 | 83% | Vowels |
| N | 60 | 70 | 0.86 | 86% | Nasals |
| IH | 100 | 100 | 1.00 | 100% | Vowels |
| N | 70 | 90 | 0.78 | 78% | Nasals |
| JH | 90 | 30 | 3.00 | 33% ⚠ | Affricates |
| L | 70 | 60 | 1.17 | 86% | Liquids |
| IH | 120 | 60 | 2.00 | 50% | Vowels |
| SH | 120 | 80 | 1.50 | 67% | Sibilants |
| D | 60 | 70 | 0.86 | 86% | Stops |
| EH | 90 | 80 | 1.12 | 89% | Vowels |
| M | 90 | 70 | 1.29 | 78% | Nasals |
| OW | 120 | 150 | 0.80 | 80% | Vowels |
| S | 80 | 110 | 0.73 | 73% | Sibilants |
| EH | 90 | 90 | 1.00 | 100% | Vowels |
| N | 100 | 70 | 1.43 | 70% | Nasals |
| T | 80 | 50 | 1.60 | 63% | Stops |
| AH | 60 | 70 | 0.86 | 86% | Vowels |
| N | 80 | 100 | 0.80 | 80% | Nasals |
| S | 110 | 160 | 0.69 | 69% | Sibilants |
| **DSS** | | | | **71.3%** | |

### `02-hamlet` — To be or not to be, that is the question.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| Tʰ | 50 | 90 | 0.56 | 56% | Other |
| Ʉː | 70 | 60 | 1.17 | 86% | Other |
| Bʲ | 60 | 70 | 0.86 | 86% | Other |
| IY | 70 | 110 | 0.64 | 64% | Vowels |
| Ɒ | 80 | 70 | 1.14 | 87% | Other |
| R | 60 | 60 | 1.00 | 100% | Liquids |
| N | 140 | 70 | 2.00 | 50% | Nasals |
| AA | 120 | 150 | 0.80 | 80% | Vowels |
| T | 40 | 40 | 1.00 | 100% | Stops |
| Tʰ | 100 | 90 | 1.11 | 90% | Other |
| Ʉː | 60 | 60 | 1.00 | 100% | Other |
| Bʲ | 70 | 50 | 1.40 | 71% | Other |
| IY | 90 | 280 | 0.32 | 32% ⚠ | Vowels |
| AE | 140 | 40 | 3.50 | 29% ⚠ | Vowels |
| T | 80 | 100 | 0.80 | 80% | Stops |
| IH | 30 | 40 | 0.75 | 75% | Vowels |
| Z | 30 | 80 | 0.38 | 38% ⚠ | Sibilants |
| DH | 120 | 40 | 3.00 | 33% ⚠ | Dentals |
| AH | 110 | 60 | 1.83 | 55% | Vowels |
| Cʷ | 90 | 40 | 2.25 | 44% ⚠ | Other |
| EH | 30 | 110 | 0.27 | 27% ⚠ | Vowels |
| S | 30 | 110 | 0.27 | 27% ⚠ | Sibilants |
| CH | 30 | 90 | 0.33 | 33% ⚠ | Affricates |
| AH | 30 | 130 | 0.23 | 23% ⚠ | Vowels |
| N | 30 | 70 | 0.43 | 43% ⚠ | Nasals |
| **DSS** | | | | **60.4%** | |

### `03-sibilants` — She sells seashells by the seashore.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| SH | 100 | 150 | 0.67 | 67% | Sibilants |
| IY | 40 | 70 | 0.57 | 57% | Vowels |
| S | 100 | 120 | 0.83 | 83% | Sibilants |
| EH | 80 | 70 | 1.14 | 87% | Vowels |
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
| **DSS** | | | | **76.9%** | |

### `04-dentals` — This thing is worth the effort.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| DH | 30 | 50 | 0.60 | 60% | Dentals |
| IH | 40 | 80 | 0.50 | 50% | Vowels |
| S | 30 | 60 | 0.50 | 50% | Sibilants |
| T̪ | 230 | 120 | 1.92 | 52% | Other |
| IH | 30 | 70 | 0.43 | 43% ⚠ | Vowels |
| N | 210 | 100 | 2.10 | 48% ⚠ | Nasals |
| Z | 70 | 70 | 1.00 | 100% | Sibilants |
| W | 70 | 60 | 1.17 | 86% | Other |
| ɝ | 80 | 60 | 1.33 | 75% | Other |
| TH | 150 | 130 | 1.15 | 87% | Dentals |
| DH | 70 | 30 | 2.33 | 43% ⚠ | Dentals |
| AH | 110 | 50 | 2.20 | 45% ⚠ | Vowels |
| EH | 30 | 50 | 0.60 | 60% | Vowels |
| F | 120 | 140 | 0.86 | 86% | Other |
| ER | 40 | 120 | 0.33 | 33% ⚠ | Vowels |
| T | 70 | 40 | 1.75 | 57% | Stops |
| **DSS** | | | | **60.9%** | |

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
| EH | 60 | 130 | 0.46 | 46% ⚠ | Vowels |
| R | 30 | 190 | 0.16 | 16% ⚠ | Liquids |
| **DSS** | | | | **52.4%** | |

### `06-affricates` — Judge the changing church bells.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| JH | 50 | 80 | 0.63 | 63% | Affricates |
| AH | 90 | 100 | 0.90 | 90% | Vowels |
| JH | 40 | 40 | 1.00 | 100% | Affricates |
| DH | 150 | 120 | 1.25 | 80% | Dentals |
| AH | 30 | 60 | 0.50 | 50% | Vowels |
| CH | 120 | 120 | 1.00 | 100% | Affricates |
| EJ | 40 | 130 | 0.31 | 31% ⚠ | Other |
| N | 230 | 80 | 2.88 | 35% ⚠ | Nasals |
| JH | 110 | 60 | 1.83 | 55% | Affricates |
| IH | 70 | 70 | 1.00 | 100% | Vowels |
| NG | 70 | 80 | 0.88 | 88% | Nasals |
| CH | 100 | 120 | 0.83 | 83% | Affricates |
| ɝ | 30 | 110 | 0.27 | 27% ⚠ | Other |
| CH | 140 | 140 | 1.00 | 100% | Affricates |
| B | 30 | 40 | 0.75 | 75% | Stops |
| EH | 90 | 100 | 0.90 | 90% | Vowels |
| Ɫ | 70 | 150 | 0.47 | 47% ⚠ | Other |
| Z | 130 | 190 | 0.68 | 68% | Sibilants |
| **DSS** | | | | **71.2%** | |

### `07-stress` — English fricatives shift sharply.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| IH | 30 | 70 | 0.43 | 43% ⚠ | Vowels |
| N | 40 | 150 | 0.27 | 27% ⚠ | Nasals |
| JH | 30 | 30 | 1.00 | 100% | Affricates |
| L | 30 | 50 | 0.60 | 60% | Liquids |
| IH | 30 | 60 | 0.50 | 50% | Vowels |
| SH | 30 | 90 | 0.33 | 33% ⚠ | Sibilants |
| SPN | 1390 | 550 | 2.53 | 40% ⚠ | Other |
| SH | 50 | 130 | 0.38 | 38% ⚠ | Sibilants |
| IH | 30 | 60 | 0.50 | 50% | Vowels |
| F | 30 | 90 | 0.33 | 33% ⚠ | Other |
| T | 30 | 40 | 0.75 | 75% | Stops |
| SH | 100 | 130 | 0.77 | 77% | Sibilants |
| AA | 80 | 60 | 1.33 | 75% | Vowels |
| R | 60 | 90 | 0.67 | 67% | Liquids |
| Pʲ | 130 | 90 | 1.44 | 69% | Other |
| L | 80 | 60 | 1.33 | 75% | Liquids |
| IY | 120 | 210 | 0.57 | 57% | Vowels |
| **DSS** | | | | **57.0%** | |

### `08-finals` — Leave these clothes and bags.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| L | 160 | 70 | 2.29 | 44% ⚠ | Liquids |
| IY | 30 | 90 | 0.33 | 33% ⚠ | Vowels |
| V | 170 | 50 | 3.40 | 29% ⚠ | VoicedFric |
| DH | 30 | 50 | 0.60 | 60% | Dentals |
| IY | 80 | 120 | 0.67 | 67% | Vowels |
| Z | 140 | 70 | 2.00 | 50% | Sibilants |
| K | 30 | 110 | 0.27 | 27% ⚠ | Stops |
| L | 80 | 30 | 2.67 | 37% ⚠ | Liquids |
| OW | 230 | 150 | 1.53 | 65% | Vowels |
| DH | 80 | 40 | 2.00 | 50% | Dentals |
| Z | 140 | 90 | 1.56 | 64% | Sibilants |
| AE | 90 | 80 | 1.12 | 89% | Vowels |
| N | 80 | 90 | 0.89 | 89% | Nasals |
| B | 100 | 220 | 0.45 | 45% ⚠ | Stops |
| AE | 130 | 80 | 1.63 | 62% | Vowels |
| G | 80 | 215 | 0.37 | 37% ⚠ | Stops |
| **DSS** | | | | **53.1%** | |

## Family summary

| Family | N | Mean sim | Mean ratio |
|---|---:|---:|---:|
| Vowels | 53 | 65.2% | 0.97 |
| Sibilants | 26 | 64.1% | 1.05 |
| Stops | 13 | 63.9% | 0.89 |
| Nasals | 13 | 64.3% | 1.24 |
| Liquids | 12 | 61.6% | 1.11 |
| Dentals | 13 | 56.0% | 1.78 |
| VoicedFric | 1 | 29.4% | 3.40 |
| Affricates | 9 | 74.1% | 1.18 |
| Other | 25 | 66.7% | 1.07 |

## Improvement leads (sim < 60%)

| ARPA | Sample | ours (ms) | mary (ms) | ratio | sim | family |
|---|---|---:|---:|---:|---:|---|
| R | `05-voiced-dentals` | 30 | 190 | 0.16 | 16% | Liquids |
| | | | | | → too short | |
| AH | `02-hamlet` | 30 | 130 | 0.23 | 23% | Vowels |
| | | | | | → too short | |
| DH | `05-voiced-dentals` | 160 | 40 | 4.00 | 25% | Dentals |
| | | | | | → too long | |
| N | `07-stress` | 40 | 150 | 0.27 | 27% | Nasals |
| | | | | | → too short | |
| S | `02-hamlet` | 30 | 110 | 0.27 | 27% | Sibilants |
| | | | | | → too short | |
| K | `08-finals` | 30 | 110 | 0.27 | 27% | Stops |
| | | | | | → too short | |
| ɝ | `06-affricates` | 30 | 110 | 0.27 | 27% | Other |
| | | | | | → too short | |
| EH | `02-hamlet` | 30 | 110 | 0.27 | 27% | Vowels |
| | | | | | → too short | |
| AE | `02-hamlet` | 140 | 40 | 3.50 | 29% | Vowels |
| | | | | | → too long | |
| V | `08-finals` | 170 | 50 | 3.40 | 29% | VoicedFric |
| | | | | | → too long | |
| ER | `05-voiced-dentals` | 30 | 100 | 0.30 | 30% | Vowels |
| | | | | | → too short | |
| EJ | `06-affricates` | 40 | 130 | 0.31 | 31% | Other |
| | | | | | → too short | |
| IY | `02-hamlet` | 90 | 280 | 0.32 | 32% | Vowels |
| | | | | | → too short | |
| ER | `04-dentals` | 40 | 120 | 0.33 | 33% | Vowels |
| | | | | | → too short | |
| DH | `02-hamlet` | 120 | 40 | 3.00 | 33% | Dentals |
| | | | | | → too long | |
| SH | `07-stress` | 30 | 90 | 0.33 | 33% | Sibilants |
| | | | | | → too short | |
| F | `07-stress` | 30 | 90 | 0.33 | 33% | Other |
| | | | | | → too short | |
| IY | `08-finals` | 30 | 90 | 0.33 | 33% | Vowels |
| | | | | | → too short | |
| CH | `02-hamlet` | 30 | 90 | 0.33 | 33% | Affricates |
| | | | | | → too short | |
| JH | `01-demo` | 90 | 30 | 3.00 | 33% | Affricates |
| | | | | | → too long | |
| N | `06-affricates` | 230 | 80 | 2.88 | 35% | Nasals |
| | | | | | → too long | |
| AH | `01-demo` | 110 | 40 | 2.75 | 36% | Vowels |
| | | | | | → too long | |
| G | `08-finals` | 80 | 215 | 0.37 | 37% | Stops |
| | | | | | → too short | |
| L | `08-finals` | 80 | 30 | 2.67 | 37% | Liquids |
| | | | | | → too long | |
| Z | `05-voiced-dentals` | 30 | 80 | 0.37 | 37% | Sibilants |
| | | | | | → too short | |
| Z | `02-hamlet` | 30 | 80 | 0.38 | 38% | Sibilants |
| | | | | | → too short | |
| ER | `05-voiced-dentals` | 30 | 80 | 0.38 | 38% | Vowels |
| | | | | | → too short | |
| SH | `07-stress` | 50 | 130 | 0.38 | 38% | Sibilants |
| | | | | | → too short | |
| DH | `05-voiced-dentals` | 130 | 50 | 2.60 | 38% | Dentals |
| | | | | | → too long | |
| SPN | `07-stress` | 1390 | 550 | 2.53 | 40% | Other |
| | | | | | → too long | |
| IH | `01-demo` | 40 | 100 | 0.40 | 40% | Vowels |
| | | | | | → too short | |
| DH | `05-voiced-dentals` | 120 | 50 | 2.40 | 42% | Dentals |
| | | | | | → too long | |
| F | `01-demo` | 50 | 120 | 0.42 | 42% | Other |
| | | | | | → too short | |
| DH | `04-dentals` | 70 | 30 | 2.33 | 43% | Dentals |
| | | | | | → too long | |
| L | `01-demo` | 30 | 70 | 0.43 | 43% | Liquids |
| | | | | | → too short | |
| IH | `04-dentals` | 30 | 70 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| IH | `07-stress` | 30 | 70 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| Z | `03-sibilants` | 140 | 60 | 2.33 | 43% | Sibilants |
| | | | | | → too long | |
| N | `02-hamlet` | 30 | 70 | 0.43 | 43% | Nasals |
| | | | | | → too short | |
| L | `08-finals` | 160 | 70 | 2.29 | 44% | Liquids |
| | | | | | → too long | |
| Cʷ | `02-hamlet` | 90 | 40 | 2.25 | 44% | Other |
| | | | | | → too long | |
| AH | `04-dentals` | 110 | 50 | 2.20 | 45% | Vowels |
| | | | | | → too long | |
| B | `08-finals` | 100 | 220 | 0.45 | 45% | Stops |
| | | | | | → too short | |
| EH | `05-voiced-dentals` | 60 | 130 | 0.46 | 46% | Vowels |
| | | | | | → too short | |
| Ɫ | `06-affricates` | 70 | 150 | 0.47 | 47% | Other |
| | | | | | → too short | |
| N | `04-dentals` | 210 | 100 | 2.10 | 48% | Nasals |
| | | | | | → too long | |
| R | `03-sibilants` | 110 | 220 | 0.50 | 50% | Liquids |
| | | | | | → too short | |
| Z | `05-voiced-dentals` | 160 | 80 | 2.00 | 50% | Sibilants |
| | | | | | → too long | |
| IH | `01-demo` | 120 | 60 | 2.00 | 50% | Vowels |
| | | | | | → too long | |
| Z | `01-demo` | 120 | 60 | 2.00 | 50% | Sibilants |
| | | | | | → too long | |
| N | `02-hamlet` | 140 | 70 | 2.00 | 50% | Nasals |
| | | | | | → too long | |
| S | `04-dentals` | 30 | 60 | 0.50 | 50% | Sibilants |
| | | | | | → too short | |
| IH | `07-stress` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| IH | `07-stress` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| DH | `08-finals` | 80 | 40 | 2.00 | 50% | Dentals |
| | | | | | → too long | |
| Z | `08-finals` | 140 | 70 | 2.00 | 50% | Sibilants |
| | | | | | → too long | |
| IH | `04-dentals` | 40 | 80 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| AH | `06-affricates` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| B | `03-sibilants` | 50 | 100 | 0.50 | 50% | Stops |
| | | | | | → too short | |
| T̪ | `04-dentals` | 230 | 120 | 1.92 | 52% | Other |
| | | | | | → too long | |
| IH | `01-demo` | 110 | 60 | 1.83 | 55% | Vowels |
| | | | | | → too long | |
| AH | `02-hamlet` | 110 | 60 | 1.83 | 55% | Vowels |
| | | | | | → too long | |
| JH | `06-affricates` | 110 | 60 | 1.83 | 55% | Affricates |
| | | | | | → too long | |
| Tʰ | `02-hamlet` | 50 | 90 | 0.56 | 56% | Other |
| | | | | | → too short | |
| S | `03-sibilants` | 180 | 100 | 1.80 | 56% | Sibilants |
| | | | | | → too long | |
| IY | `03-sibilants` | 40 | 70 | 0.57 | 57% | Vowels |
| | | | | | → too short | |
| T | `04-dentals` | 70 | 40 | 1.75 | 57% | Stops |
| | | | | | → too long | |
| IY | `07-stress` | 120 | 210 | 0.57 | 57% | Vowels |
| | | | | | → too short | |
| DH | `08-finals` | 30 | 50 | 0.60 | 60% | Dentals |
| | | | | | → too short | |
| B | `01-demo` | 100 | 60 | 1.67 | 60% | Stops |
| | | | | | → too long | |
