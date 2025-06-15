/*
 * Copyright 2024-2025 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/quic.h>
#include <openssl/provider.h>
#ifdef _WIN32 /* Windows */
# include <winsock2.h>
#else /* Linux/Unix */
# include <netinet/in.h>
# include <unistd.h>
# include <signal.h>
#endif
#include <assert.h>
#include <string.h> /* For strcmp */
#include <stdio.h>
#include <stdlib.h>

/* -------------------- Runtime‑configurable behaviour -------------------- */

/* Default: close stream right after sending "hello" (original demo). */
#define DEFAULT_CLOSE_AFTER_HELLO 1

/* ----------------------------- Globals ---------------------------------- */

/* Flag is set when the user presses Ctrl+\ (SIGQUIT) to close the *current* connection. */
static volatile sig_atomic_t g_quit_received = 0;

static void handle_sigquit(int sig)
{
    (void)sig;
    g_quit_received = 1; /* handled inside the echo loop */
}

/* ------------------------- TLS/QUIC helpers ----------------------------- */

/* ALPN string for TLS handshake */
static const unsigned char alpn_ossltest[] = {
    /* "\x08ossltest" (hex for EBCDIC resilience) */
    0x08, 0x6f, 0x73, 0x73, 0x6c, 0x74, 0x65, 0x73, 0x74
};

/* This callback validates and negotiates the desired ALPN on the server side. */
static int select_alpn(SSL *ssl,
                       const unsigned char **out, unsigned char *out_len,
                       const unsigned char *in, unsigned int in_len,
                       void *arg)
{
    (void)ssl;
    (void)arg;
    if (SSL_select_next_proto((unsigned char **)out, out_len,
                              alpn_ossltest, sizeof(alpn_ossltest), in, in_len)
            != OPENSSL_NPN_NEGOTIATED)
        return SSL_TLSEXT_ERR_ALERT_FATAL;

    return SSL_TLSEXT_ERR_OK;
}

/* Create SSL_CTX. */
static SSL_CTX *create_ctx(const char *cert_path, const char *key_path)
{
    SSL_CTX *ctx;

    ctx = SSL_CTX_new(OSSL_QUIC_server_method());
    if (ctx == NULL)
        goto err;

    /* Load certificate and corresponding private key. */
    if (SSL_CTX_use_certificate_chain_file(ctx, cert_path) <= 0) {
        fprintf(stderr, "couldn't load certificate file: %s\n", cert_path);
        goto err;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_path, SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "couldn't load key file: %s\n", key_path);
        goto err;
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "private key check failed\n");
        goto err;
    }

    /* Setup ALPN negotiation callback. */
    SSL_CTX_set_alpn_select_cb(ctx, select_alpn, NULL);

    /* Optionally set key exchange. */
    /*
    if (!SSL_CTX_set1_groups_list(ctx, "x25519:MLKEM512:MLKEM768:MLKEM1024")) {
        fprintf(stderr, "failed to set key exchange group\n");
        goto err;
    }
    */

    return ctx;

err:
    SSL_CTX_free(ctx);
    return NULL;
}

/* Create UDP socket using given port. */
static int create_socket(uint16_t port)
{
    int fd = -1;
    struct sockaddr_in sa = {0};

    if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket");
        goto err;
    }

    sa.sin_family  = AF_INET;
    sa.sin_port    = htons(port);

    if (bind(fd, (const struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("bind");
        goto err;
    }

    return fd;

err:
    if (fd >= 0)
        BIO_closesocket(fd);

    return -1;
}

/* Gracefully conclude the default stream by sending a FIN (no payload). */
static int conclude_default_stream(SSL *conn)
{
    /* Zero‑length write with the CONCLUDE flag = FIN only. */
    return SSL_stream_conclude(conn, 0);
}

/*
 * Main loop for servicing a single incoming QUIC connection.
 *
 * If \p close_after_hello is non‑zero the server behaves like the original
 * demo: send "hello\n" with FIN and tear down the connection.
 *
 * Otherwise the stream is kept open and the server enters an interactive echo
 * loop.  When the user hits Ctrl+C the server sends FIN to the client,
 * performs a clean shutdown, and returns so that it can wait for the next
 * connection.
 */
static int run_quic_conn(SSL *conn, int close_after_hello)
{
    size_t written = 0;
    fprintf(stderr, "=> Received connection\n");

    if (close_after_hello) {
        /* Original behaviour: send greeting + FIN. */
        if (!SSL_write_ex2(conn, "hello\n", 6, SSL_WRITE_FLAG_CONCLUDE, &written)
            || written != 6) {
            fprintf(stderr, "couldn't write on connection\n");
            ERR_print_errors_fp(stderr);
            return 0;
        }

        if (SSL_shutdown(conn) != 1) {
            ERR_print_errors_fp(stderr);
            return 0;
        }

        fprintf(stderr, "=> Finished with connection\n");
        return 1;
    }

    /* Keep‑open mode: send greeting without FIN. */
    if (!SSL_write_ex(conn, "hello\n", 6, &written) || written != 6) {
        fprintf(stderr, "couldn't write greeting\n");
        ERR_print_errors_fp(stderr);
        return 0;
    }

    /* Turn the connection non‑blocking for the echo loop. */
    if (!SSL_set_blocking_mode(conn, 0)) {
        fprintf(stderr, "failed to set non‑blocking mode\n");
        return 0;
    }

    fprintf(stderr, "=> Echo loop: Ctrl+\\ to close this connection, Ctrl+C to exit server\n");

    for (;;) {
        /* Detect Ctrl+\ request to close connection gracefully. */
        if (g_quit_received) {
            fprintf(stderr, "=> SIGQUIT caught — closing current connection gracefully\n");
            conclude_default_stream(conn);
            SSL_shutdown(conn); /* Best‑effort */
            g_quit_received = 0; /* Reset flag for next connection. */
            break;
        }

        char buf[2048];
        size_t readbytes = 0;
        int ret = SSL_read_ex(conn, buf, sizeof(buf), &readbytes);

        if (ret == 0) {
            /* QUIC stack wants more network I/O. */
            continue;
        }

        if (ret < 0) {
            int err = SSL_get_error(conn, ret);
            if (err == SSL_ERROR_ZERO_RETURN) {
                fprintf(stderr, "=> Peer closed the connection\n");
                break;
            }
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                continue; /* Retry. */

            fprintf(stderr, "read error in echo loop\n");
            ERR_print_errors_fp(stderr);
            break;
        }

        /* Print what we got to stdout. */
        fwrite(buf, 1, readbytes, stdout);
        fflush(stdout);
    }

    /* Ensure proper closure if we exited without FIN. */
    if (!SSL_get_shutdown(conn)) {
        conclude_default_stream(conn);
        SSL_shutdown(conn);
    }

    fprintf(stderr, "=> Finished with connection\n");
    return 1;
}

/* Main loop for server to accept QUIC connections. */
static int run_quic_server(SSL_CTX *ctx, int fd, int close_after_hello)
{
    int ok = 0;
    SSL *listener = NULL, *conn = NULL;

    if ((listener = SSL_new_listener(ctx, 0)) == NULL)
        goto err;

    if (!SSL_set_fd(listener, fd))
        goto err;

    if (!SSL_listen(listener))
        goto err;

    if (!SSL_set_blocking_mode(listener, 1))
        goto err;

    for (;;) {
        fprintf(stderr, "=> Waiting for connection...\n");

        conn = SSL_accept_connection(listener, 0); /* blocking */
        if (conn == NULL) {
            fprintf(stderr, "error while accepting connection\n");
            goto err;
        }

        if (!run_quic_conn(conn, close_after_hello)) {
            SSL_free(conn);
            goto err;
        }

        SSL_free(conn);
    }

    ok = 1;
err:
    if (!ok)
        ERR_print_errors_fp(stderr);

    SSL_free(listener);
    return ok;
}

/* ------------------------------ main() ---------------------------------- */
int main(int argc, char **argv)
{
    int rc = 1;
    SSL_CTX *ctx = NULL;
    int fd = -1;
    unsigned long port;

    int close_after_hello = DEFAULT_CLOSE_AFTER_HELLO;

    if (argc < 4) {
        fprintf(stderr,
                "usage: %s <port> <server.crt> <server.key> [--keep-open]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    /* Optional behaviour flag. */
    if (argc >= 5 && strcmp(argv[4], "--keep-open") == 0)
        close_after_hello = 0;

    /* Install Ctrl+C handler (only on non‑Windows). */
#ifndef _WIN32
    struct sigaction sa = {0};
    sa.sa_handler = handle_sigquit;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGQUIT, &sa, NULL);
#endif

    /* Create SSL_CTX. */
    if ((ctx = create_ctx(argv[2], argv[3])) == NULL)
        goto err;

    /* Parse port number. */
    port = strtoul(argv[1], NULL, 0);
    if (port == 0 || port > UINT16_MAX) {
        fprintf(stderr, "invalid port: %lu\n", port);
        goto err;
    }

    /* Create UDP socket. */
    if ((fd = create_socket((uint16_t)port)) < 0)
        goto err;

    /* Run the QUIC server loop. */
    if (!run_quic_server(ctx, fd, close_after_hello))
        goto err;

    rc = 0;
err:
    if (rc != 0)
        ERR_print_errors_fp(stderr);

    SSL_CTX_free(ctx);

    if (fd != -1)
        BIO_closesocket(fd);

    return rc;
}
