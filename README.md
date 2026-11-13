# Movie Recommendation Algorithms in C

A terminal recommendation system using a user–movie bipartite graph and six approaches:

1. Random unseen movies, without duplicates.
2. Highest-degree unseen movies.
3. Recommendations from the user with the greatest watched-movie overlap.
4. Similarity-weighted predicted ratings based on commonly rated movies.
5. K-means clustering of movie rating vectors.
6. Alternating user–movie random walks.

The project uses standard C11 and does not require an external recommendation library.
Menus and console messages are in Turkish. Recommendations display movie IDs rather than titles.

## Build and run

Requirements: GCC (or a compatible C11 compiler), its math library, and Python 3.10+ for helpers/tests.
Run commands from the repository root:

```sh
python scripts/build.py
```

Linux/macOS:

```sh
./build/movie_recommender
```

Windows PowerShell:

```powershell
.\build\movie_recommender.exe
```

With no argument, the program uses `data/sample_ratings.tsv`. Enter user ID **1** to try the demo.
This file is a tiny **synthetic example**, not a subset of MovieLens.
It demonstrates behavior; it does not support meaningful recommendation-quality evaluation.

## Use the real MovieLens data

The original project was used with **MovieLens 100K**: 100,000 ratings from 943 users on 1,682 movies.
The raw dataset is not bundled. Download it from the [official GroupLens page](https://grouplens.org/datasets/movielens/100k/),
read its [usage conditions](https://files.grouplens.org/datasets/movielens/ml-100k/README), and extract it locally.

Place the extracted dataset at `data/movielens/ml-100k/`, then run:

```powershell
.\build\movie_recommender.exe .\data\movielens\ml-100k\u.data
```

On Linux/macOS:

```sh
./build/movie_recommender data/movielens/ml-100k/u.data
```

Any local path is accepted. For example, a local `u.data` beside the executable can be passed explicitly.
The `data/movielens/` directory and raw `u.data` files are ignored by Git.
GroupLens requires separate permission for redistribution; link to its download instead of committing the dataset.

## Input format

Each whitespace-separated line contains:

```text
user_id movie_id rating timestamp
```

Each nonblank line must contain exactly these four bounded integers. Missing or extra fields,
values outside the C `int` range, and lines longer than 1022 characters are rejected.
Blank lines are ignored; fields are never combined across two records.

IDs must be positive and ratings must be integers from 1 to 5. Timestamps are read but do not
affect recommendations. Duplicate user/movie pairs overwrite the previous rating.
Unrated entries are represented internally as zero. The dense matrix is allocated to the observed
dimensions, with up to 10,000 distinct users and movies each; memory and clustering cost still grow
with dataset size.

## Layout

```text
src/main.c                Graph loading, recommendation methods and terminal menu
data/sample_ratings.tsv   Synthetic demo fixture
data/README.md            Dataset provenance and local placement
scripts/build.py          Portable build helper
tests/check.py            All six methods and invalid-input checks
```

## Tests

```sh
python -B tests/check.py
```

Tests compile with warnings treated as errors and check all six methods, unseen-movie filtering,
uniqueness, ranking on a small example, malformed files and termination when every movie is already seen.
Tests run offline using synthetic data. GitHub Actions runs the same checks.

## Dataset credit

F. Maxwell Harper and Joseph A. Konstan. 2015. *The MovieLens Datasets: History and Context.*
ACM Transactions on Interactive Intelligent Systems 5(4), Article 19.
[doi:10.1145/2827872](https://doi.org/10.1145/2827872).
