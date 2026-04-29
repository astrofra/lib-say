# MFA Duration Similarity Score

- Generated: `2026-04-29T09:30:51+02:00`
- Metric: DSS = mean( min(r, 1/r) ) per phoneme pair, r = ours/mary duration
- 100% = perfect timing match with MaryTTS reference

## Per-sample results

| Sample | Text | DSS | Pairs | LLH ours/s | LLH mary/s |
|---|---|---:|---:|---:|---:|
| `01-demo` | Hello from lib-say. This is an English demo sentence. | **71.9%** | 37 | -13.40 | -15.92 |
| `02-hamlet` | To be or not to be, that is the question. | **63.5%** | 24 | -22.84 | -21.07 |
| `03-sibilants` | She sells seashells by the seashore. | **76.3%** | 21 | -24.85 | -23.79 |
| `04-dentals` | This thing is worth the effort. | **57.7%** | 16 | -34.17 | -35.09 |
| `05-voiced-dentals` | Those feathers gather there. | **54.7%** | 15 | -29.22 | -32.67 |
| `06-affricates` | Judge the changing church bells. | **59.2%** | 18 | -27.09 | -34.15 |
| `07-stress` | English fricatives shift sharply. | **57.7%** | 17 | -21.77 | -26.00 |
| `08-finals` | Leave these clothes and bags. | **58.3%** | 16 | -21.14 | -34.88 |

**Global DSS: 62.4%**

## Per-sample phoneme pairs

### `01-demo` — Hello from lib-say. This is an English demo sentence.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| HH | 80 | 30 | 2.67 | 38% ⚠ | Other |
| AH | 40 | 30 | 1.33 | 75% | Vowels |
| L | 60 | 70 | 0.86 | 86% | Liquids |
| OW | 160 | 130 | 1.23 | 81% | Vowels |
| F | 40 | 100 | 0.40 | 40% ⚠ | Other |
| R | 80 | 40 | 2.00 | 50% | Liquids |
| AH | 70 | 40 | 1.75 | 57% | Vowels |
| M | 140 | 60 | 2.33 | 43% ⚠ | Nasals |
| L | 80 | 70 | 1.14 | 87% | Liquids |
| IH | 30 | 60 | 0.50 | 50% ⚠ | Vowels |
| B | 80 | 60 | 1.33 | 75% | Stops |
| S | 100 | 130 | 0.77 | 77% | Sibilants |
| EJ | 220 | 330 | 0.67 | 67% | Other |
| DH | 30 | 30 | 1.00 | 100% | Dentals |
| IH | 70 | 80 | 0.87 | 87% | Vowels |
| S | 120 | 90 | 1.33 | 75% | Sibilants |
| IH | 40 | 90 | 0.44 | 44% ⚠ | Vowels |
| Z | 120 | 60 | 2.00 | 50% | Sibilants |
| AH | 50 | 60 | 0.83 | 83% | Vowels |
| N | 60 | 60 | 1.00 | 100% | Nasals |
| IH | 90 | 110 | 0.82 | 82% | Vowels |
| N | 80 | 90 | 0.89 | 89% | Nasals |
| JH | 90 | 30 | 3.00 | 33% ⚠ | Affricates |
| L | 70 | 60 | 1.17 | 86% | Liquids |
| IH | 120 | 60 | 2.00 | 50% | Vowels |
| SH | 120 | 100 | 1.20 | 83% | Sibilants |
| D | 60 | 50 | 1.20 | 83% | Stops |
| EH | 100 | 80 | 1.25 | 80% | Vowels |
| M | 70 | 70 | 1.00 | 100% | Nasals |
| OW | 130 | 150 | 0.87 | 87% | Vowels |
| S | 70 | 110 | 0.64 | 64% | Sibilants |
| EH | 100 | 90 | 1.11 | 90% | Vowels |
| N | 100 | 60 | 1.67 | 60% | Nasals |
| T | 80 | 60 | 1.33 | 75% | Stops |
| AH | 60 | 70 | 0.86 | 86% | Vowels |
| N | 80 | 110 | 0.73 | 73% | Nasals |
| S | 110 | 150 | 0.73 | 73% | Sibilants |
| **DSS** | | | | **71.9%** | |

### `02-hamlet` — To be or not to be, that is the question.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| Tʰ | 50 | 90 | 0.56 | 56% | Other |
| Ʉː | 70 | 60 | 1.17 | 86% | Other |
| Bʲ | 60 | 60 | 1.00 | 100% | Other |
| IY | 70 | 120 | 0.58 | 58% | Vowels |
| Ɒ | 70 | 70 | 1.00 | 100% | Other |
| R | 70 | 60 | 1.17 | 86% | Liquids |
| N | 130 | 70 | 1.86 | 54% | Nasals |
| AA | 130 | 140 | 0.93 | 93% | Vowels |
| T | 40 | 50 | 0.80 | 80% | Stops |
| Tʰ | 100 | 90 | 1.11 | 90% | Other |
| Ʉː | 70 | 60 | 1.17 | 86% | Other |
| Bʲ | 60 | 50 | 1.20 | 83% | Other |
| IY | 90 | 290 | 0.31 | 31% ⚠ | Vowels |
| AE | 120 | 40 | 3.00 | 33% ⚠ | Vowels |
| T | 100 | 90 | 1.11 | 90% | Stops |
| IH | 40 | 50 | 0.80 | 80% | Vowels |
| Z | 30 | 80 | 0.38 | 38% ⚠ | Sibilants |
| AH | 230 | 40 | 5.75 | 17% ⚠ | Vowels |
| Cʷ | 30 | 60 | 0.50 | 50% | Other |
| EH | 10 | 40 | 0.25 | 25% ⚠ | Vowels |
| S | 30 | 110 | 0.27 | 27% ⚠ | Sibilants |
| CH | 40 | 110 | 0.36 | 36% ⚠ | Affricates |
| AH | 50 | 90 | 0.56 | 56% | Vowels |
| N | 90 | 130 | 0.69 | 69% | Nasals |
| **DSS** | | | | **63.5%** | |

### `03-sibilants` — She sells seashells by the seashore.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| SH | 100 | 140 | 0.71 | 71% | Sibilants |
| IY | 40 | 80 | 0.50 | 50% | Vowels |
| S | 90 | 120 | 0.75 | 75% | Sibilants |
| EH | 90 | 70 | 1.29 | 78% | Vowels |
| Ɫ | 90 | 90 | 1.00 | 100% | Other |
| Z | 40 | 40 | 1.00 | 100% | Sibilants |
| S | 180 | 90 | 2.00 | 50% | Sibilants |
| IY | 110 | 120 | 0.92 | 92% | Vowels |
| SH | 120 | 100 | 1.20 | 83% | Sibilants |
| EH | 70 | 80 | 0.87 | 87% | Vowels |
| Ɫ | 90 | 80 | 1.13 | 89% | Other |
| Z | 120 | 60 | 2.00 | 50% | Sibilants |
| B | 70 | 100 | 0.70 | 70% | Stops |
| AY | 180 | 130 | 1.38 | 72% | Vowels |
| DH | 40 | 40 | 1.00 | 100% | Dentals |
| AH | 30 | 50 | 0.60 | 60% | Vowels |
| S | 120 | 110 | 1.09 | 92% | Sibilants |
| IY | 120 | 120 | 1.00 | 100% | Vowels |
| SH | 110 | 110 | 1.00 | 100% | Sibilants |
| Ɒ | 70 | 110 | 0.64 | 64% | Other |
| R | 40 | 210 | 0.19 | 19% ⚠ | Liquids |
| **DSS** | | | | **76.3%** | |

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
| ɝ | 80 | 60 | 1.33 | 75% | Other |
| TH | 150 | 130 | 1.15 | 87% | Dentals |
| DH | 70 | 30 | 2.33 | 43% ⚠ | Dentals |
| AH | 110 | 40 | 2.75 | 36% ⚠ | Vowels |
| EH | 30 | 60 | 0.50 | 50% | Vowels |
| F | 120 | 150 | 0.80 | 80% | Other |
| ER | 30 | 110 | 0.27 | 27% ⚠ | Vowels |
| T | 30 | 40 | 0.75 | 75% | Stops |
| **DSS** | | | | **57.7%** | |

### `05-voiced-dentals` — Those feathers gather there.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| DH | 100 | 60 | 1.67 | 60% | Dentals |
| OW | 180 | 120 | 1.50 | 67% | Vowels |
| Z | 30 | 80 | 0.37 | 37% ⚠ | Sibilants |
| F | 110 | 110 | 1.00 | 100% | Other |
| EH | 100 | 110 | 0.91 | 91% | Vowels |
| DH | 130 | 50 | 2.60 | 38% ⚠ | Dentals |
| ER | 30 | 100 | 0.30 | 30% ⚠ | Vowels |
| Z | 160 | 90 | 1.78 | 56% | Sibilants |
| G | 60 | 70 | 0.86 | 86% | Stops |
| AE | 130 | 120 | 1.08 | 92% | Vowels |
| DH | 130 | 50 | 2.60 | 38% ⚠ | Dentals |
| ER | 30 | 80 | 0.38 | 38% ⚠ | Vowels |
| DH | 150 | 40 | 3.75 | 27% ⚠ | Dentals |
| EH | 60 | 140 | 0.43 | 43% ⚠ | Vowels |
| R | 30 | 170 | 0.18 | 18% ⚠ | Liquids |
| **DSS** | | | | **54.7%** | |

### `06-affricates` — Judge the changing church bells.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| JH | 40 | 80 | 0.50 | 50% | Affricates |
| AH | 70 | 100 | 0.70 | 70% | Vowels |
| JH | 30 | 80 | 0.37 | 37% ⚠ | Affricates |
| DH | 150 | 80 | 1.87 | 53% | Dentals |
| AH | 30 | 60 | 0.50 | 50% | Vowels |
| CH | 150 | 130 | 1.15 | 87% | Affricates |
| EJ | 30 | 120 | 0.25 | 25% ⚠ | Other |
| N | 120 | 80 | 1.50 | 67% | Nasals |
| JH | 30 | 60 | 0.50 | 50% | Affricates |
| IH | 30 | 70 | 0.43 | 43% ⚠ | Vowels |
| NG | 30 | 80 | 0.37 | 37% ⚠ | Nasals |
| CH | 110 | 120 | 0.92 | 92% | Affricates |
| ɝ | 100 | 110 | 0.91 | 91% | Other |
| CH | 30 | 130 | 0.23 | 23% ⚠ | Affricates |
| B | 30 | 40 | 0.75 | 75% | Stops |
| EH | 100 | 100 | 1.00 | 100% | Vowels |
| Ɫ | 70 | 160 | 0.44 | 44% ⚠ | Other |
| Z | 130 | 180 | 0.72 | 72% | Sibilants |
| **DSS** | | | | **59.2%** | |

### `07-stress` — English fricatives shift sharply.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| IH | 30 | 70 | 0.43 | 43% ⚠ | Vowels |
| N | 30 | 150 | 0.20 | 20% ⚠ | Nasals |
| JH | 40 | 30 | 1.33 | 75% | Affricates |
| L | 30 | 50 | 0.60 | 60% | Liquids |
| IH | 30 | 60 | 0.50 | 50% | Vowels |
| SH | 30 | 40 | 0.75 | 75% | Sibilants |
| SPN | 1390 | 600 | 2.32 | 43% ⚠ | Other |
| SH | 50 | 130 | 0.38 | 38% ⚠ | Sibilants |
| IH | 30 | 60 | 0.50 | 50% | Vowels |
| F | 30 | 50 | 0.60 | 60% | Other |
| T | 30 | 80 | 0.38 | 38% ⚠ | Stops |
| SH | 100 | 130 | 0.77 | 77% | Sibilants |
| AA | 80 | 60 | 1.33 | 75% | Vowels |
| R | 60 | 90 | 0.67 | 67% | Liquids |
| Pʲ | 130 | 90 | 1.44 | 69% | Other |
| L | 80 | 60 | 1.33 | 75% | Liquids |
| IY | 120 | 180 | 0.67 | 67% | Vowels |
| **DSS** | | | | **57.7%** | |

### `08-finals` — Leave these clothes and bags.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| L | 160 | 70 | 2.29 | 44% ⚠ | Liquids |
| IY | 30 | 80 | 0.38 | 38% ⚠ | Vowels |
| V | 170 | 60 | 2.83 | 35% ⚠ | VoicedFric |
| DH | 30 | 40 | 0.75 | 75% | Dentals |
| IY | 80 | 130 | 0.62 | 62% | Vowels |
| Z | 140 | 70 | 2.00 | 50% | Sibilants |
| K | 30 | 110 | 0.27 | 27% ⚠ | Stops |
| L | 90 | 30 | 3.00 | 33% ⚠ | Liquids |
| OW | 220 | 150 | 1.47 | 68% | Vowels |
| DH | 80 | 40 | 2.00 | 50% | Dentals |
| Z | 140 | 90 | 1.56 | 64% | Sibilants |
| AE | 90 | 80 | 1.12 | 89% | Vowels |
| N | 80 | 90 | 0.89 | 89% | Nasals |
| B | 180 | 210 | 0.86 | 86% | Stops |
| AE | 110 | 90 | 1.22 | 82% | Vowels |
| G | 90 | 215 | 0.42 | 42% ⚠ | Stops |
| **DSS** | | | | **58.3%** | |

## Family summary

| Family | N | Mean sim | Mean ratio |
|---|---:|---:|---:|
| Vowels | 53 | 63.0% | 0.99 |
| Sibilants | 26 | 65.5% | 1.03 |
| Stops | 13 | 69.3% | 0.83 |
| Nasals | 13 | 64.9% | 1.19 |
| Liquids | 12 | 59.2% | 1.22 |
| Dentals | 12 | 61.0% | 1.78 |
| VoicedFric | 1 | 35.3% | 2.83 |
| Affricates | 9 | 53.7% | 0.93 |
| Other | 25 | 70.5% | 1.07 |

## Improvement leads (sim < 60%)

| ARPA | Sample | ours (ms) | mary (ms) | ratio | sim | family |
|---|---|---:|---:|---:|---:|---|
| AH | `02-hamlet` | 230 | 40 | 5.75 | 17% | Vowels |
| | | | | | → too long | |
| R | `05-voiced-dentals` | 30 | 170 | 0.18 | 18% | Liquids |
| | | | | | → too short | |
| R | `03-sibilants` | 40 | 210 | 0.19 | 19% | Liquids |
| | | | | | → too short | |
| N | `07-stress` | 30 | 150 | 0.20 | 20% | Nasals |
| | | | | | → too short | |
| CH | `06-affricates` | 30 | 130 | 0.23 | 23% | Affricates |
| | | | | | → too short | |
| EH | `02-hamlet` | 10 | 40 | 0.25 | 25% | Vowels |
| | | | | | → too short | |
| EJ | `06-affricates` | 30 | 120 | 0.25 | 25% | Other |
| | | | | | → too short | |
| DH | `05-voiced-dentals` | 150 | 40 | 3.75 | 27% | Dentals |
| | | | | | → too long | |
| S | `02-hamlet` | 30 | 110 | 0.27 | 27% | Sibilants |
| | | | | | → too short | |
| K | `08-finals` | 30 | 110 | 0.27 | 27% | Stops |
| | | | | | → too short | |
| ER | `04-dentals` | 30 | 110 | 0.27 | 27% | Vowels |
| | | | | | → too short | |
| ER | `05-voiced-dentals` | 30 | 100 | 0.30 | 30% | Vowels |
| | | | | | → too short | |
| IY | `02-hamlet` | 90 | 290 | 0.31 | 31% | Vowels |
| | | | | | → too short | |
| L | `08-finals` | 90 | 30 | 3.00 | 33% | Liquids |
| | | | | | → too long | |
| AE | `02-hamlet` | 120 | 40 | 3.00 | 33% | Vowels |
| | | | | | → too long | |
| JH | `01-demo` | 90 | 30 | 3.00 | 33% | Affricates |
| | | | | | → too long | |
| V | `08-finals` | 170 | 60 | 2.83 | 35% | VoicedFric |
| | | | | | → too long | |
| AH | `04-dentals` | 110 | 40 | 2.75 | 36% | Vowels |
| | | | | | → too long | |
| CH | `02-hamlet` | 40 | 110 | 0.36 | 36% | Affricates |
| | | | | | → too short | |
| NG | `06-affricates` | 30 | 80 | 0.37 | 37% | Nasals |
| | | | | | → too short | |
| Z | `05-voiced-dentals` | 30 | 80 | 0.37 | 37% | Sibilants |
| | | | | | → too short | |
| JH | `06-affricates` | 30 | 80 | 0.37 | 37% | Affricates |
| | | | | | → too short | |
| HH | `01-demo` | 80 | 30 | 2.67 | 38% | Other |
| | | | | | → too long | |
| Z | `02-hamlet` | 30 | 80 | 0.38 | 38% | Sibilants |
| | | | | | → too short | |
| ER | `05-voiced-dentals` | 30 | 80 | 0.38 | 38% | Vowels |
| | | | | | → too short | |
| T | `07-stress` | 30 | 80 | 0.38 | 38% | Stops |
| | | | | | → too short | |
| S | `04-dentals` | 30 | 80 | 0.38 | 38% | Sibilants |
| | | | | | → too short | |
| IY | `08-finals` | 30 | 80 | 0.38 | 38% | Vowels |
| | | | | | → too short | |
| SH | `07-stress` | 50 | 130 | 0.38 | 38% | Sibilants |
| | | | | | → too short | |
| DH | `05-voiced-dentals` | 130 | 50 | 2.60 | 38% | Dentals |
| | | | | | → too long | |
| DH | `05-voiced-dentals` | 130 | 50 | 2.60 | 38% | Dentals |
| | | | | | → too long | |
| F | `01-demo` | 40 | 100 | 0.40 | 40% | Other |
| | | | | | → too short | |
| G | `08-finals` | 90 | 215 | 0.42 | 42% | Stops |
| | | | | | → too short | |
| DH | `04-dentals` | 70 | 30 | 2.33 | 43% | Dentals |
| | | | | | → too long | |
| IH | `04-dentals` | 30 | 70 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| N | `04-dentals` | 210 | 90 | 2.33 | 43% | Nasals |
| | | | | | → too long | |
| IH | `07-stress` | 30 | 70 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| M | `01-demo` | 140 | 60 | 2.33 | 43% | Nasals |
| | | | | | → too long | |
| EH | `05-voiced-dentals` | 60 | 140 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| IH | `06-affricates` | 30 | 70 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| SPN | `07-stress` | 1390 | 600 | 2.32 | 43% | Other |
| | | | | | → too long | |
| T̪ | `04-dentals` | 230 | 100 | 2.30 | 43% | Other |
| | | | | | → too long | |
| Ɫ | `06-affricates` | 70 | 160 | 0.44 | 44% | Other |
| | | | | | → too short | |
| L | `08-finals` | 160 | 70 | 2.29 | 44% | Liquids |
| | | | | | → too long | |
| IH | `01-demo` | 40 | 90 | 0.44 | 44% | Vowels |
| | | | | | → too short | |
| IH | `01-demo` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| IH | `01-demo` | 120 | 60 | 2.00 | 50% | Vowels |
| | | | | | → too long | |
| Z | `01-demo` | 120 | 60 | 2.00 | 50% | Sibilants |
| | | | | | → too long | |
| S | `03-sibilants` | 180 | 90 | 2.00 | 50% | Sibilants |
| | | | | | → too long | |
| EH | `04-dentals` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| JH | `06-affricates` | 30 | 60 | 0.50 | 50% | Affricates |
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
| JH | `06-affricates` | 40 | 80 | 0.50 | 50% | Affricates |
| | | | | | → too short | |
| IY | `03-sibilants` | 40 | 80 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| R | `01-demo` | 80 | 40 | 2.00 | 50% | Liquids |
| | | | | | → too long | |
| AH | `06-affricates` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| Z | `03-sibilants` | 120 | 60 | 2.00 | 50% | Sibilants |
| | | | | | → too long | |
| Cʷ | `02-hamlet` | 30 | 60 | 0.50 | 50% | Other |
| | | | | | → too short | |
| DH | `06-affricates` | 150 | 80 | 1.87 | 53% | Dentals |
| | | | | | → too long | |
| N | `02-hamlet` | 130 | 70 | 1.86 | 54% | Nasals |
| | | | | | → too long | |
| Tʰ | `02-hamlet` | 50 | 90 | 0.56 | 56% | Other |
| | | | | | → too short | |
| AH | `02-hamlet` | 50 | 90 | 0.56 | 56% | Vowels |
| | | | | | → too short | |
| Z | `05-voiced-dentals` | 160 | 90 | 1.78 | 56% | Sibilants |
| | | | | | → too long | |
| AH | `01-demo` | 70 | 40 | 1.75 | 57% | Vowels |
| | | | | | → too long | |
| IY | `02-hamlet` | 70 | 120 | 0.58 | 58% | Vowels |
| | | | | | → too short | |
