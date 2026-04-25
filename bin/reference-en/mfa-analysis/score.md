# MFA Duration Similarity Score

- Generated: `2026-04-25T20:34:39+02:00`
- Metric: DSS = mean( min(r, 1/r) ) per phoneme pair, r = ours/mary duration
- 100% = perfect timing match with MaryTTS reference

## Per-sample results

| Sample | Text | DSS | Pairs | LLH ours/s | LLH mary/s |
|---|---|---:|---:|---:|---:|
| `01-demo` | Hello from lib-say. This is an English demo sentence. | **68.8%** | 35 | -13.35 | -16.33 |
| `02-hamlet` | To be or not to be, that is the question. | **69.8%** | 23 | -22.74 | -19.96 |
| `03-sibilants` | She sells seashells by the seashore. | **46.1%** | 16 | -26.26 | -26.95 |
| `04-dentals` | This thing is worth the effort. | **63.4%** | 17 | -31.27 | -38.49 |
| `05-voiced-dentals` | Those feathers gather there. | **57.9%** | 14 | -28.43 | -36.12 |
| `06-affricates` | Judge the changing church bells. | **74.1%** | 19 | -29.34 | -33.44 |
| `07-stress` | English fricatives shift sharply. | **62.9%** | 24 | -21.10 | -24.96 |
| `08-finals` | Leave these clothes and bags. | **58.8%** | 17 | -20.75 | -35.54 |

**Global DSS: 62.7%**

## Per-sample phoneme pairs

### `01-demo` — Hello from lib-say. This is an English demo sentence.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| HH | 80 | 30 | 2.67 | 38% ⚠ | Other |
| AH | 40 | 40 | 1.00 | 100% | Vowels |
| L | 70 | 50 | 1.40 | 71% | Liquids |
| OW | 160 | 130 | 1.23 | 81% | Vowels |
| F | 90 | 120 | 0.75 | 75% | Other |
| R | 40 | 30 | 1.33 | 75% | Liquids |
| AH | 110 | 40 | 2.75 | 36% ⚠ | Vowels |
| M | 110 | 60 | 1.83 | 55% | Nasals |
| SPN | 230 | 190 | 1.21 | 83% | Other |
| S | 110 | 130 | 0.85 | 85% | Sibilants |
| EH | 190 | 340 | 0.56 | 56% | Vowels |
| DH | 30 | 30 | 1.00 | 100% | Dentals |
| IH | 50 | 80 | 0.62 | 62% | Vowels |
| S | 30 | 90 | 0.33 | 33% ⚠ | Sibilants |
| IY | 30 | 90 | 0.33 | 33% ⚠ | Vowels |
| Z | 50 | 60 | 0.83 | 83% | Sibilants |
| AH | 60 | 60 | 1.00 | 100% | Vowels |
| N | 40 | 70 | 0.57 | 57% | Nasals |
| EH | 30 | 90 | 0.33 | 33% ⚠ | Vowels |
| N | 160 | 100 | 1.60 | 62% | Nasals |
| JH | 90 | 30 | 3.00 | 33% ⚠ | Affricates |
| L | 80 | 80 | 1.00 | 100% | Liquids |
| IY | 110 | 40 | 2.75 | 36% ⚠ | Vowels |
| SH | 110 | 70 | 1.57 | 64% | Sibilants |
| D | 70 | 80 | 0.87 | 87% | Stops |
| EH | 110 | 80 | 1.37 | 73% | Vowels |
| M | 80 | 70 | 1.14 | 87% | Nasals |
| OW | 120 | 150 | 0.80 | 80% | Vowels |
| S | 90 | 110 | 0.82 | 82% | Sibilants |
| EH | 120 | 90 | 1.33 | 75% | Vowels |
| N | 110 | 60 | 1.83 | 55% | Nasals |
| T | 80 | 60 | 1.33 | 75% | Stops |
| AH | 50 | 70 | 0.71 | 71% | Vowels |
| N | 90 | 90 | 1.00 | 100% | Nasals |
| S | 120 | 170 | 0.71 | 71% | Sibilants |
| **DSS** | | | | **68.8%** | |

### `02-hamlet` — To be or not to be, that is the question.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| AH | 80 | 80 | 1.00 | 100% | Vowels |
| Bʲ | 60 | 60 | 1.00 | 100% | Other |
| IY | 70 | 110 | 0.64 | 64% | Vowels |
| Ɒ | 90 | 80 | 1.13 | 89% | Other |
| R | 60 | 60 | 1.00 | 100% | Liquids |
| N | 140 | 70 | 2.00 | 50% | Nasals |
| AH | 140 | 150 | 0.93 | 93% | Vowels |
| T | 30 | 130 | 0.23 | 23% ⚠ | Stops |
| AH | 80 | 60 | 1.33 | 75% | Vowels |
| Bʲ | 70 | 50 | 1.40 | 71% | Other |
| IY | 70 | 300 | 0.23 | 23% ⚠ | Vowels |
| AE | 150 | 120 | 1.25 | 80% | Vowels |
| T | 80 | 50 | 1.60 | 62% | Stops |
| IY | 90 | 60 | 1.50 | 67% | Vowels |
| Z | 70 | 30 | 2.33 | 43% ⚠ | Sibilants |
| DH | 60 | 90 | 0.67 | 67% | Dentals |
| AH | 30 | 40 | 0.75 | 75% | Vowels |
| Cʷ | 80 | 110 | 0.73 | 73% | Other |
| EH | 210 | 110 | 1.91 | 52% | Vowels |
| S | 120 | 90 | 1.33 | 75% | Sibilants |
| Ʈ | 120 | 130 | 0.92 | 92% | Other |
| AH | 40 | 70 | 0.57 | 57% | Vowels |
| N | 110 | 150 | 0.73 | 73% | Nasals |
| **DSS** | | | | **69.8%** | |

### `03-sibilants` — She sells seashells by the seashore.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| SH | 30 | 150 | 0.20 | 20% ⚠ | Sibilants |
| IY | 30 | 70 | 0.43 | 43% ⚠ | Vowels |
| S | 50 | 110 | 0.45 | 45% ⚠ | Sibilants |
| EH | 30 | 110 | 0.27 | 27% ⚠ | Vowels |
| L | 70 | 70 | 1.00 | 100% | Liquids |
| Z | 30 | 100 | 0.30 | 30% ⚠ | Sibilants |
| SPN | 1500 | 480 | 3.12 | 32% ⚠ | Other |
| B | 40 | 80 | 0.50 | 50% | Stops |
| AY | 40 | 130 | 0.31 | 31% ⚠ | Vowels |
| DH | 60 | 40 | 1.50 | 67% | Dentals |
| AH | 30 | 40 | 0.75 | 75% | Vowels |
| S | 30 | 120 | 0.25 | 25% ⚠ | Sibilants |
| IY | 30 | 120 | 0.25 | 25% ⚠ | Vowels |
| SH | 60 | 110 | 0.55 | 55% | Sibilants |
| Ɒ | 70 | 110 | 0.64 | 64% | Other |
| R | 110 | 220 | 0.50 | 50% ⚠ | Liquids |
| **DSS** | | | | **46.1%** | |

### `04-dentals` — This thing is worth the effort.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| DH | 30 | 50 | 0.60 | 60% | Dentals |
| IH | 50 | 80 | 0.62 | 62% | Vowels |
| S | 30 | 80 | 0.38 | 38% ⚠ | Sibilants |
| T̪ | 240 | 100 | 2.40 | 42% ⚠ | Other |
| IH | 30 | 80 | 0.38 | 38% ⚠ | Vowels |
| NG | 140 | 90 | 1.56 | 64% | Nasals |
| IY | 110 | 70 | 1.57 | 64% | Vowels |
| Z | 30 | 40 | 0.75 | 75% | Sibilants |
| Ʋ | 70 | 70 | 1.00 | 100% | Other |
| ER | 70 | 130 | 0.54 | 54% | Vowels |
| T̪ | 190 | 30 | 6.33 | 16% ⚠ | Other |
| DH | 60 | 40 | 1.50 | 67% | Dentals |
| AH | 30 | 40 | 0.75 | 75% | Vowels |
| EH | 130 | 150 | 0.87 | 87% | Vowels |
| F | 120 | 120 | 1.00 | 100% | Other |
| AO | 40 | 40 | 1.00 | 100% | Vowels |
| T | 80 | 30 | 2.67 | 38% ⚠ | Stops |
| **DSS** | | | | **63.4%** | |

### `05-voiced-dentals` — Those feathers gather there.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| DH | 110 | 60 | 1.83 | 55% | Dentals |
| OW | 160 | 120 | 1.33 | 75% | Vowels |
| Z | 50 | 80 | 0.62 | 62% | Sibilants |
| F | 180 | 110 | 1.64 | 61% | Other |
| EH | 100 | 110 | 0.91 | 91% | Vowels |
| DH | 220 | 50 | 4.40 | 23% ⚠ | Dentals |
| AH | 30 | 100 | 0.30 | 30% ⚠ | Vowels |
| Z | 70 | 90 | 0.78 | 78% | Sibilants |
| G | 70 | 80 | 0.87 | 87% | Stops |
| AH | 110 | 110 | 1.00 | 100% | Vowels |
| DH | 30 | 50 | 0.60 | 60% | Dentals |
| AH | 30 | 80 | 0.38 | 38% ⚠ | Vowels |
| DH | 240 | 40 | 6.00 | 17% ⚠ | Dentals |
| ER | 110 | 320 | 0.34 | 34% ⚠ | Vowels |
| **DSS** | | | | **57.9%** | |

### `06-affricates` — Judge the changing church bells.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| JH | 50 | 80 | 0.62 | 62% | Affricates |
| AH | 90 | 100 | 0.90 | 90% | Vowels |
| JH | 160 | 110 | 1.45 | 69% | Affricates |
| DH | 50 | 50 | 1.00 | 100% | Dentals |
| AH | 30 | 60 | 0.50 | 50% | Vowels |
| CH | 90 | 130 | 0.69 | 69% | Affricates |
| EH | 70 | 120 | 0.58 | 58% | Vowels |
| N | 60 | 80 | 0.75 | 75% | Nasals |
| JH | 30 | 60 | 0.50 | 50% | Affricates |
| IY | 30 | 80 | 0.38 | 38% ⚠ | Vowels |
| NG | 10 | 10 | 1.00 | 100% | Nasals |
| G | 80 | 80 | 1.00 | 100% | Stops |
| CH | 110 | 100 | 1.10 | 91% | Affricates |
| Aː | 130 | 110 | 1.18 | 85% | Other |
| CH | 150 | 160 | 0.94 | 94% | Affricates |
| B | 30 | 70 | 0.43 | 43% ⚠ | Stops |
| EH | 110 | 140 | 0.79 | 79% | Vowels |
| L | 90 | 120 | 0.75 | 75% | Liquids |
| Z | 120 | 150 | 0.80 | 80% | Sibilants |
| **DSS** | | | | **74.1%** | |

### `07-stress` — English fricatives shift sharply.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| EH | 30 | 110 | 0.27 | 27% ⚠ | Vowels |
| N | 140 | 110 | 1.27 | 79% | Nasals |
| JH | 90 | 30 | 3.00 | 33% ⚠ | Affricates |
| L | 80 | 60 | 1.33 | 75% | Liquids |
| IY | 100 | 50 | 2.00 | 50% | Vowels |
| SH | 30 | 80 | 0.37 | 37% ⚠ | Sibilants |
| F | 190 | 100 | 1.90 | 53% | Other |
| R | 140 | 50 | 2.80 | 36% ⚠ | Liquids |
| IY | 60 | 80 | 0.75 | 75% | Vowels |
| K | 70 | 60 | 1.17 | 86% | Stops |
| AH | 70 | 40 | 1.75 | 57% | Vowels |
| Tʲ | 80 | 40 | 2.00 | 50% | Other |
| IY | 70 | 70 | 1.00 | 100% | Vowels |
| V | 50 | 70 | 0.71 | 71% | VoicedFric |
| Z | 40 | 70 | 0.57 | 57% | Sibilants |
| SH | 240 | 110 | 2.18 | 46% ⚠ | Sibilants |
| IY | 90 | 60 | 1.50 | 67% | Vowels |
| F | 120 | 90 | 1.33 | 75% | Other |
| T | 70 | 40 | 1.75 | 57% | Stops |
| SH | 110 | 130 | 0.85 | 85% | Sibilants |
| AH | 170 | 150 | 1.13 | 88% | Vowels |
| P | 130 | 90 | 1.44 | 69% | Stops |
| L | 70 | 60 | 1.17 | 86% | Liquids |
| IY | 110 | 220 | 0.50 | 50% ⚠ | Vowels |
| **DSS** | | | | **62.9%** | |

### `08-finals` — Leave these clothes and bags.

| ARPA | ours (ms) | mary (ms) | ratio | sim | family |
|---|---:|---:|---:|---:|---|
| L | 150 | 70 | 2.14 | 47% ⚠ | Liquids |
| IY | 30 | 90 | 0.33 | 33% ⚠ | Vowels |
| V | 180 | 50 | 3.60 | 28% ⚠ | VoicedFric |
| DH | 30 | 40 | 0.75 | 75% | Dentals |
| IY | 80 | 120 | 0.67 | 67% | Vowels |
| Z | 140 | 80 | 1.75 | 57% | Sibilants |
| K | 30 | 110 | 0.27 | 27% ⚠ | Stops |
| L | 60 | 30 | 2.00 | 50% ⚠ | Liquids |
| OW | 210 | 140 | 1.50 | 67% | Vowels |
| DH | 110 | 60 | 1.83 | 55% | Dentals |
| Z | 140 | 80 | 1.75 | 57% | Sibilants |
| AH | 90 | 80 | 1.13 | 89% | Vowels |
| N | 90 | 80 | 1.12 | 89% | Nasals |
| B | 170 | 90 | 1.89 | 53% | Stops |
| AH | 150 | 210 | 0.71 | 71% | Vowels |
| G | 80 | 90 | 0.89 | 89% | Stops |
| Z | 100 | 215 | 0.47 | 47% ⚠ | Sibilants |
| **DSS** | | | | **58.8%** | |

## Family summary

| Family | N | Mean sim | Mean ratio |
|---|---:|---:|---:|
| Vowels | 59 | 63.2% | 0.90 |
| Sibilants | 25 | 57.2% | 0.87 |
| Stops | 15 | 63.1% | 1.13 |
| Nasals | 13 | 72.8% | 1.26 |
| Liquids | 12 | 72.0% | 1.37 |
| Dentals | 12 | 62.0% | 1.81 |
| VoicedFric | 2 | 49.6% | 2.16 |
| Affricates | 8 | 62.7% | 1.41 |
| Other | 19 | 68.3% | 1.70 |

## Improvement leads (sim < 60%)

| ARPA | Sample | ours (ms) | mary (ms) | ratio | sim | family |
|---|---|---:|---:|---:|---:|---|
| T̪ | `04-dentals` | 190 | 30 | 6.33 | 16% | Other |
| | | | | | → too long | |
| DH | `05-voiced-dentals` | 240 | 40 | 6.00 | 17% | Dentals |
| | | | | | → too long | |
| SH | `03-sibilants` | 30 | 150 | 0.20 | 20% | Sibilants |
| | | | | | → too short | |
| DH | `05-voiced-dentals` | 220 | 50 | 4.40 | 23% | Dentals |
| | | | | | → too long | |
| T | `02-hamlet` | 30 | 130 | 0.23 | 23% | Stops |
| | | | | | → too short | |
| IY | `02-hamlet` | 70 | 300 | 0.23 | 23% | Vowels |
| | | | | | → too short | |
| IY | `03-sibilants` | 30 | 120 | 0.25 | 25% | Vowels |
| | | | | | → too short | |
| S | `03-sibilants` | 30 | 120 | 0.25 | 25% | Sibilants |
| | | | | | → too short | |
| K | `08-finals` | 30 | 110 | 0.27 | 27% | Stops |
| | | | | | → too short | |
| EH | `07-stress` | 30 | 110 | 0.27 | 27% | Vowels |
| | | | | | → too short | |
| EH | `03-sibilants` | 30 | 110 | 0.27 | 27% | Vowels |
| | | | | | → too short | |
| V | `08-finals` | 180 | 50 | 3.60 | 28% | VoicedFric |
| | | | | | → too long | |
| Z | `03-sibilants` | 30 | 100 | 0.30 | 30% | Sibilants |
| | | | | | → too short | |
| AH | `05-voiced-dentals` | 30 | 100 | 0.30 | 30% | Vowels |
| | | | | | → too short | |
| AY | `03-sibilants` | 40 | 130 | 0.31 | 31% | Vowels |
| | | | | | → too short | |
| SPN | `03-sibilants` | 1500 | 480 | 3.12 | 32% | Other |
| | | | | | → too long | |
| S | `01-demo` | 30 | 90 | 0.33 | 33% | Sibilants |
| | | | | | → too short | |
| EH | `01-demo` | 30 | 90 | 0.33 | 33% | Vowels |
| | | | | | → too short | |
| JH | `07-stress` | 90 | 30 | 3.00 | 33% | Affricates |
| | | | | | → too long | |
| IY | `08-finals` | 30 | 90 | 0.33 | 33% | Vowels |
| | | | | | → too short | |
| IY | `01-demo` | 30 | 90 | 0.33 | 33% | Vowels |
| | | | | | → too short | |
| JH | `01-demo` | 90 | 30 | 3.00 | 33% | Affricates |
| | | | | | → too long | |
| ER | `05-voiced-dentals` | 110 | 320 | 0.34 | 34% | Vowels |
| | | | | | → too short | |
| R | `07-stress` | 140 | 50 | 2.80 | 36% | Liquids |
| | | | | | → too long | |
| IY | `01-demo` | 110 | 40 | 2.75 | 36% | Vowels |
| | | | | | → too long | |
| AH | `01-demo` | 110 | 40 | 2.75 | 36% | Vowels |
| | | | | | → too long | |
| SH | `07-stress` | 30 | 80 | 0.37 | 37% | Sibilants |
| | | | | | → too short | |
| HH | `01-demo` | 80 | 30 | 2.67 | 38% | Other |
| | | | | | → too long | |
| T | `04-dentals` | 80 | 30 | 2.67 | 38% | Stops |
| | | | | | → too long | |
| AH | `05-voiced-dentals` | 30 | 80 | 0.38 | 38% | Vowels |
| | | | | | → too short | |
| S | `04-dentals` | 30 | 80 | 0.38 | 38% | Sibilants |
| | | | | | → too short | |
| IH | `04-dentals` | 30 | 80 | 0.38 | 38% | Vowels |
| | | | | | → too short | |
| IY | `06-affricates` | 30 | 80 | 0.38 | 38% | Vowels |
| | | | | | → too short | |
| T̪ | `04-dentals` | 240 | 100 | 2.40 | 42% | Other |
| | | | | | → too long | |
| IY | `03-sibilants` | 30 | 70 | 0.43 | 43% | Vowels |
| | | | | | → too short | |
| Z | `02-hamlet` | 70 | 30 | 2.33 | 43% | Sibilants |
| | | | | | → too long | |
| B | `06-affricates` | 30 | 70 | 0.43 | 43% | Stops |
| | | | | | → too short | |
| S | `03-sibilants` | 50 | 110 | 0.45 | 45% | Sibilants |
| | | | | | → too short | |
| SH | `07-stress` | 240 | 110 | 2.18 | 46% | Sibilants |
| | | | | | → too long | |
| Z | `08-finals` | 100 | 215 | 0.47 | 47% | Sibilants |
| | | | | | → too short | |
| L | `08-finals` | 150 | 70 | 2.14 | 47% | Liquids |
| | | | | | → too long | |
| L | `08-finals` | 60 | 30 | 2.00 | 50% | Liquids |
| | | | | | → too long | |
| R | `03-sibilants` | 110 | 220 | 0.50 | 50% | Liquids |
| | | | | | → too short | |
| IY | `07-stress` | 110 | 220 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| N | `02-hamlet` | 140 | 70 | 2.00 | 50% | Nasals |
| | | | | | → too long | |
| JH | `06-affricates` | 30 | 60 | 0.50 | 50% | Affricates |
| | | | | | → too short | |
| IY | `07-stress` | 100 | 50 | 2.00 | 50% | Vowels |
| | | | | | → too long | |
| Tʲ | `07-stress` | 80 | 40 | 2.00 | 50% | Other |
| | | | | | → too long | |
| AH | `06-affricates` | 30 | 60 | 0.50 | 50% | Vowels |
| | | | | | → too short | |
| B | `03-sibilants` | 40 | 80 | 0.50 | 50% | Stops |
| | | | | | → too short | |
| EH | `02-hamlet` | 210 | 110 | 1.91 | 52% | Vowels |
| | | | | | → too long | |
| F | `07-stress` | 190 | 100 | 1.90 | 53% | Other |
| | | | | | → too long | |
| B | `08-finals` | 170 | 90 | 1.89 | 53% | Stops |
| | | | | | → too long | |
| ER | `04-dentals` | 70 | 130 | 0.54 | 54% | Vowels |
| | | | | | → too short | |
| SH | `03-sibilants` | 60 | 110 | 0.55 | 55% | Sibilants |
| | | | | | → too short | |
| DH | `05-voiced-dentals` | 110 | 60 | 1.83 | 55% | Dentals |
| | | | | | → too long | |
| DH | `08-finals` | 110 | 60 | 1.83 | 55% | Dentals |
| | | | | | → too long | |
| M | `01-demo` | 110 | 60 | 1.83 | 55% | Nasals |
| | | | | | → too long | |
| N | `01-demo` | 110 | 60 | 1.83 | 55% | Nasals |
| | | | | | → too long | |
| EH | `01-demo` | 190 | 340 | 0.56 | 56% | Vowels |
| | | | | | → too short | |
| AH | `02-hamlet` | 40 | 70 | 0.57 | 57% | Vowels |
| | | | | | → too short | |
| N | `01-demo` | 40 | 70 | 0.57 | 57% | Nasals |
| | | | | | → too short | |
| T | `07-stress` | 70 | 40 | 1.75 | 57% | Stops |
| | | | | | → too long | |
| Z | `07-stress` | 40 | 70 | 0.57 | 57% | Sibilants |
| | | | | | → too short | |
| Z | `08-finals` | 140 | 80 | 1.75 | 57% | Sibilants |
| | | | | | → too long | |
| Z | `08-finals` | 140 | 80 | 1.75 | 57% | Sibilants |
| | | | | | → too long | |
| AH | `07-stress` | 70 | 40 | 1.75 | 57% | Vowels |
| | | | | | → too long | |
| EH | `06-affricates` | 70 | 120 | 0.58 | 58% | Vowels |
| | | | | | → too short | |
| DH | `05-voiced-dentals` | 30 | 50 | 0.60 | 60% | Dentals |
| | | | | | → too short | |
