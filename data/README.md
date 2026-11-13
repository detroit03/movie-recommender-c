# Ratings data

`sample_ratings.tsv` is a synthetic fixture created for demonstration and tests.
It contains three users and four movie IDs and is not extracted from MovieLens.

For real experiments, obtain MovieLens 100K directly from:

- Dataset: https://grouplens.org/datasets/movielens/100k/
- Usage conditions and citation: https://files.grouplens.org/datasets/movielens/ml-100k/README

Keep the extracted `ml-100k` directory inside `data/movielens/`, which Git ignores.
Pass `data/movielens/ml-100k/u.data` as the program's first command-line argument.
The upstream conditions require separate permission before redistribution.
