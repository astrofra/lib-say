# MFA Duration Similarity Score

- Generated: `2026-04-29T17:32:49+02:00`
- Metric: DSS = mean( min(r, 1/r) ) per phoneme pair, r = ours/mary duration
- 100% = perfect timing match with MaryTTS reference

## Per-sample results

| Sample | Text | DSS | Pairs | LLH ours/s | LLH mary/s |
|---|---|---:|---:|---:|---:|
| `01-demo` | Hello from lib-say. This is an English demo sentence. | **69.1%** | 37 | -13.70 | -15.78 |
| `02-hamlet` | To be or not to be, that is the question. | **61.4%** | 25 | -23.64 | -20.77 |
| `03-sibilants` | She sells seashells by the seashore. | **75.7%** | 21 | -25.13 | -24.37 |
| `04-dentals` | This thing is worth the effort. | **57.4%** | 16 | -33.75 | -34.84 |
| `05-voiced-dentals` | Those feathers gather there. | **51.9%** | 15 | -29.27 | -32.62 |
| `06-affricates` | Judge the changing church bells. | **70.4%** | 18 | -27.72 | -33.52 |
| `07-stress` | English fricatives shift sharply. | **58.3%** | 17 | -21.67 | -25.48 |
| `08-finals` | Leave these clothes and bags. | **55.5%** | 16 | -23.01 | -35.36 |

**Global DSS: 62.5%**

## Per-sample phoneme pairs

### `01-demo` — Hello from lib-say. This is an English demo sentence.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| HH | 80 | 30 | 2.67 | 38% ⚠ | Other |
| AH | 30 | 30 | 1.00 | 100% | Vowels |
| L | 80 | 70 | 1.14 | 87% | Liquids |
| OW | 150 | 120 | 1.25 | 80% | Vowels |
| F | 50 | 120 | 0.42 | 42% ⚠ | Other |
| R | 40 | 30 | 1.33 | 75% | Liquids |
| AH | 110 | 40 | 2.75 | 36% ⚠ | Vowels |
| M | 110 | 80 | 1.38 | 73% | Nasals |
| L | 110 | 50 | 2.20 | 45% ⚠ | Liquids |
| IH | 30 | 60 | 0.50 | 50% ⚠ | Vowels |
| B | 80 | 60 | 1.33 | 75% | Stops |
| S | 110 | 130 | 0.85 | 85% | Sibilants |
| EJ | 210 | 320 | 0.66 | 66% | Other |
| DH | 30 | 30 | 1.00 | 100% | Dentals |
| IH | 70 | 80 | 0.87 | 87% | Vowels |
| S | 120 | 90 | 1.33 | 75% | Sibilants |
| IH | 40 | 90 | 0.44 | 44% ⚠ | Vowels |
| Z | 120 | 60 | 2.00 | 50% | Sibilants |
| AH | 50 | 60 | 0.83 | 83% | Vowels |
| N | 60 | 80 | 0.75 | 75% | Nasals |
| IH | 30 | 90 | 0.33 | 33% ⚠ | Vowels |
| N | 140 | 90 | 1.56 | 64% | Nasals |
| JH | 90 | 30 | 3.00 | 33% ⚠ | Affricates |
| L | 40 | 60 | 0.67 | 67% | Liquids |
| IH | 140 | 60 | 2.33 | 43% ⚠ | Vowels |
| SH | 130 | 100 | 1.30 | 77% | Sibilants |
| D | 60 | 50 | 1.20 | 83% | Stops |
| EH | 90 | 80 | 1.12 | 89% | Vowels |
| M | 90 | 70 | 1.29 | 78% | Nasals |
| OW | 120 | 150 | 0.80 | 80% | Vowels |
| S | 80 | 110 | 0.73 | 73% | Sibilants |
| EH | 90 | 90 | 1.00 | 100% | Vowels |
| N | 100 | 60 | 1.67 | 60% | Nasals |
| T | 80 | 60 | 1.33 | 75% | Stops |
| AH | 60 | 70 | 0.86 | 86% | Vowels |
| N | 80 | 100 | 0.80 | 80% | Nasals |
| S | 110 | 160 | 0.69 | 69% | Sibilants |
| **DSS** | | | | **69.1%** | |

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
| IY | 90 | 300 | 0.30 | 30% ⚠ | Vowels |
| AE | 120 | 40 | 3.00 | 33% ⚠ | Vowels |
| T | 100 | 100 | 1.00 | 100% | Stops |
| IH | 30 | 40 | 0.75 | 75% | Vowels |
| Z | 30 | 80 | 0.38 | 38% ⚠ | Sibilants |
| DH | 110 | 40 | 2.75 | 36% ⚠ | Dentals |
| AH | 110 | 60 | 1.83 | 55% | Vowels |
| Cʷ | 90 | 40 | 2.25 | 44% ⚠ | Other |
| EH | 30 | 110 | 0.27 | 27% ⚠ | Vowels |
| S | 30 | 110 | 0.27 | 27% ⚠ | Sibilants |
| CH | 30 | 90 | 0.33 | 33% ⚠ | Affricates |
| AH | 30 | 130 | 0.23 | 23% ⚠ | Vowels |
| N | 30 | 70 | 0.43 | 43% ⚠ | Nasals |
| **DSS** | | | | **61.4%** | |

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
| Z | 140 | 50 | 2.80 | 36% ⚠ | Sibilants |
| B | 50 | 110 | 0.45 | 45% ⚠ | Stops |
| AY | 180 | 130 | 1.38 | 72% | Vowels |
| DH | 30 | 40 | 0.75 | 75% | Dentals |
| AH | 30 | 50 | 0.60 | 60% | Vowels |
| S | 110 | 110 | 1.00 | 100% | Sibilants |
| IY | 130 | 120 | 1.08 | 92% | Vowels |
| SH | 110 | 110 | 1.00 | 100% | Sibilants |
| Ɒ | 70 | 110 | 0.64 | 64% | Other |
| R | 40 | 220 | 0.18 | 18% ⚠ | Liquids |
| **DSS** | | | | **75.7%** | |

### `04-dentals` — This thing is worth the effort.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| DH | 50 | 60 | 0.83 | 83% | Dentals |
| IH | 30 | 70 | 0.43 | 43% ⚠ | Vowels |
| S | 30 | 80 | 0.38 | 38% ⚠ | Sibilants |
| T̪ | 230 | 100 | 2.30 | 43% ⚠ | Other |
| IH | 30 | 70 | 0.43 | 43% ⚠ | Vowels |
| N | 210 | 80 | 2.62 | 38% ⚠ | Nasals |
| Z | 70 | 90 | 0.78 | 78% | Sibilants |
| W | 70 | 60 | 1.17 | 86% | Other |
| ɝ | 80 | 60 | 1.33 | 75% | Other |
| TH | 150 | 130 | 1.15 | 87% | Dentals |
| DH | 70 | 30 | 2.33 | 43% ⚠ | Dentals |
| AH | 110 | 40 | 2.75 | 36% ⚠ | Vowels |
| EH | 30 | 60 | 0.50 | 50% | Vowels |
| F | 120 | 140 | 0.86 | 86% | Other |
| ER | 40 | 120 | 0.33 | 33% ⚠ | Vowels |
| T | 70 | 40 | 1.75 | 57% | Stops |
| **DSS** | | | | **57.4%** | |

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
| Z | 160 | 90 | 1.78 | 56% | Sibilants |
| G | 70 | 70 | 1.00 | 100% | Stops |
| AE | 60 | 120 | 0.50 | 50% | Vowels |
| DH | 30 | 50 | 0.60 | 60% | Dentals |
| ER | 30 | 80 | 0.38 | 38% ⚠ | Vowels |
| DH | 310 | 40 | 7.75 | 13% ⚠ | Dentals |
| EH | 60 | 140 | 0.43 | 43% ⚠ | Vowels |
| R | 30 | 180 | 0.17 | 17% ⚠ | Liquids |
| **DSS** | | | | **51.9%** | |

### `06-affricates` — Judge the changing church bells.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| JH | 50 | 80 | 0.63 | 63% | Affricates |
| AH | 90 | 100 | 0.90 | 90% | Vowels |
| JH | 40 | 40 | 1.00 | 100% | Affricates |
| DH | 140 | 120 | 1.17 | 86% | Dentals |
| AH | 30 | 60 | 0.50 | 50% | Vowels |
| CH | 120 | 130 | 0.92 | 92% | Affricates |
| EJ | 40 | 120 | 0.33 | 33% ⚠ | Other |
| N | 230 | 80 | 2.88 | 35% ⚠ | Nasals |
| JH | 110 | 60 | 1.83 | 55% | Affricates |
| IH | 60 | 70 | 0.86 | 86% | Vowels |
| NG | 70 | 80 | 0.88 | 88% | Nasals |
| CH | 100 | 120 | 0.83 | 83% | Affricates |
| ɝ | 30 | 110 | 0.27 | 27% ⚠ | Other |
| CH | 140 | 140 | 1.00 | 100% | Affricates |
| B | 30 | 40 | 0.75 | 75% | Stops |
| EH | 90 | 100 | 0.90 | 90% | Vowels |
| Ɫ | 70 | 160 | 0.44 | 44% ⚠ | Other |
| Z | 130 | 180 | 0.72 | 72% | Sibilants |
| **DSS** | | | | **70.4%** | |

### `07-stress` — English fricatives shift sharply.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| IH | 30 | 60 | 0.50 | 50% | Vowels |
| N | 40 | 160 | 0.25 | 25% ⚠ | Nasals |
| JH | 30 | 30 | 1.00 | 100% | Affricates |
| L | 30 | 50 | 0.60 | 60% | Liquids |
| IH | 30 | 60 | 0.50 | 50% | Vowels |
| SH | 30 | 80 | 0.37 | 37% ⚠ | Sibilants |
| SPN | 1390 | 470 | 2.96 | 34% ⚠ | Other |
| SH | 50 | 120 | 0.42 | 42% ⚠ | Sibilants |
| IH | 30 | 60 | 0.50 | 50% | Vowels |
| F | 30 | 90 | 0.33 | 33% ⚠ | Other |
| T | 30 | 40 | 0.75 | 75% | Stops |
| SH | 100 | 130 | 0.77 | 77% | Sibilants |
| AA | 80 | 60 | 1.33 | 75% | Vowels |
| R | 60 | 90 | 0.67 | 67% | Liquids |
| Pʲ | 130 | 90 | 1.44 | 69% | Other |
| L | 70 | 60 | 1.17 | 86% | Liquids |
| IY | 130 | 210 | 0.62 | 62% | Vowels |
| **DSS** | | | | **58.3%** | |

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
| Z | 140 | 100 | 1.40 | 71% | Sibilants |
| AE | 80 | 80 | 1.00 | 100% | Vowels |
| N | 90 | 90 | 1.00 | 100% | Nasals |
| B | 100 | 210 | 0.48 | 48% ⚠ | Stops |
| AE | 130 | 90 | 1.44 | 69% | Vowels |
| G | 80 | 215 | 0.37 | 37% ⚠ | Stops |
| **DSS** | | | | **55.5%** | |

## Family summary

| Family | N | Mean sim | Mean ratio |
|---|---:|---:|---:|
| Vowels | 53 | 62.7% | 0.91 |
| Sibilants | 26 | 64.9% | 1.04 |
| Stops | 13 | 69.1% | 0.90 |
| Nasals | 13 | 62.2% | 1.35 |
| Liquids | 12 | 58.6% | 1.17 |
| Dentals | 13 | 60.9% | 1.94 |
| VoicedFric | 1 | 29.4% | 3.40 |
| Affricates | 9 | 73.3% | 1.17 |
| Other | 25 | 65.1% | 1.15 |

## Improvement leads (sim < 60%)

| ARPA | Sample | ours (ms) | mary (ms) | ratio | sim | family |
|---|---|---:|---:|---:|---:|---|
| DH | `05-voiced-dentals` | 310 | 40 | 7.75 | 13% | Dentals |
| | | | | | → too long | |
| R | `05-voiced-dentals` | 30 | 180 | 0.17 | 17% | Liquids |
| | | | | | → too short | |
| R | `03-sibilants` | 40 | 220 | 0.18 | 18% | Liquids |
| | | | | | → too short | |
| AH | `02-hamlet` | 30 | 130 | 0.23 | 23% | Vowels |
| | | | | | → too short | |
| N | `07-stress` | 40 | 160 | 0.25 | 25% | Nasals |
| | | | | | → too short | |
| S | `02-hamlet` | 30 | 110 | 0.27 | 27% | Sibilants |
| | | | | | → too short | |
| K | `08-finals` | 30 | 110 | 0.27 | 27% | Stops |
| | | | | | → too short | |
| ɝ | `06-affricates` | 30 | 110 | 0.27 | 27% | Other |
| | | | | | → too short | |
| EH | `02-hamlet` | 30 | 110 | 0.27 | 27% | Vowels |
| | | | | | → too short | |
| V | `08-finals` | 170 | 50 | 3.40 | 29% | VoicedFric |
| | | | | | → too long | |
| IY | `02-hamlet` | 90 | 300 | 0.30 | 30% | Vowels |
| | | | | | → too short | |
| ER | `05-voiced-dentals` | 30 | 100 | 0.30 | 30% | Vowels |
| | | | | | → too short | |
| ER | `04-dentals` | 40 | 120 | 0.33 | 33% | Vowels |
| | | | | | → too short | |
| F | `07-stress` | 30 | 90 | 0.33 | 33% | Other |
| | | | | | → too short | |
| IY | `08-finals` | 30 | 90 | 0.33 | 33% | Vowels |
| | | | | | → too short | |
| EJ | `06-affricates` | 40 | 120 | 0.33 | 33% | Other |
| | | | | | → too short | |
| AE | `02-hamlet` | 120 | 40 | 3.00 | 33% | Vowels |
| | | | | | → too long | |
| CH | `02-hamlet` | 30 | 90 | 0.33 | 33% | Affricates |
| | | | | | → too short | |
| IH | `01-demo` | 30 | 90 | 0.33 | 33% | Vowels |
| | | | | | → too short | |
| JH | `01-demo` | 90 | 30 | 3.00 | 33% | Affricates |
| | | | | | → too long | |
| SPN | `07-stress` | 1390 | 470 | 2.96 | 34% | Other |
| | | | | | → too long | |
| N | `06-affricates` | 230 | 80 | 2.88 | 35% | Nasals |
| | | | | | → too long | |
| Z | `03-sibilants` | 140 | 50 | 2.80 | 36% | Sibilants |
| | | | | | → too long | |
| DH | `02-hamlet` | 110 | 40 | 2.75 | 36% | Dentals |
| | | | | | → too long | |
| AH | `04-dentals` | 110 | 40 | 2.75 | 36% | Vowels |
| | | | | | → too long | |
| AH | `01-demo` | 110 | 40 | 2.75 | 36% | Vowels |
| | | | | | → too long | |
| G | `08-finals` | 80 | 215 | 0.37 | 37% | Stops |
| | | | | | → too short | |
| L | `08-finals` | 80 | 30 | 2.67 | 37% | Liquids |
| | | | | | → too long | |
| Z | `05-voiced-dentals` | 30 | 80 | 0.37 | 37% | Sibilants |
| | | | | | → too short | |
| SH | `07-stress` | 30 | 80 | 0.37 | 37% | Sibilants |
| | | | | | → too short | |
| HH | `01-demo` | 80 | 30 | 2.67 | 38% | Other |
| | | | | | → too long | |
| Z | `02-hamlet` | 30 | 80 | 0.38 | 38% | Sibilants |
| | | | | | → too short | |
| ER | `05-voiced-dentals` | 30 | 80 | 0.38 | 38% | Vowels |
| | | | | | → too short | |
| S | `04-dentals` | 30 | 80 | 0.38 | 38% | Sibilants |
| | | | | | → too short | |
| N | `04-dentals` | 210 | 80 | 2.62 | 38% | Nasals |
| | | | | | → too long | |
| DH | `05-voiced-dentals` | 130 | 50 | 2.60 | 38% | Dentals |
| | | | | | → too long | |
| SH | `07-stress` | 50 | 120 | 0.42 | 42% | Sibilants |
| | | | | | → too short | |
| F | `01-demo` | 50 | 120 | 0.42 | 42% | Other |
| | | | | | → too short | |
| DH | `04-dentals` | 70 | 30 | 2.33 | 43% | Dentals |
| | | | | | → too long | |
| IH | `04-dentals` | 30 | 70 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| IH | `01-demo` | 140 | 60 | 2.33 | 43% | Vowels |
| | | | | | → too long | |
| IH | `04-dentals` | 30 | 70 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| EH | `05-voiced-dentals` | 60 | 140 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| N | `02-hamlet` | 30 | 70 | 0.43 | 43% | Nasals |
| | | | | | → too short | |
| T̪ | `04-dentals` | 230 | 100 | 2.30 | 43% | Other |
| | | | | | → too long | |
| L | `08-finals` | 160 | 70 | 2.29 | 44% | Liquids |
| | | | | | → too long | |
| Ɫ | `06-affricates` | 70 | 160 | 0.44 | 44% | Other |
| | | | | | → too short | |
| IH | `01-demo` | 40 | 90 | 0.44 | 44% | Vowels |
| | | | | | → too short | |
| Cʷ | `02-hamlet` | 90 | 40 | 2.25 | 44% | Other |
| | | | | | → too long | |
| L | `01-demo` | 110 | 50 | 2.20 | 45% | Liquids |
| | | | | | → too long | |
| B | `03-sibilants` | 50 | 110 | 0.45 | 45% | Stops |
| | | | | | → too short | |
| B | `08-finals` | 100 | 210 | 0.48 | 48% | Stops |
| | | | | | → too short | |
| IH | `01-demo` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| Z | `01-demo` | 120 | 60 | 2.00 | 50% | Sibilants |
| | | | | | → too long | |
| N | `02-hamlet` | 140 | 70 | 2.00 | 50% | Nasals |
| | | | | | → too long | |
| EH | `04-dentals` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| IH | `07-stress` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| IH | `07-stress` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| IH | `07-stress` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| DH | `08-finals` | 80 | 40 | 2.00 | 50% | Dentals |
| | | | | | → too long | |
| Z | `08-finals` | 140 | 70 | 2.00 | 50% | Sibilants |
| | | | | | → too long | |
| IY | `03-sibilants` | 40 | 80 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| AE | `05-voiced-dentals` | 60 | 120 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| AH | `06-affricates` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| AH | `02-hamlet` | 110 | 60 | 1.83 | 55% | Vowels |
| | | | | | → too long | |
| JH | `06-affricates` | 110 | 60 | 1.83 | 55% | Affricates |
| | | | | | → too long | |
| Tʰ | `02-hamlet` | 50 | 90 | 0.56 | 56% | Other |
| | | | | | → too short | |
| S | `03-sibilants` | 180 | 100 | 1.80 | 56% | Sibilants |
| | | | | | → too long | |
| Z | `05-voiced-dentals` | 160 | 90 | 1.78 | 56% | Sibilants |
| | | | | | → too long | |
| T | `04-dentals` | 70 | 40 | 1.75 | 57% | Stops |
| | | | | | → too long | |
| DH | `08-finals` | 30 | 50 | 0.60 | 60% | Dentals |
| | | | | | → too short | |
