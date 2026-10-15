// p2721q - controle de brilho do Dell P2721Q via DDC/CI (macOS Apple Silicon)
//
// Uso:
//   p2721q get            mostra o brilho atual
//   p2721q set <0-100>    define o brilho
//   p2721q up [passo]     aumenta (padrão 10)
//   p2721q down [passo]   diminui (padrão 10)
//
// Usa a API privada IOAVService do IOKit para enviar VCP 0x10 (luminância).

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef CFTypeRef IOAVServiceRef;
extern IOAVServiceRef IOAVServiceCreateWithService(CFAllocatorRef, io_service_t);
extern IOReturn IOAVServiceReadI2C(IOAVServiceRef, uint32_t chip, uint32_t offset, void *buf, uint32_t len);
extern IOReturn IOAVServiceWriteI2C(IOAVServiceRef, uint32_t chip, uint32_t offset, void *buf, uint32_t len);

#define DDC_ADDR 0x37
#define EDID_ADDR 0x50
#define VCP_BRIGHTNESS 0x10
#define MODEL_NAME "P2721Q"

static int edid_matches(IOAVServiceRef svc) {
    unsigned char edid[128] = {0}, off = 0;
    if (IOAVServiceWriteI2C(svc, EDID_ADDR, 0x00, &off, 0) != KERN_SUCCESS) { /* alguns firmwares exigem 0 bytes */ }
    if (IOAVServiceReadI2C(svc, EDID_ADDR, 0x00, edid, sizeof edid) != KERN_SUCCESS) return 0;
    // descritores de 18 bytes a partir do offset 54; tag 0xFC = nome do monitor
    for (int i = 54; i <= 108; i += 18) {
        if (edid[i] == 0 && edid[i + 1] == 0 && edid[i + 3] == 0xFC) {
            char name[14] = {0};
            memcpy(name, &edid[i + 5], 13);
            return strstr(name, MODEL_NAME) != NULL;
        }
    }
    return 0;
}

static IOAVServiceRef find_monitor(void) {
    io_iterator_t it;
    if (IOServiceGetMatchingServices(kIOMainPortDefault, IOServiceMatching("DCPAVServiceProxy"), &it) != KERN_SUCCESS)
        return NULL;
    io_service_t s;
    IOAVServiceRef found = NULL;
    while (!found && (s = IOIteratorNext(it))) {
        CFTypeRef loc = IORegistryEntryCreateCFProperty(s, CFSTR("Location"), kCFAllocatorDefault, 0);
        int external = loc && CFGetTypeID(loc) == CFStringGetTypeID() && CFStringCompare(loc, CFSTR("External"), 0) == kCFCompareEqualTo;
        if (loc) CFRelease(loc);
        if (external) {
            IOAVServiceRef svc = IOAVServiceCreateWithService(kCFAllocatorDefault, s);
            if (svc && edid_matches(svc)) found = svc;
            else if (svc) CFRelease(svc);
        }
        IOObjectRelease(s);
    }
    IOObjectRelease(it);
    return found;
}

static int ddc_set(IOAVServiceRef svc, unsigned char vcp, unsigned value) {
    unsigned char b[6] = {0x84, 0x03, vcp, (value >> 8) & 0xFF, value & 0xFF, 0};
    b[5] = (DDC_ADDR << 1) ^ 0x51 ^ b[0] ^ b[1] ^ b[2] ^ b[3] ^ b[4];
    for (int i = 0; i < 4; i++) {
        if (IOAVServiceWriteI2C(svc, DDC_ADDR, 0x51, b, sizeof b) == KERN_SUCCESS) return 0;
        usleep(20000);
    }
    return -1;
}

static int ddc_get(IOAVServiceRef svc, unsigned char vcp, unsigned *cur, unsigned *max) {
    unsigned char req[4] = {0x82, 0x01, vcp, 0};
    req[3] = (DDC_ADDR << 1) ^ 0x51 ^ req[0] ^ req[1] ^ req[2];
    for (int i = 0; i < 4; i++) {
        unsigned char rep[12] = {0};
        if (IOAVServiceWriteI2C(svc, DDC_ADDR, 0x51, req, sizeof req) == KERN_SUCCESS) {
            usleep(50000);
            if (IOAVServiceReadI2C(svc, DDC_ADDR, 0x00, rep, sizeof rep) == KERN_SUCCESS && rep[2] == 0x02 && rep[4] == vcp) {
                if (rep[3] != 0) return 1;  // 1 = VCP não suportado
                *max = (rep[6] << 8) | rep[7];
                *cur = (rep[8] << 8) | rep[9];
                return 0;
            }
        }
        usleep(30000);
    }
    return -1;
}

static const struct { const char *name; unsigned char vcp; } ALIASES[] = {
    {"brightness", 0x10}, {"contrast", 0x12},
    {"input", 0x60}, {"preset", 0xDC}, {"colortemp", 0x14},
    {"red", 0x16}, {"green", 0x18}, {"blue", 0x1A}, {"power", 0xD6},
};

static int parse_vcp(const char *a) {
    for (size_t i = 0; i < sizeof ALIASES / sizeof *ALIASES; i++)
        if (!strcmp(a, ALIASES[i].name)) return ALIASES[i].vcp;
    char *end;
    long v = strtol(a, &end, 0);
    return (*end || v < 0 || v > 255) ? -1 : (int)v;
}

static int cmd_get(IOAVServiceRef svc, int vcp) {
    unsigned cur, max;
    int r = ddc_get(svc, vcp, &cur, &max);
    if (r == 1) { fprintf(stderr, "VCP 0x%02X não suportado\n", vcp); return 1; }
    if (r) { fprintf(stderr, "erro: falha ao ler VCP 0x%02X\n", vcp); return 1; }
    printf("%u (máx %u)\n", cur, max);
    return 0;
}

static int cmd_set(IOAVServiceRef svc, int vcp, long v) {
    if (v < 0) v = 0;
    if (v > 65535) v = 65535;
    if (ddc_set(svc, vcp, (unsigned)v)) { fprintf(stderr, "erro: falha ao escrever VCP 0x%02X\n", vcp); return 1; }
    printf("%ld\n", v);
    return 0;
}

static int cmd_scan(IOAVServiceRef svc) {
    for (int c = 0; c < 256; c++) {
        unsigned cur, max;
        if (ddc_get(svc, c, &cur, &max) == 0) {
            const char *name = "";
            for (size_t i = 0; i < sizeof ALIASES / sizeof *ALIASES; i++)
                if (ALIASES[i].vcp == c) name = ALIASES[i].name;
            printf("0x%02X  atual=%-5u máx=%-5u %s\n", c, cur, max, name);
        }
    }
    return 0;
}

static int cmd_caps(IOAVServiceRef svc) {
    char caps[2048] = {0};
    size_t len = 0;
    unsigned off = 0;
    while (off < sizeof caps - 64) {
        unsigned char req[5] = {0x83, 0xF3, (off >> 8) & 0xFF, off & 0xFF, 0};
        req[4] = (DDC_ADDR << 1) ^ 0x51 ^ req[0] ^ req[1] ^ req[2] ^ req[3];
        unsigned char rep[48] = {0};
        int ok = 0;
        for (int i = 0; i < 5 && !ok; i++) {
            if (IOAVServiceWriteI2C(svc, DDC_ADDR, 0x51, req, sizeof req) == KERN_SUCCESS) {
                usleep(60000);
                ok = IOAVServiceReadI2C(svc, DDC_ADDR, 0x00, rep, sizeof rep) == KERN_SUCCESS && rep[2] == 0xE3;
            }
            if (!ok) usleep(40000);
        }
        if (!ok) { fprintf(stderr, "erro: falha ao ler capabilities\n"); return 1; }
        int n = (rep[1] & 0x7F) - 3;
        if (n <= 0) break;
        memcpy(caps + len, &rep[5], n);
        len += n;
        off += n;
    }
    printf("%s\n", caps);
    return 0;
}

// Liga/desliga o dithering temporal do macOS (mesma técnica do Stillcolor).
// A propriedade "enableDither" fica nas entradas AppleCLCD2 e volta a Yes após reiniciar.
static int cmd_dither(const char *mode) {
    int set = mode && (!strcmp(mode, "on") || !strcmp(mode, "off"));
    CFBooleanRef val = (mode && !strcmp(mode, "on")) ? kCFBooleanTrue : kCFBooleanFalse;
    io_iterator_t it;
    if (IOServiceGetMatchingServices(kIOMainPortDefault, IOServiceMatching("AppleCLCD2"), &it) != KERN_SUCCESS) {
        fprintf(stderr, "erro: AppleCLCD2 não encontrado\n");
        return 1;
    }
    io_service_t s;
    int n = 0, fail = 0;
    while ((s = IOIteratorNext(it))) {
        CFTypeRef cur = IORegistryEntryCreateCFProperty(s, CFSTR("enableDither"), kCFAllocatorDefault, 0);
        if (cur) {
            n++;
            if (set) {
                if (IORegistryEntrySetCFProperty(s, CFSTR("enableDither"), val) != KERN_SUCCESS) fail++;
                CFRelease(cur);
                cur = IORegistryEntryCreateCFProperty(s, CFSTR("enableDither"), kCFAllocatorDefault, 0);
            }
            printf("display %d: dithering %s\n", n, cur && CFEqual(cur, kCFBooleanTrue) ? "ligado" : "desligado");
            if (cur) CFRelease(cur);
        }
        IOObjectRelease(s);
    }
    IOObjectRelease(it);
    if (!n) { fprintf(stderr, "erro: nenhuma propriedade enableDither encontrada\n"); return 1; }
    if (fail) { fprintf(stderr, "erro: falha ao gravar em %d display(s) (talvez precise de sudo)\n", fail); return 1; }
    return 0;
}

static void usage(const char *p) {
    fprintf(stderr,
        "uso: %s <comando>\n"
        "  get                     brilho atual\n"
        "  set <0-100>             define o brilho\n"
        "  up|down [passo]         ajusta o brilho (padrão 10)\n"
        "  get <nome|0xNN>         lê um recurso (ex.: contrast, input, 0x12)\n"
        "  set <nome|0xNN> <valor> escreve um recurso (ex.: set contrast 60)\n"
        "  dither [on|off]         dithering do macOS (off ajuda no flicker; volta ao reiniciar)\n"
        "  caps                    mostra as capabilities oficiais do monitor\n"
        "  scan                    lista todos os VCP que o monitor suporta\n"
        "nomes: brightness contrast input preset colortemp red green blue power\n", p);
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(argv[0]); return 2; }
    if (!strcmp(argv[1], "dither")) return cmd_dither(argc >= 3 ? argv[2] : NULL);
    IOAVServiceRef svc = find_monitor();
    if (!svc) { fprintf(stderr, "erro: monitor Dell %s não encontrado (DDC/CI ativado no menu do monitor?)\n", MODEL_NAME); return 1; }

    const char *cmd = argv[1];
    int rc = 0;

    if (!strcmp(cmd, "get")) {
        int vcp = argc >= 3 ? parse_vcp(argv[2]) : VCP_BRIGHTNESS;
        if (vcp < 0) { fprintf(stderr, "recurso inválido: %s\n", argv[2]); rc = 2; }
        else if (argc >= 3) rc = cmd_get(svc, vcp);
        else { unsigned cur, max; rc = ddc_get(svc, vcp, &cur, &max); if (rc) fprintf(stderr, "erro: falha ao ler o brilho\n"); else printf("%u\n", cur); }
    } else if (!strcmp(cmd, "set") && argc >= 3) {
        if (argc >= 4) {
            int vcp = parse_vcp(argv[2]);
            if (vcp < 0) { fprintf(stderr, "recurso inválido: %s\n", argv[2]); rc = 2; }
            else rc = cmd_set(svc, vcp, strtol(argv[3], NULL, 0));
        } else {
            long v = strtol(argv[2], NULL, 10);
            rc = cmd_set(svc, VCP_BRIGHTNESS, v > 100 ? 100 : v);
        }
    } else if (!strcmp(cmd, "up") || !strcmp(cmd, "down")) {
        long step = argc >= 3 ? strtol(argv[2], NULL, 10) : 10;
        unsigned cur, max;
        if (ddc_get(svc, VCP_BRIGHTNESS, &cur, &max)) { fprintf(stderr, "erro: falha ao ler o brilho\n"); rc = 1; }
        else {
            long v = (long)cur + (cmd[0] == 'u' ? step : -step);
            if (v > (long)max) v = max;
            rc = cmd_set(svc, VCP_BRIGHTNESS, v);
        }
    } else if (!strcmp(cmd, "caps")) {
        rc = cmd_caps(svc);
    } else if (!strcmp(cmd, "scan")) {
        rc = cmd_scan(svc);
    } else {
        usage(argv[0]);
        rc = 2;
    }
    CFRelease(svc);
    return rc ? 1 : 0;
}
