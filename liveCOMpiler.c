#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
  #include <windows.h>
  #define SLEEP_MS(ms) Sleep(ms)
  #define CLEAR "cls"
  #define BINARY ".livec_out.exe"
#else
  #include <unistd.h>
  #define SLEEP_MS(ms) usleep((ms) * 1000)
  #define CLEAR "clear"
  #define BINARY "/tmp/.livec_out"
#endif

#define POLL_MS     500
#define CHUNK       8192
#define MD5_STR_LEN 33

/* ── portable MD5 (RSA Data Security public-domain implementation) ─────── */

typedef unsigned int  u32;
typedef unsigned char u8;

typedef struct { u32 state[4]; u32 count[2]; u8 buf[64]; } MD5_CTX;

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

static void md5_encode(u8 *out, const u32 *in, u32 len) {
    for (u32 i = 0, j = 0; j < len; i++, j += 4) {
        out[j]   = (u8)(in[i]);
        out[j+1] = (u8)(in[i] >> 8);
        out[j+2] = (u8)(in[i] >> 16);
        out[j+3] = (u8)(in[i] >> 24);
    }
}
static void md5_decode(u32 *out, const u8 *in, u32 len) {
    for (u32 i = 0, j = 0; j < len; i++, j += 4)
        out[i] = ((u32)in[j]) | (((u32)in[j+1])<<8) |
                 (((u32)in[j+2])<<16) | (((u32)in[j+3])<<24);
}

#define F(x,y,z) (((x)&(y))|((~x)&(z)))
#define G(x,y,z) (((x)&(z))|((y)&(~z)))
#define H(x,y,z) ((x)^(y)^(z))
#define I(x,y,z) ((y)^((x)|(~z)))
#define RL(x,n)  (((x)<<(n))|((x)>>(32-(n))))
#define FF(a,b,c,d,x,s,t) { (a)+=F(b,c,d)+(x)+(t); (a)=RL(a,s)+(b); }
#define GG(a,b,c,d,x,s,t) { (a)+=G(b,c,d)+(x)+(t); (a)=RL(a,s)+(b); }
#define HH(a,b,c,d,x,s,t) { (a)+=H(b,c,d)+(x)+(t); (a)=RL(a,s)+(b); }
#define II(a,b,c,d,x,s,t) { (a)+=I(b,c,d)+(x)+(t); (a)=RL(a,s)+(b); }

static void md5_transform(u32 state[4], const u8 block[64]) {
    u32 a=state[0],b=state[1],c=state[2],d=state[3],x[16];
    md5_decode(x, block, 64);
    FF(a,b,c,d,x[ 0],S11,0xd76aa478); FF(d,a,b,c,x[ 1],S12,0xe8c7b756);
    FF(c,d,a,b,x[ 2],S13,0x242070db); FF(b,c,d,a,x[ 3],S14,0xc1bdceee);
    FF(a,b,c,d,x[ 4],S11,0xf57c0faf); FF(d,a,b,c,x[ 5],S12,0x4787c62a);
    FF(c,d,a,b,x[ 6],S13,0xa8304613); FF(b,c,d,a,x[ 7],S14,0xfd469501);
    FF(a,b,c,d,x[ 8],S11,0x698098d8); FF(d,a,b,c,x[ 9],S12,0x8b44f7af);
    FF(c,d,a,b,x[10],S13,0xffff5bb1); FF(b,c,d,a,x[11],S14,0x895cd7be);
    FF(a,b,c,d,x[12],S11,0x6b901122); FF(d,a,b,c,x[13],S12,0xfd987193);
    FF(c,d,a,b,x[14],S13,0xa679438e); FF(b,c,d,a,x[15],S14,0x49b40821);
    GG(a,b,c,d,x[ 1],S21,0xf61e2562); GG(d,a,b,c,x[ 6],S22,0xc040b340);
    GG(c,d,a,b,x[11],S23,0x265e5a51); GG(b,c,d,a,x[ 0],S24,0xe9b6c7aa);
    GG(a,b,c,d,x[ 5],S21,0xd62f105d); GG(d,a,b,c,x[10],S22,0x02441453);
    GG(c,d,a,b,x[15],S23,0xd8a1e681); GG(b,c,d,a,x[ 4],S24,0xe7d3fbc8);
    GG(a,b,c,d,x[ 9],S21,0x21e1cde6); GG(d,a,b,c,x[14],S22,0xc33707d6);
    GG(c,d,a,b,x[ 3],S23,0xf4d50d87); GG(b,c,d,a,x[ 8],S24,0x455a14ed);
    GG(a,b,c,d,x[13],S21,0xa9e3e905); GG(d,a,b,c,x[ 2],S22,0xfcefa3f8);
    GG(c,d,a,b,x[ 7],S23,0x676f02d9); GG(b,c,d,a,x[12],S24,0x8d2a4c8a);
    HH(a,b,c,d,x[ 5],S31,0xfffa3942); HH(d,a,b,c,x[ 8],S32,0x8771f681);
    HH(c,d,a,b,x[11],S33,0x6d9d6122); HH(b,c,d,a,x[14],S34,0xfde5380c);
    HH(a,b,c,d,x[ 1],S31,0xa4beea44); HH(d,a,b,c,x[ 4],S32,0x4bdecfa9);
    HH(c,d,a,b,x[ 7],S33,0xf6bb4b60); HH(b,c,d,a,x[10],S34,0xbebfbc70);
    HH(a,b,c,d,x[13],S31,0x289b7ec6); HH(d,a,b,c,x[ 0],S32,0xeaa127fa);
    HH(c,d,a,b,x[ 3],S33,0xd4ef3085); HH(b,c,d,a,x[ 6],S34,0x04881d05);
    HH(a,b,c,d,x[ 9],S31,0xd9d4d039); HH(d,a,b,c,x[12],S32,0xe6db99e5);
    HH(c,d,a,b,x[15],S33,0x1fa27cf8); HH(b,c,d,a,x[ 2],S34,0xc4ac5665);
    II(a,b,c,d,x[ 0],S41,0xf4292244); II(d,a,b,c,x[ 7],S42,0x432aff97);
    II(c,d,a,b,x[14],S43,0xab9423a7); II(b,c,d,a,x[ 5],S44,0xfc93a039);
    II(a,b,c,d,x[12],S41,0x655b59c3); II(d,a,b,c,x[ 3],S42,0x8f0ccc92);
    II(c,d,a,b,x[10],S43,0xffeff47d); II(b,c,d,a,x[ 1],S44,0x85845dd1);
    II(a,b,c,d,x[ 8],S41,0x6fa87e4f); II(d,a,b,c,x[15],S42,0xfe2ce6e0);
    II(c,d,a,b,x[ 6],S43,0xa3014314); II(b,c,d,a,x[13],S44,0x4e0811a1);
    II(a,b,c,d,x[ 4],S41,0xf7537e82); II(d,a,b,c,x[11],S42,0xbd3af235);
    II(c,d,a,b,x[ 2],S43,0x2ad7d2bb); II(b,c,d,a,x[ 9],S44,0xeb86d391);
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
    memset(x, 0, sizeof(x));
}

static void md5_init(MD5_CTX *ctx) {
    ctx->count[0] = ctx->count[1] = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
}

static void md5_update(MD5_CTX *ctx, const u8 *in, u32 len) {
    u32 idx = (ctx->count[0] >> 3) & 0x3f;
    ctx->count[0] += len << 3;
    if (ctx->count[0] < (len << 3)) ctx->count[1]++;
    ctx->count[1] += len >> 29;
    u32 part = 64 - idx;
    u32 i;
    if (len >= part) {
        memcpy(&ctx->buf[idx], in, part);
        md5_transform(ctx->state, ctx->buf);
        for (i = part; i + 63 < len; i += 64)
            md5_transform(ctx->state, &in[i]);
        idx = 0;
    } else { i = 0; }
    memcpy(&ctx->buf[idx], &in[i], len - i);
}

static void md5_final(u8 digest[16], MD5_CTX *ctx) {
    static const u8 pad[64] = { 0x80 };
    u8 bits[8];
    md5_encode(bits, ctx->count, 8);
    u32 idx = (ctx->count[0] >> 3) & 0x3f;
    u32 padlen = (idx < 56) ? (56 - idx) : (120 - idx);
    md5_update(ctx, pad, padlen);
    md5_update(ctx, bits, 8);
    md5_encode(digest, ctx->state, 16);
    memset(ctx, 0, sizeof(*ctx));
}

/* ── file MD5 ────────────────────────────────────────────────────────────── */

static int file_md5(const char *path, char out[MD5_STR_LEN]) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    MD5_CTX ctx;
    md5_init(&ctx);
    u8 buf[CHUNK];
    size_t n;
    while ((n = fread(buf, 1, CHUNK, f)) > 0)
        md5_update(&ctx, buf, (u32)n);
    fclose(f);
    u8 digest[16];
    md5_final(digest, &ctx);
    for (int i = 0; i < 16; i++)
        sprintf(&out[i*2], "%02x", digest[i]);
    out[32] = '\0';
    return 1;
}

/* ── compile + run ───────────────────────────────────────────────────────── */

static const char *src_name = NULL;

static void cleanup(void) {
    remove(BINARY);
}

static void on_signal(int sig) {
    (void)sig;
    cleanup();
    exit(0);
}

static void compile_and_run(const char *src) {
    system(CLEAR);

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "gcc -o %s %s -lm 2>/tmp/.livec_err", BINARY, src);

    if (system(cmd) != 0) {
        printf("error: %s\n\n", src_name);
        FILE *ef = fopen("/tmp/.livec_err", "r");
        if (ef) {
            char line[256];
            while (fgets(line, sizeof(line), ef)) fputs(line, stdout);
            fclose(ef);
        }
        return;
    }

    system(BINARY);
    printf("\n— exited · watching %s —\n", src_name);
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: liveCOMpiler <file.c>\n");
        return 1;
    }

    const char *src = argv[1];

    /* extract basename for display */
    src_name = strrchr(src, '/');
    src_name = src_name ? src_name + 1 : src;

    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);
    atexit(cleanup);

    char last_hash[MD5_STR_LEN] = "";
    char curr_hash[MD5_STR_LEN];

    while (1) {
        if (file_md5(src, curr_hash)) {
            if (strcmp(curr_hash, last_hash) != 0) {
                memcpy(last_hash, curr_hash, MD5_STR_LEN);
                compile_and_run(src);
            }
        }
        SLEEP_MS(POLL_MS);
    }
}
