#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <oqs/oqs.h>
#include <math.h>

// === Configuration: Choose algorithm and output CSV path ===
#define ALG_NAME OQS_KEM_alg_hqc_256 // Change here for another KEM algorithm
#define ALG_DIR "hqc_256"            // Change this name to match algorithm folder
#define DEFAULT_REPEAT 1000
// ===========================================================

// === Helper: Create folder path (recursive) ===
void create_dir(const char *path) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "mkdir -p %s", path);
    system(tmp);
}

// === Time difference in microseconds ===
double time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1e6 +
           (end.tv_nsec - start.tv_nsec) / 1e3;
}

// === Compute statistics: average, min, max, variance, stddev ===
void compute_stats(double *values, int count, double *avg, double *min, double *max, double *var, double *stddev) {
    double sum = 0.0;
    *min = values[0];
    *max = values[0];
    for (int i = 0; i < count; i++) {
        sum += values[i];
        if (values[i] < *min) *min = values[i];
        if (values[i] > *max) *max = values[i];
    }
    *avg = sum / count;
    double v = 0.0;
    for (int i = 0; i < count; i++) {
        v += (values[i] - *avg) * (values[i] - *avg);
    }
    *var = v / count;
    *stddev = sqrt(*var);
}

int main(int argc, char *argv[]) {

    const char *base_path = "..";
    
    int repeat = DEFAULT_REPEAT;
    if (argc > 1) {
        repeat = atoi(argv[1]);
        if (repeat <= 0) repeat = DEFAULT_REPEAT;
    }

    // === Load KEM algorithm ===
    OQS_KEM *kem = OQS_KEM_new(ALG_NAME);
    if (!kem) {
        fprintf(stderr, "Algorithm not available\n");
        return EXIT_FAILURE;
    }

    // === Print and build output path ===
    char timestamp[64];
    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", localtime(&now));

    char dirpath[256];
    snprintf(dirpath, sizeof(dirpath), "%s/benchmarks/kem/%s/%s", base_path, ALG_DIR, timestamp);
    create_dir(dirpath);

    printf("Benchmarking KEM: %s\n\n", kem->method_name);
    printf("Public key: %zu bytes\n", kem->length_public_key);
    printf("Secret key: %zu bytes\n", kem->length_secret_key);
    printf("Ciphertext:  %zu bytes\n", kem->length_ciphertext);
    printf("Shared key:  %zu bytes\n\n", kem->length_shared_secret);

    // === Allocate memory ===
    uint8_t *pk = malloc(kem->length_public_key);
    uint8_t *sk = malloc(kem->length_secret_key);
    uint8_t *ct = malloc(kem->length_ciphertext);
    uint8_t *ss = malloc(kem->length_shared_secret);
    uint8_t *ss2 = malloc(kem->length_shared_secret);
    double *times_us[3], *cycles[3];
    for (int i = 0; i < 3; i++) {
        times_us[i] = malloc(sizeof(double) * repeat);
        cycles[i] = malloc(sizeof(double) * repeat);
    }

    // === Setup PMU ===
    int has_pmu = 1;
    struct perf_event_attr pe = {0};
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_CPU_CYCLES;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    int fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    if (fd == -1) {
        perror("perf_event_open (PMU not available, cycles will be skipped)");
        has_pmu = 0;
    }

    if (!has_pmu) {
        printf("Warning: PMU not available — skipping CPU cycle measurements\n");
    }

    struct timespec start, end;
    for (int i = 0; i < repeat; i++) {
        uint64_t cyc;

        // === KeyGen ===
        if (has_pmu) { ioctl(fd, PERF_EVENT_IOC_RESET, 0); ioctl(fd, PERF_EVENT_IOC_ENABLE, 0); }
        clock_gettime(CLOCK_MONOTONIC, &start);
        OQS_KEM_keypair(kem, pk, sk);
        clock_gettime(CLOCK_MONOTONIC, &end);
        if (has_pmu) { ioctl(fd, PERF_EVENT_IOC_DISABLE, 0); read(fd, &cyc, sizeof(cyc)); }
        times_us[0][i] = time_diff(start, end);
        cycles[0][i] = (double)cyc;

        // === Encaps ===
        if (has_pmu) { ioctl(fd, PERF_EVENT_IOC_RESET, 0); ioctl(fd, PERF_EVENT_IOC_ENABLE, 0); }
        clock_gettime(CLOCK_MONOTONIC, &start);
        OQS_KEM_encaps(kem, ct, ss, pk);
        clock_gettime(CLOCK_MONOTONIC, &end);
        if (has_pmu) { ioctl(fd, PERF_EVENT_IOC_DISABLE, 0); read(fd, &cyc, sizeof(cyc)); }
        times_us[1][i] = time_diff(start, end);
        cycles[1][i] = (double)cyc;

        // === Decaps ===
        if (has_pmu) { ioctl(fd, PERF_EVENT_IOC_RESET, 0); ioctl(fd, PERF_EVENT_IOC_ENABLE, 0); }
        clock_gettime(CLOCK_MONOTONIC, &start);
        OQS_KEM_decaps(kem, ss2, ct, sk);
        clock_gettime(CLOCK_MONOTONIC, &end);
        if (has_pmu) { ioctl(fd, PERF_EVENT_IOC_DISABLE, 0); read(fd, &cyc, sizeof(cyc)); }
        times_us[2][i] = time_diff(start, end);
        cycles[2][i] = (double)cyc;

        if (memcmp(ss, ss2, kem->length_shared_secret) != 0) {
            fprintf(stderr, "Shared secret mismatch\n");
            return EXIT_FAILURE;
        }
    }

    // === Write metadata ===
    char meta_path[300];
    snprintf(meta_path, sizeof(meta_path), "%s/metadata.txt", dirpath);
    FILE *meta = fopen(meta_path, "w");
    fprintf(meta, "Algorithm: %s\n", kem->method_name);
    fprintf(meta, "PublicKeyBytes: %zu\nSecretKeyBytes: %zu\nCiphertextBytes: %zu\nSharedSecretBytes: %zu\nRepeat: %d\n",
        kem->length_public_key, kem->length_secret_key, kem->length_ciphertext, kem->length_shared_secret, repeat);
    fclose(meta);

    // === Write raw data ===
    const char *labels[3] = {"keygen", "encaps", "decaps"};
    for (int i = 0; i < 3; i++) {
        char file1[300], file2[300];
        snprintf(file1, sizeof(file1), "%s/raw_%s_us.csv", dirpath, labels[i]);
        snprintf(file2, sizeof(file2), "%s/raw_%s_cycles.csv", dirpath, labels[i]);
        FILE *f1 = fopen(file1, "w"), *f2 = fopen(file2, "w");
        for (int j = 0; j < repeat; j++) {
            fprintf(f1, "%.2f\n", times_us[i][j]);
            if (has_pmu) fprintf(f2, "%.0f\n", cycles[i][j]);
        }
        fclose(f1); fclose(f2);
    }

    // === Write summary ===
    char summary_path[300];
    snprintf(summary_path, sizeof(summary_path), "%s/summary.csv", dirpath);
    FILE *sum = fopen(summary_path, "w");
    fprintf(sum, "Operation,Metric,Average,Min,Max,Variance,Sigma\n");
    for (int i = 0; i < 3; i++) {
        double avg, min, max, var, std;
        compute_stats(times_us[i], repeat, &avg, &min, &max, &var, &std);
        fprintf(sum, "%s,us,%.2f,%.2f,%.2f,%.2f,%.2f\n", labels[i], avg, min, max, var, std);
        if (has_pmu) {
            compute_stats(cycles[i], repeat, &avg, &min, &max, &var, &std);
            fprintf(sum, "%s,cycles,%.0f,%.0f,%.0f,%.0f,%.0f\n", labels[i], avg, min, max, var, std);
        }
    }
    fclose(sum);

    // === Cleanup ===
    for (int i = 0; i < 3; i++) {
        free(times_us[i]);
        free(cycles[i]);
    }
    free(pk); free(sk); free(ct); free(ss); free(ss2);
    if (has_pmu) close(fd);
    OQS_KEM_free(kem);

    printf("Benchmark complete. Results saved to %s\n", dirpath);
    return 0;
}

