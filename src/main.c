#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#define MAX_RECORDS 10000
#define MAX_STEPS 10
#define INF DBL_MAX

typedef struct {
    int *users;
    int *items;
    int **adjMatrix;
    int numUsers;
    int numItems;
} BipartiteGraph;


static void *allocate(size_t count, size_t size) {
    void *ptr = calloc(count ? count : 1, size);
    if (!ptr) { fprintf(stderr, "Bellek ayrilamadi.\n"); exit(EXIT_FAILURE); }
    return ptr;
}

void freeGraph(BipartiteGraph *graph) {
    for (int i = 0; i < graph->numUsers; ++i) free(graph->adjMatrix[i]);
    free(graph->adjMatrix); free(graph->users); free(graph->items);
    memset(graph, 0, sizeof(*graph));
}

/* Read exactly four bounded integers on one line; never join adjacent records. */
static int readRating(FILE *file, int *userId, int *itemId, int *rating, int *timestamp) {
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        if (!strchr(line, '\n') && !feof(file)) return -1;
        char *cursor = line;
        while (isspace((unsigned char)*cursor)) ++cursor;
        if (!*cursor) continue;
        int *fields[] = {userId, itemId, rating, timestamp};
        for (int i = 0; i < 4; ++i) {
            char *end;
            errno = 0;
            long value = strtol(cursor, &end, 10);
            if (end == cursor || errno || value < INT_MIN || value > INT_MAX) return -1;
            if (*end && !isspace((unsigned char)*end)) return -1;
            *fields[i] = (int)value;
            cursor = end;
            while (isspace((unsigned char)*cursor)) ++cursor;
        }
        return *cursor ? -1 : 1;
    }
    return ferror(file) ? -1 : 0;
}

int createBipartiteGraph(const char *filename, BipartiteGraph *graph) {
    memset(graph, 0, sizeof(*graph));
    FILE *file = fopen(filename, "r");
    if (!file) { fprintf(stderr, "Dosya acilamiyor: %s\n", filename); return 0; }
    graph->users = allocate(MAX_RECORDS, sizeof(int));
    graph->items = allocate(MAX_RECORDS, sizeof(int));
    int userId, itemId, rating, timestamp, fields;
    /* First pass: discover dimensions, rather than reserving 10000 x 10000. */
    while ((fields = readRating(file, &userId, &itemId, &rating, &timestamp)) == 1) {
        if (userId <= 0 || itemId <= 0 || rating < 1 || rating > 5) goto invalid;
        int u = 0, m = 0;
        while (u < graph->numUsers && graph->users[u] != userId) ++u;
        while (m < graph->numItems && graph->items[m] != itemId) ++m;
        if ((u == graph->numUsers && u == MAX_RECORDS) ||
            (m == graph->numItems && m == MAX_RECORDS)) goto invalid;
        if (u == graph->numUsers) graph->users[graph->numUsers++] = userId;
        if (m == graph->numItems) graph->items[graph->numItems++] = itemId;
    }
    if (fields != 0 || !graph->numUsers || !graph->numItems) goto invalid;
    graph->adjMatrix = allocate(graph->numUsers, sizeof(int *));
    for (int u = 0; u < graph->numUsers; ++u)
        graph->adjMatrix[u] = allocate(graph->numItems, sizeof(int));
    rewind(file);
    while ((fields = readRating(file, &userId, &itemId, &rating, &timestamp)) == 1) {
        if (userId <= 0 || itemId <= 0 || rating < 1 || rating > 5) goto invalid;
        int u = 0, m = 0;
        while (u < graph->numUsers && graph->users[u] != userId) ++u;
        while (m < graph->numItems && graph->items[m] != itemId) ++m;
        if (u == graph->numUsers || m == graph->numItems) goto invalid;
        graph->adjMatrix[u][m] = rating;
    }
    if (fields != 0) goto invalid;
    fclose(file);
    return 1;
invalid:
    fprintf(stderr, "Gecersiz, bos veya kapasiteyi asan veri dosyasi.\n");
    fclose(file);
    if (!graph->adjMatrix) graph->numUsers = 0;
    freeGraph(graph);
    return 0;
}

void recommendRandomMovies(BipartiteGraph *graph, int userId) {
    int u = 0;
    while (u < graph->numUsers && graph->users[u] != userId) ++u;
    if (u == graph->numUsers) { puts("Kullanici bulunamadi"); return; }
    int *candidates = allocate(graph->numItems, sizeof(int));
    int count = 0;
    for (int m = 0; m < graph->numItems; ++m)
        if (!graph->adjMatrix[u][m]) candidates[count++] = m;
    for (int i = 0; i < count && i < 5; ++i) {
        int j = i + rand() % (count - i);
        int tmp = candidates[i]; candidates[i] = candidates[j]; candidates[j] = tmp;
        printf("Film ID: %d\n", graph->items[candidates[i]]);
    }
    if (!count) puts("Onerilecek film yok");
    free(candidates);
}

void recommendHighestDegreeMovies(BipartiteGraph *graph, int userId) {
    int userIndex = -1;
    for (int i = 0; i < graph->numUsers; i++) {
        if (graph->users[i] == userId) {
            userIndex = i;
            break;
        }
    }

    if (userIndex == -1) {
        printf("Kullanici bulunamadi\n");
        return;
    }

    int *degrees = allocate(graph->numItems, sizeof(int));
    for (int i = 0; i < graph->numItems; i++) {
        degrees[i] = 0;
        for (int j = 0; j < graph->numUsers; j++) {
            if (graph->adjMatrix[j][i] > 0) {
                degrees[i]++;
            }
        }
    }

    int recommendations = 0;
    printf("Derecesi en yuksek film onerileri:\n");

    while (recommendations < 5) {
        int maxDegree = -1;
        int maxDegreeIndex = -1;

        for (int i = 0; i < graph->numItems; i++) {
            if (graph->adjMatrix[userIndex][i] == 0 && degrees[i] > maxDegree) {
                maxDegree = degrees[i];
                maxDegreeIndex = i;
            }
        }

        if (maxDegreeIndex == -1) {
            break;
        }

        printf("Film ID: %d, Derecesi: %d\n", graph->items[maxDegreeIndex], degrees[maxDegreeIndex]);
        recommendations++;
        degrees[maxDegreeIndex] = -1;
    }

    free(degrees);
}

void recommendByMostSimilarUser(BipartiteGraph *graph, int userId) {
    int userIndex = -1;
    for (int i = 0; i < graph->numUsers; i++) {
        if (graph->users[i] == userId) {
            userIndex = i;
            break;
        }
    }

    if (userIndex == -1) {
        printf("Kullanici bulunamadi\n");
        return;
    }

    double maxSimilarity = 0;
    int mostSimilarUserIndex = -1;
    // Tüm kullanıcılar arasında en benzer kullanıcıyı bulma

    for (int i = 0; i < graph->numUsers; i++) {
        if (i != userIndex) {
            double similarity = 0;
            // Ortak izlenen filmleri sayma
            for (int j = 0; j < graph->numItems; j++) {
                if (graph->adjMatrix[userIndex][j] > 0 && graph->adjMatrix[i][j] > 0) {
                    similarity += 1;
                }
            }
            if (similarity > maxSimilarity) {
                maxSimilarity = similarity;
                mostSimilarUserIndex = i;
            }
        }
    }

    if (mostSimilarUserIndex == -1) {
        printf("Kimse bulunamadi %d.\n", userId);
        return;
    }

    printf(" %d kullanicisina en yakin kullanici  %d.\n", userId, graph->users[mostSimilarUserIndex]);

    printf("Benzerlige gore filmler :\n");

    int recommendedMovies[graph->numItems];
    int movieRatings[graph->numItems];
    int count = 0;
    // En benzer kullanıcının izlediği filmleri toplama
    for (int i = 0; i < graph->numItems; i++) {
        if (graph->adjMatrix[mostSimilarUserIndex][i] > 0 && graph->adjMatrix[userIndex][i] == 0) {
            recommendedMovies[count] = i;
            movieRatings[count] = graph->adjMatrix[mostSimilarUserIndex][i];
            count++;
        }
    }
    // Önerilen filmleri puana göre sıralama (Bubble Sort)
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (movieRatings[j] < movieRatings[j + 1]) {
                int temp = movieRatings[j];
                movieRatings[j] = movieRatings[j + 1];
                movieRatings[j + 1] = temp;

                temp = recommendedMovies[j];
                recommendedMovies[j] = recommendedMovies[j + 1];
                recommendedMovies[j + 1] = temp;
            }
        }
    }

    int recommendations = 0;
    for (int i = 0; i < count && recommendations < 5; i++) {
        printf("Film ID: %d, Degerlendirme: %d\n", graph->items[recommendedMovies[i]], movieRatings[i]);
        recommendations++;
    }

    if (recommendations == 0) {
        printf("Onerilecek film yok\n");
    }
}

// Kullanıcılar arası ağırlıklı mesafe hesaplama fonksiyonu
double calculateWeightedDistance(int *user1Ratings, int *user2Ratings, int numItems) {
    double distance = 0.0;
    int common = 0;
    for (int i = 0; i < numItems; ++i) {
        if (user1Ratings[i] && user2Ratings[i]) {
            double diff = user1Ratings[i] - user2Ratings[i];
            distance += diff * diff; ++common;
        }
    }
    return common ? sqrt(distance / common) : INFINITY;
}

void recommendWeightedMovies(BipartiteGraph *graph, int userId) {
    int u = 0;
    while (u < graph->numUsers && graph->users[u] != userId) ++u;
    if (u == graph->numUsers) { puts("Kullanici bulunamadi"); return; }
    double *scores = allocate(graph->numItems, sizeof(double));
    double *weights = allocate(graph->numItems, sizeof(double));
    /* Compute each user's similarity once; missing ratings are not zero ratings. */
    for (int v = 0; v < graph->numUsers; ++v) {
        if (v == u) continue;
        double distance = calculateWeightedDistance(graph->adjMatrix[u], graph->adjMatrix[v], graph->numItems);
        if (!isfinite(distance)) continue;
        double similarity = 1.0 / (1.0 + distance);
        for (int m = 0; m < graph->numItems; ++m) {
            if (!graph->adjMatrix[u][m] && graph->adjMatrix[v][m]) {
                scores[m] += similarity * graph->adjMatrix[v][m];
                weights[m] += similarity;
            }
        }
    }
    for (int m = 0; m < graph->numItems; ++m)
        scores[m] = weights[m] ? scores[m] / weights[m] : -1.0;
    for (int n = 0; n < 5; ++n) {
        int best = -1;
        for (int m = 0; m < graph->numItems; ++m)
            if (scores[m] >= 0 && (best < 0 || scores[m] > scores[best])) best = m;
        if (best < 0) { if (!n) puts("Onerilecek film yok"); break; }
        printf("Film ID: %d, Tahmini puan: %.3f\n", graph->items[best], scores[best]);
        scores[best] = -1;
    }
    free(scores); free(weights);
}

void recommendByClustering(BipartiteGraph *graph, int userId, int k) {
    int u = 0;
    while (u < graph->numUsers && graph->users[u] != userId) ++u;
    if (u == graph->numUsers) { puts("Kullanici bulunamadi"); return; }
    if (k < 1 || k > graph->numItems) { puts("Gecersiz kume sayisi"); return; }
    /* Each movie is a vector of numUsers ratings (a column of adjMatrix). */
    double *centroids = allocate((size_t)k * graph->numUsers, sizeof(double));
    double *sums = allocate((size_t)k * graph->numUsers, sizeof(double));
    int *clusters = allocate(graph->numItems, sizeof(int));
    int *counts = allocate(k, sizeof(int));
    int *indices = allocate(graph->numItems, sizeof(int));
    for (int m = 0; m < graph->numItems; ++m) { indices[m] = m; clusters[m] = -1; }
    for (int c = 0; c < k; ++c) {
        int j = c + rand() % (graph->numItems - c);
        int tmp = indices[c]; indices[c] = indices[j]; indices[j] = tmp;
        for (int v = 0; v < graph->numUsers; ++v)
            centroids[(size_t)c * graph->numUsers + v] = graph->adjMatrix[v][indices[c]];
    }
    for (int iteration = 0; iteration < 100; ++iteration) {
        int changed = 0;
        memset(counts, 0, (size_t)k * sizeof(int));
        memset(sums, 0, (size_t)k * graph->numUsers * sizeof(double));
        for (int m = 0; m < graph->numItems; ++m) {
            double bestDistance = INFINITY;
            int best = 0;
            for (int c = 0; c < k; ++c) {
                double distance = 0;
                for (int v = 0; v < graph->numUsers; ++v) {
                    double diff = graph->adjMatrix[v][m] - centroids[(size_t)c * graph->numUsers + v];
                    distance += diff * diff;
                }
                if (distance < bestDistance) { bestDistance = distance; best = c; }
            }
            changed |= clusters[m] != best; clusters[m] = best; ++counts[best];
            for (int v = 0; v < graph->numUsers; ++v)
                sums[(size_t)best * graph->numUsers + v] += graph->adjMatrix[v][m];
        }
        for (int c = 0; c < k; ++c)
            if (counts[c]) for (int v = 0; v < graph->numUsers; ++v)
                centroids[(size_t)c * graph->numUsers + v] = sums[(size_t)c * graph->numUsers + v] / counts[c];
        if (!changed) break;
    }
    memset(counts, 0, (size_t)k * sizeof(int));
    for (int m = 0; m < graph->numItems; ++m)
        if (graph->adjMatrix[u][m]) ++counts[clusters[m]];
    int favorite = 0;
    for (int c = 1; c < k; ++c) if (counts[c] > counts[favorite]) favorite = c;
    int recommended = 0;
    for (int m = 0; m < graph->numItems && recommended < 5; ++m)
        if (counts[favorite] && clusters[m] == favorite && !graph->adjMatrix[u][m]) {
            printf("Film ID: %d\n", graph->items[m]); ++recommended;
        }
    if (!recommended) puts("Kumeden onerilecek film yok");
    free(centroids); free(sums); free(clusters); free(counts); free(indices);
}


void computeTransitionMatrix(BipartiteGraph *graph, double **transitionMatrix) {
     // Kullanıcı ve öğe ilişkilerine göre geçiş olasılıklarını hesaplama
    for (int i = 0; i < graph->numUsers; i++) {
        double rowSum = 0.0;
        for (int j = 0; j < graph->numItems; j++) {
            rowSum += graph->adjMatrix[i][j];
        }

        for (int j = 0; j < graph->numItems; j++) {
            if (rowSum > 0) {
                transitionMatrix[i][j] = graph->adjMatrix[i][j] / rowSum;
            } else {
                transitionMatrix[i][j] = 0.0;
            }
        }
    }
}

void randomWalkDistance(BipartiteGraph *graph, int userId, int steps) {
    int u = 0;
    while (u < graph->numUsers && graph->users[u] != userId) ++u;
    if (u == graph->numUsers) { puts("Kullanici bulunamadi"); return; }
    if (steps < 1) { puts("Adim sayisi pozitif olmali"); return; }
    double *users = allocate(graph->numUsers, sizeof(double));
    double *movies = allocate(graph->numItems, sizeof(double));
    double *userDegrees = allocate(graph->numUsers, sizeof(double));
    double *movieDegrees = allocate(graph->numItems, sizeof(double));
    for (int v = 0; v < graph->numUsers; ++v)
        for (int m = 0; m < graph->numItems; ++m) {
            userDegrees[v] += graph->adjMatrix[v][m];
            movieDegrees[m] += graph->adjMatrix[v][m];
        }
    users[u] = 1.0;
    /* Each round ends on movies: user->movie, then movie->user before the next round. */
    for (int round = 0; round < steps; ++round) {
        memset(movies, 0, (size_t)graph->numItems * sizeof(double));
        for (int v = 0; v < graph->numUsers; ++v)
            if (userDegrees[v]) for (int m = 0; m < graph->numItems; ++m)
                movies[m] += users[v] * graph->adjMatrix[v][m] / userDegrees[v];
        if (round + 1 == steps) break;
        memset(users, 0, (size_t)graph->numUsers * sizeof(double));
        for (int m = 0; m < graph->numItems; ++m)
            if (movieDegrees[m]) for (int v = 0; v < graph->numUsers; ++v)
                users[v] += movies[m] * graph->adjMatrix[v][m] / movieDegrees[m];
    }
    for (int n = 0; n < 5; ++n) {
        int best = -1;
        for (int m = 0; m < graph->numItems; ++m)
            if (!graph->adjMatrix[u][m] && movies[m] > 0 && (best < 0 || movies[m] > movies[best])) best = m;
        if (best < 0) { if (!n) puts("Ulasilabilir film yok"); break; }
        printf("Film ID: %d, Olasilik: %.6f\n", graph->items[best], movies[best]); movies[best] = -1;
    }
    free(users); free(movies); free(userDegrees); free(movieDegrees);
}

void displayMenu() {
    printf("\n===== Film Oneri Sistemi =====\n");
    printf("1. Rastgele film onerisi\n");
    printf("2. En yuksek dereceli film onerisi\n");
    printf("3. En yakin kullanciya gore film onerisi\n");
    printf("4. Agirlikli uzakliga gore film onerisi\n");
    printf("5. Clusteringe gore film onerisi\n");
    printf("6. Rassal yuruyuse gore film onerisi\n");
    printf("0. Cikis!\n");
    printf("=======================================\n");
    printf("Secimi giriniz: ");
}


int readInteger(int *value) {
    char line[128], *end;
    if (!fgets(line, sizeof(line), stdin)) return 0;
    if (!strchr(line, '\n') && !feof(stdin)) {
        int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
        puts("Girdi cok uzun"); return 0;
    }
    errno = 0;
    long parsed = strtol(line, &end, 10);
    if (end == line || errno || parsed < INT_MIN || parsed > INT_MAX) {
        puts("Gecersiz sayi"); return 0;
    }
    while (isspace((unsigned char)*end)) ++end;
    if (*end) { puts("Gecersiz sayi"); return 0; }
    *value = (int)parsed; return 1;
}

int main(int argc, char **argv) {
    BipartiteGraph graph = {0};
    srand((unsigned int)time(NULL));
    if (argc > 2) { fprintf(stderr, "Usage: %s [ratings_file]\n", argv[0]); return EXIT_FAILURE; }
    const char *ratings = argc == 2 ? argv[1] : "data/sample_ratings.tsv";
    if (!createBipartiteGraph(ratings, &graph)) return EXIT_FAILURE;

    int userId, choice;
    printf("Kullanici ID giriniz: ");
    if (!readInteger(&userId)) { freeGraph(&graph); return EXIT_FAILURE; }

    do {
        displayMenu();
        if (!readInteger(&choice)) break;

        switch (choice) {
            case 1:
                recommendRandomMovies(&graph, userId);
                break;
            case 2:
                recommendHighestDegreeMovies(&graph, userId);
                break;
            case 3:
                recommendByMostSimilarUser(&graph, userId);
                break;
            case 4:
                recommendWeightedMovies(&graph, userId);
                break;
            case 5: {
                int k;
                printf("Kac cluster kumesi olmasini istiyorsunuz ");
                if (!readInteger(&k)) { choice = 0; break; }
                recommendByClustering(&graph, userId, k);
                break;
            }
            case 6:
                randomWalkDistance(&graph, userId, MAX_STEPS);
                break;
            case 0:
                printf("Program kapatiliyor\n");
                break;
            default:
                printf("Boyle bir secenek yok.Tekrar deneyiniz!\n");
                break;
        }
    } while (choice != 0);

    freeGraph(&graph);
    return 0;
}
