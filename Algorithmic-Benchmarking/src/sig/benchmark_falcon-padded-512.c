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

// === Configuration: Select signature algorithm and output path ===
#define ALG_NAME OQS_SIG_alg_falcon_padded_512 // <== Change to e.g., OQS_SIG_alg_dilithium_5 for Dilithium5
#define ALG_DIR "falcon_padded_512"            // <== Folder name for results, adjust accordingly
#define DEFAULT_REPEAT 1000
#define MESSAGE_LEN 32 // Length of dummy message for signing
// ================================================================

// === Helper: Create directory recursively ===
void create_dir(const char *path) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "mkdir -p %s", path);
    system(tmp);
}

// === Calculate time difference in microseconds ===
double time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1e6 +
           (end.tv_nsec - start.tv_nsec) / 1e3;
}

// === Compute average, min, max, variance, and standard deviation ===
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

    // === Load signature algorithm ===
    OQS_SIG *sig = OQS_SIG_new(ALG_NAME);
    if (!sig) {
        fprintf(stderr, "Algorithm not available\n");
        return EXIT_FAILURE;
    }

    // === Create timestamp and output directory ===
    char timestamp[64];
    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", localtime(&now));

    char dirpath[256];
    snprintf(dirpath, sizeof(dirpath), "%s/benchmarks/sig/%s/%s", base_path, ALG_DIR, timestamp);
    create_dir(dirpath);

    // === Print metadata ===
    printf("Benchmarking Signature: %s\n\n", sig->method_name);
    printf("Public key: %zu bytes\n", sig->length_public_key);
    printf("Secret key: %zu bytes\n", sig->length_secret_key);
    printf("Signature:  %zu bytes\n\n", sig->length_signature);

    // === Allocate memory ===
    uint8_t *pk = malloc(sig->length_public_key);
    uint8_t *sk = malloc(sig->length_secret_key);
    uint8_t *msg = malloc(MESSAGE_LEN);
    uint8_t *sig_buf = malloc(sig->length_signature);
    memset(msg, 42, MESSAGE_LEN); // Fill message with dummy data
    size_t sig_len = 0;

    double *times_us[3], *cycles[3];
    for (int i = 0; i < 3; i++) {
        times_us[i] = malloc(sizeof(double) * repeat);
        cycles[i] = malloc(sizeof(double) * repeat);
    }

    // === Setup performance counter (PMU) ===
    struct perf_event_attr pe = {0};
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_CPU_CYCLES;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

   int has_pmu = 1;
    int fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    if (fd == -1) {
        perror("perf_event_open (PMU not available, cycles will be skipped)");
        has_pmu = 0;
    }

    // === Benchmark loop ===
    struct timespec start, end;
    for (int i = 0; i < repeat; i++) {
        uint64_t cyc;

        // --- KeyGen ---
        ioctl(fd, PERF_EVENT_IOC_RESET, 0); ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
        clock_gettime(CLOCK_MONOTONIC, &start);
        OQS_SIG_keypair(sig, pk, sk);
        clock_gettime(CLOCK_MONOTONIC, &end);
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0); read(fd, &cyc, sizeof(cyc));
        times_us[0][i] = time_diff(start, end);
        cycles[0][i] = (double)cyc;

        // --- Sign ---
        ioctl(fd, PERF_EVENT_IOC_RESET, 0); ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
        clock_gettime(CLOCK_MONOTONIC, &start);
        OQS_SIG_sign(sig, sig_buf, &sig_len, msg, MESSAGE_LEN, sk);
        clock_gettime(CLOCK_MONOTONIC, &end);
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0); read(fd, &cyc, sizeof(cyc));
        times_us[1][i] = time_diff(start, end);
        cycles[1][i] = (double)cyc;

        // --- Verify ---
        ioctl(fd, PERF_EVENT_IOC_RESET, 0); ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
        clock_gettime(CLOCK_MONOTONIC, &start);
        OQS_SIG_verify(sig, msg, MESSAGE_LEN, sig_buf, sig_len, pk);
        clock_gettime(CLOCK_MONOTONIC, &end);
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0); read(fd, &cyc, sizeof(cyc));
        times_us[2][i] = time_diff(start, end);
        cycles[2][i] = (double)cyc;
    }

    // === Write metadata to file ===
    char meta_path[300];
    snprintf(meta_path, sizeof(meta_path), "%s/metadata.txt", dirpath);
    FILE *meta = fopen(meta_path, "w");
    fprintf(meta, "Algorithm: %s\n", sig->method_name);
    fprintf(meta, "PublicKeyBytes: %zu\nSecretKeyBytes: %zu\nSignatureBytes: %zu\nRepeat: %d\n",
        sig->length_public_key, sig->length_secret_key, sig->length_signature, repeat);
    fclose(meta);

    // === Write raw measurements to CSV ===
    const char *labels[3] = {"keygen", "sign", "verify"};
    for (int i = 0; i < 3; i++) {
        char file1[300], file2[300];
        snprintf(file1, sizeof(file1), "%s/raw_%s_us.csv", dirpath, labels[i]);
        snprintf(file2, sizeof(file2), "%s/raw_%s_cycles.csv", dirpath, labels[i]);
        FILE *f1 = fopen(file1, "w"), *f2 = fopen(file2, "w");
        for (int j = 0; j < repeat; j++) {
            fprintf(f1, "%.2f\n", times_us[i][j]);
            if (has_pmu) {
                fprintf(f2, "%.0f\n", cycles[i][j]);
            }
        }
        fclose(f1); fclose(f2);
    }

    // === Write statistical summary ===
    char summary_path[300];
    snprintf(summary_path, sizeof(summary_path), "%s/summary.csv", dirpath);
    FILE *sum = fopen(summary_path, "w");
    fprintf(sum, "Operation,Metric,Average,Min,Max,Variance,Sigma\n");
    for (int i = 0; i < 3; i++) {
        double avg, min, max, var, std;
        compute_stats(times_us[i], repeat, &avg, &min, &max, &var, &std);
        fprintf(sum, "%s,us,%.2f,%.2f,%.2f,%.2f,%.2f\n", labels[i], avg, min, max, var, std);
        compute_stats(cycles[i], repeat, &avg, &min, &max, &var, &std);
        fprintf(sum, "%s,cycles,%.0f,%.0f,%.0f,%.0f,%.0f\n", labels[i], avg, min, max, var, std);
    }
    fclose(sum);

    // === Cleanup ===
    for (int i = 0; i < 3; i++) {
        free(times_us[i]);
        free(cycles[i]);
    }
    free(pk); free(sk); free(msg); free(sig_buf);
    if (has_pmu) close(fd);
    OQS_SIG_free(sig);

    printf("Benchmark complete. Results saved to %s\n", dirpath);
    return 0;
}
