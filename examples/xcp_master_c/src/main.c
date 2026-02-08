#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

static uint16_t ctr = 0;

static int send_cmd(int s, const uint8_t *cmd, uint16_t len) {
    if (len > 1024)
        return -1;
    uint8_t buf[4 + 1024];
    buf[0] = (uint8_t)(len & 0xFF);
    buf[1] = (uint8_t)(len >> 8);
    buf[2] = (uint8_t)(ctr & 0xFF);
    buf[3] = (uint8_t)(ctr >> 8);
    memcpy(&buf[4], cmd, len);
    ssize_t want = (ssize_t)(4 + len);
    ssize_t n = send(s, buf, (size_t)want, 0);
    if (n != want)
        return -1;
    ctr++;
    return 0;
}

static int recv_resp(int s, uint8_t *buf, uint16_t *len_out) {
    uint8_t hdr[4];
    ssize_t n = recv(s, hdr, 4, MSG_WAITALL);
    if (n != 4)
        return -1;
    uint16_t len = (uint16_t)hdr[0] | ((uint16_t)hdr[1] << 8);
    if (len > 1024)
        return -1;
    n = recv(s, buf, len, MSG_WAITALL);
    if (n != len)
        return -1;
    *len_out = len;
    return 0;
}

static int connect_xcp(int s) {
    uint8_t cmd[2] = {0xFF, 0x00};
    if (send_cmd(s, cmd, 2) != 0)
        return -1;
    uint8_t resp[256];
    uint16_t rl;
    if (recv_resp(s, resp, &rl) != 0)
        return -1;
    return rl > 0 && resp[0] == 0xFF ? 0 : -1;
}

static int disconnect_xcp(int s) {
    uint8_t cmd[1] = {0xFE};
    if (send_cmd(s, cmd, 1) != 0)
        return -1;
    uint8_t resp[256];
    uint16_t rl;
    if (recv_resp(s, resp, &rl) != 0)
        return -1;
    return rl > 0 && resp[0] == 0xFF ? 0 : -1;
}

static int get_status(int s) {
    uint8_t cmd[1] = {0xFD};
    if (send_cmd(s, cmd, 1) != 0)
        return -1;
    uint8_t resp[256];
    uint16_t rl;
    if (recv_resp(s, resp, &rl) != 0)
        return -1;
    return rl > 0 && resp[0] == 0xFF ? 0 : -1;
}

static int get_id(int s) {
    uint8_t cmd[2] = {0xFA, 0x00};
    if (send_cmd(s, cmd, 2) != 0)
        return -1;
    uint8_t resp[512];
    uint16_t rl;
    if (recv_resp(s, resp, &rl) != 0)
        return -1;
    return rl > 0 && resp[0] == 0xFF ? 0 : -1;
}

static int set_mta(int s, uint32_t addr, uint8_t addr_ext) {
    uint8_t cmd[1 + 2 + 1 + 4];
    cmd[0] = 0xF6;
    cmd[1] = 0;
    cmd[2] = 0;
    cmd[3] = addr_ext;
    cmd[4] = (uint8_t)(addr & 0xFF);
    cmd[5] = (uint8_t)((addr >> 8) & 0xFF);
    cmd[6] = (uint8_t)((addr >> 16) & 0xFF);
    cmd[7] = (uint8_t)((addr >> 24) & 0xFF);
    if (send_cmd(s, cmd, sizeof(cmd)) != 0)
        return -1;
    uint8_t resp[256];
    uint16_t rl;
    if (recv_resp(s, resp, &rl) != 0)
        return -1;
    return rl > 0 && resp[0] == 0xFF ? 0 : -1;
}

static int short_download(int s, uint32_t addr, uint16_t val) {
    uint8_t cmd[1 + 1 + 1 + 1 + 4 + 2];
    cmd[0] = 0xF4;
    cmd[1] = 2;
    cmd[2] = 0;
    cmd[3] = 0;
    cmd[4] = (uint8_t)(addr & 0xFF);
    cmd[5] = (uint8_t)((addr >> 8) & 0xFF);
    cmd[6] = (uint8_t)((addr >> 16) & 0xFF);
    cmd[7] = (uint8_t)((addr >> 24) & 0xFF);
    cmd[8] = (uint8_t)(val & 0xFF);
    cmd[9] = (uint8_t)((val >> 8) & 0xFF);
    if (send_cmd(s, cmd, sizeof(cmd)) != 0)
        return -1;
    uint8_t resp[256];
    uint16_t rl;
    if (recv_resp(s, resp, &rl) != 0)
        return -1;
    return rl > 0 && resp[0] == 0xFF ? 0 : -1;
}

static int download(int s, const uint8_t *data, size_t len) {
    const size_t chunk = 32;
    for (size_t off = 0; off < len; off += chunk) {
        size_t n = len - off;
        if (n > chunk)
            n = chunk;
        uint8_t buf[1 + 1 + 32];
        buf[0] = 0xF0;
        buf[1] = (uint8_t)n;
        memcpy(&buf[2], data + off, n);
        if (send_cmd(s, buf, (uint16_t)(2 + n)) != 0)
            return -1;
        uint8_t resp[256];
        uint16_t rl;
        if (recv_resp(s, resp, &rl) != 0)
            return -1;
        if (!(rl > 0 && resp[0] == 0xFF))
            return -1;
    }
    return 0;
}

static int download_once(int s, const uint8_t *data, uint16_t n) {
    if (n > 246)
        return -1; /* MAX_CTO 248, payload limit n<=246 */
    uint8_t cmd[1 + 1 + 246];
    cmd[0] = 0xF0;
    cmd[1] = (uint8_t)n;
    memcpy(&cmd[2], data, n);
    if (send_cmd(s, cmd, (uint16_t)(2 + n)) != 0)
        return -1;
    uint8_t resp[256];
    uint16_t rl;
    if (recv_resp(s, resp, &rl) != 0)
        return -1;
    return rl > 0 && resp[0] == 0xFF ? 0 : -1;
}

int main(int argc, char **argv) {
    int loops = 1000;
    int delay_us = 0;
    int percentiles = 0;
    int bytes = 64;
    enum { MODE_SHORT = 0, MODE_DOWNLOAD = 1 } mode = MODE_SHORT;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--loops") == 0 && i + 1 < argc) {
            loops = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--delay-us") == 0 && i + 1 < argc) {
            delay_us = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--percentiles") == 0) {
            percentiles = 1;
        } else if (strcmp(argv[i], "--bytes") == 0 && i + 1 < argc) {
            bytes = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            const char *m = argv[++i];
            if (strcmp(m, "short") == 0)
                mode = MODE_SHORT;
            else if (strcmp(m, "download") == 0)
                mode = MODE_DOWNLOAD;
        }
    }
    fprintf(stderr, "Starting C Master latency test\n");
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        return 1;
    int one = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    fprintf(stderr, "Socket created\n");
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5555);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(s);
        fprintf(stderr, "Connect failed\n");
        return 1;
    }
    fprintf(stderr, "Connected\n");
    if (connect_xcp(s) != 0) {
        close(s);
        return 1;
    }
    if (get_status(s) != 0) {
        close(s);
        return 1;
    }
    if (get_id(s) != 0) {
        close(s);
        return 1;
    }
    uint32_t ADDR_COUNTER_MAX = 0x80010000u;
    uint32_t ADDR_MAP = 0x8001000Cu;
    double total = 0.0, min = 1e9, max = 0.0;
    double *lts = percentiles && loops > 0 ? (double *)malloc(sizeof(double) * (size_t)loops) : NULL;
    if (mode == MODE_DOWNLOAD) {
        if (set_mta(s, ADDR_MAP, 0) != 0) {
            close(s);
            return 1;
        }
        if (bytes < 1)
            bytes = 1;
        if (bytes > 246)
            bytes = 246;
    }
    for (int i = 0; i < loops; i++) {
        uint8_t cmd[1 + 1 + 1 + 1 + 4 + 2];
        uint16_t val = (uint16_t)(1000 + (i % 100));
        if (mode == MODE_SHORT) {
            cmd[0] = 0xF4;
            cmd[1] = 2;
            cmd[2] = 0;
            cmd[3] = 0;
            cmd[4] = (uint8_t)(ADDR_COUNTER_MAX & 0xFF);
            cmd[5] = (uint8_t)((ADDR_COUNTER_MAX >> 8) & 0xFF);
            cmd[6] = (uint8_t)((ADDR_COUNTER_MAX >> 16) & 0xFF);
            cmd[7] = (uint8_t)((ADDR_COUNTER_MAX >> 24) & 0xFF);
            cmd[8] = (uint8_t)(val & 0xFF);
            cmd[9] = (uint8_t)((val >> 8) & 0xFF);
        }
        struct timeval tv1, tv2;
        gettimeofday(&tv1, NULL);
        if (mode == MODE_SHORT) {
            if (send_cmd(s, cmd, sizeof(cmd)) != 0) {
                close(s);
                return 1;
            }
            uint8_t resp[256];
            uint16_t rl;
            if (recv_resp(s, resp, &rl) != 0) {
                close(s);
                return 1;
            }
            if (!(rl > 0 && resp[0] == 0xFF)) {
                close(s);
                return 1;
            }
        } else {
            if (set_mta(s, ADDR_MAP, 0) != 0) {
                close(s);
                return 1;
            }
            uint8_t data[246];
            for (int j = 0; j < bytes; j++)
                data[j] = (uint8_t)(j + i);
            if (download_once(s, data, (uint16_t)bytes) != 0) {
                close(s);
                return 1;
            }
        }
        gettimeofday(&tv2, NULL);
        double dt = (tv2.tv_sec - tv1.tv_sec) * 1e6 + (tv2.tv_usec - tv1.tv_usec);
        total += dt;
        if (dt < min)
            min = dt;
        if (dt > max)
            max = dt;
        if (lts)
            lts[i] = dt;
        if (delay_us > 0) {
            struct timeval tv = {0};
            tv.tv_usec = delay_us;
            select(0, NULL, NULL, NULL, &tv);
        }
    }
    double avg = total / (loops > 0 ? loops : 1);
    const char *mstr = (mode == MODE_SHORT) ? "short" : "download";
    const char *bstr = (mode == MODE_DOWNLOAD) ? " bytes set" : "";
    printf("C Master RTT: avg=%.2f us min=%.2f us max=%.2f us (N=%d, mode=%s%s)\n", avg, min, max, loops, mstr, bstr);
    if (lts && loops > 0) {
        int cmp(const void *a, const void *b) {
            double da = *(const double *)a, db = *(const double *)b;
            return (da < db) ? -1 : (da > db) ? 1 : 0;
        }
        qsort(lts, (size_t)loops, sizeof(double), cmp);
        int i50 = (int)(0.50 * (loops - 1));
        int i90 = (int)(0.90 * (loops - 1));
        int i95 = (int)(0.95 * (loops - 1));
        int i99 = (int)(0.99 * (loops - 1));
        printf("p50=%.2f us p90=%.2f us p95=%.2f us p99=%.2f us\n", lts[i50], lts[i90], lts[i95], lts[i99]);
        free(lts);
    }
    fflush(stdout);
    disconnect_xcp(s);
    close(s);
    printf("Done\n");
    return 0;
}
