/*
 * liveCOMpiler.c
 * Compile & run a C file, restarting automatically on save.
 *
 * Build:  gcc -o livecompiler liveCOMpiler.c -lpthread -lm
 * Usage:  ./livecompiler <file.c>
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <limits.h>
#include <errno.h>

#define POLL_MS 500

/* ── shared state (guarded by mutex) ──────────────────────────────────── */
static pthread_mutex_t  g_lock  = PTHREAD_MUTEX_INITIALIZER;
static pthread_t        g_tid;          /* current worker thread          */
static int              g_tid_valid = 0;
static volatile pid_t   g_child_pid = -1; /* PID of running binary        */

/* ── paths ────────────────────────────────────────────────────────────── */
static char g_src[PATH_MAX];
static char g_binary[PATH_MAX];

/* ─────────────────────────────────────────────────────────────────────── */

static void clear(void)
{
    system("clear");
}

/* Simple file fingerprint using mtime + size (no MD5 dep needed). */
static uint64_t file_stamp(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return ((uint64_t)st.st_mtime << 32) | (uint64_t)st.st_size;
}

/* Kill the child binary if it is still running. */
static void kill_child(void)
{
    pid_t pid = g_child_pid;
    if (pid > 0) {
        kill(pid, SIGKILL);
        /* reap it so we don't leave a zombie */
        waitpid(pid, NULL, 0);
        g_child_pid = -1;
    }
}

/* ── worker thread: compile then run ─────────────────────────────────── */
static void *worker(void *arg)
{
    (void)arg;

    clear();

    /* ── compile ── */
    pid_t cpid = fork();
    if (cpid == 0) {
        /* child: exec gcc */
        execlp("gcc", "gcc", "-o", g_binary, g_src, "-lm", (char *)NULL);
        _exit(127);
    }
    if (cpid < 0) { perror("fork"); return NULL; }

    int status = 0;
    waitpid(cpid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "error: compilation failed\n");
        return NULL;
    }

    /* ── run ── */
    pid_t rpid = fork();
    if (rpid == 0) {
        execlp(g_binary, g_binary, (char *)NULL);
        _exit(127);
    }
    if (rpid < 0) { perror("fork"); return NULL; }

    /* publish PID so the watcher can kill it */
    pthread_mutex_lock(&g_lock);
    g_child_pid = rpid;
    pthread_mutex_unlock(&g_lock);

    waitpid(rpid, NULL, 0);

    pthread_mutex_lock(&g_lock);
    g_child_pid = -1;
    pthread_mutex_unlock(&g_lock);

    printf("\n— exited · watching %s —\n", g_src);
    fflush(stdout);
    return NULL;
}

/* Stop the current worker thread + child process. */
static void stop_current(void)
{
    pthread_mutex_lock(&g_lock);
    kill_child();
    int valid = g_tid_valid;
    pthread_mutex_unlock(&g_lock);

    if (valid) {
        pthread_cancel(g_tid);
        pthread_join(g_tid, NULL);
        g_tid_valid = 0;
    }
}

/* Spawn a fresh worker thread. */
static void start_worker(void)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (pthread_create(&g_tid, &attr, worker, NULL) != 0) {
        perror("pthread_create");
    } else {
        g_tid_valid = 1;
    }
    pthread_attr_destroy(&attr);
}

/* ── clean exit on Ctrl-C ─────────────────────────────────────────────── */
static volatile sig_atomic_t g_quit = 0;
static void on_sigint(int s) { (void)s; g_quit = 1; }

/* ── watch loop ───────────────────────────────────────────────────────── */
static void watch(void)
{
    uint64_t last_stamp = 0;

    while (!g_quit) {
        uint64_t cur = file_stamp(g_src);

        if (cur != 0 && cur != last_stamp) {
            last_stamp = cur;
            stop_current();
            start_worker();
        }

        struct timespec ts = { 0, POLL_MS * 1000000L };
        nanosleep(&ts, NULL);
    }

    /* cleanup */
    stop_current();
    remove(g_binary);
}

/* ── main ─────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <file.c>\n", argv[0]);
        return 1;
    }

    /* resolve to absolute path without relying on realpath() */
    if (argv[1][0] == '/') {
        snprintf(g_src, sizeof(g_src), "%s", argv[1]);
    } else {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL) { perror("getcwd"); return 1; }
        snprintf(g_src, sizeof(g_src), "%s/%s", cwd, argv[1]);
    }
    /* quick existence check */
    struct stat _st;
    if (stat(g_src, &_st) != 0) { perror(g_src); return 1; }

    /* build binary path next to source: /path/to/.livec_out */
    char *slash = strrchr(g_src, '/');
    if (slash) {
        size_t dir_len = (size_t)(slash - g_src + 1);
        snprintf(g_binary, sizeof(g_binary), "%.*s.livec_out",
                 (int)dir_len, g_src);
    } else {
        snprintf(g_binary, sizeof(g_binary), ".livec_out");
    }

    struct sigaction sa = { .sa_handler = on_sigint };
    sigaction(SIGINT, &sa, NULL);

    watch();
    return 0;
}
