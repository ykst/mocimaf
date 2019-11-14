#include "playlog.h"
#include "utils.h"
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// |Magic(2) == "NT"|Version(1) == 01h|
// |{ |Entry(1) != 00h|Payload(*)|, .. }|00h|
// |Body(*)|
// |Frames(4,BE)|CRC(4,BE)|

#define RECORDER_REPEAT_BUF_SIZE (32)
typedef struct recorder {
    FILE *fp;
    uint8_t prev[RECORDER_REPEAT_BUF_SIZE];
    uint16_t repeat;
    uint32_t crc32;
    uint32_t frames;
    char *path;
} recorder_t;

static uint32_t _crc32_table[256] = {
    0x00000000L, 0xF26B8303L, 0xE13B70F7L, 0x1350F3F4L, 0xC79A971FL,
    0x35F1141CL, 0x26A1E7E8L, 0xD4CA64EBL, 0x8AD958CFL, 0x78B2DBCCL,
    0x6BE22838L, 0x9989AB3BL, 0x4D43CFD0L, 0xBF284CD3L, 0xAC78BF27L,
    0x5E133C24L, 0x105EC76FL, 0xE235446CL, 0xF165B798L, 0x030E349BL,
    0xD7C45070L, 0x25AFD373L, 0x36FF2087L, 0xC494A384L, 0x9A879FA0L,
    0x68EC1CA3L, 0x7BBCEF57L, 0x89D76C54L, 0x5D1D08BFL, 0xAF768BBCL,
    0xBC267848L, 0x4E4DFB4BL, 0x20BD8EDEL, 0xD2D60DDDL, 0xC186FE29L,
    0x33ED7D2AL, 0xE72719C1L, 0x154C9AC2L, 0x061C6936L, 0xF477EA35L,
    0xAA64D611L, 0x580F5512L, 0x4B5FA6E6L, 0xB93425E5L, 0x6DFE410EL,
    0x9F95C20DL, 0x8CC531F9L, 0x7EAEB2FAL, 0x30E349B1L, 0xC288CAB2L,
    0xD1D83946L, 0x23B3BA45L, 0xF779DEAEL, 0x05125DADL, 0x1642AE59L,
    0xE4292D5AL, 0xBA3A117EL, 0x4851927DL, 0x5B016189L, 0xA96AE28AL,
    0x7DA08661L, 0x8FCB0562L, 0x9C9BF696L, 0x6EF07595L, 0x417B1DBCL,
    0xB3109EBFL, 0xA0406D4BL, 0x522BEE48L, 0x86E18AA3L, 0x748A09A0L,
    0x67DAFA54L, 0x95B17957L, 0xCBA24573L, 0x39C9C670L, 0x2A993584L,
    0xD8F2B687L, 0x0C38D26CL, 0xFE53516FL, 0xED03A29BL, 0x1F682198L,
    0x5125DAD3L, 0xA34E59D0L, 0xB01EAA24L, 0x42752927L, 0x96BF4DCCL,
    0x64D4CECFL, 0x77843D3BL, 0x85EFBE38L, 0xDBFC821CL, 0x2997011FL,
    0x3AC7F2EBL, 0xC8AC71E8L, 0x1C661503L, 0xEE0D9600L, 0xFD5D65F4L,
    0x0F36E6F7L, 0x61C69362L, 0x93AD1061L, 0x80FDE395L, 0x72966096L,
    0xA65C047DL, 0x5437877EL, 0x4767748AL, 0xB50CF789L, 0xEB1FCBADL,
    0x197448AEL, 0x0A24BB5AL, 0xF84F3859L, 0x2C855CB2L, 0xDEEEDFB1L,
    0xCDBE2C45L, 0x3FD5AF46L, 0x7198540DL, 0x83F3D70EL, 0x90A324FAL,
    0x62C8A7F9L, 0xB602C312L, 0x44694011L, 0x5739B3E5L, 0xA55230E6L,
    0xFB410CC2L, 0x092A8FC1L, 0x1A7A7C35L, 0xE811FF36L, 0x3CDB9BDDL,
    0xCEB018DEL, 0xDDE0EB2AL, 0x2F8B6829L, 0x82F63B78L, 0x709DB87BL,
    0x63CD4B8FL, 0x91A6C88CL, 0x456CAC67L, 0xB7072F64L, 0xA457DC90L,
    0x563C5F93L, 0x082F63B7L, 0xFA44E0B4L, 0xE9141340L, 0x1B7F9043L,
    0xCFB5F4A8L, 0x3DDE77ABL, 0x2E8E845FL, 0xDCE5075CL, 0x92A8FC17L,
    0x60C37F14L, 0x73938CE0L, 0x81F80FE3L, 0x55326B08L, 0xA759E80BL,
    0xB4091BFFL, 0x466298FCL, 0x1871A4D8L, 0xEA1A27DBL, 0xF94AD42FL,
    0x0B21572CL, 0xDFEB33C7L, 0x2D80B0C4L, 0x3ED04330L, 0xCCBBC033L,
    0xA24BB5A6L, 0x502036A5L, 0x4370C551L, 0xB11B4652L, 0x65D122B9L,
    0x97BAA1BAL, 0x84EA524EL, 0x7681D14DL, 0x2892ED69L, 0xDAF96E6AL,
    0xC9A99D9EL, 0x3BC21E9DL, 0xEF087A76L, 0x1D63F975L, 0x0E330A81L,
    0xFC588982L, 0xB21572C9L, 0x407EF1CAL, 0x532E023EL, 0xA145813DL,
    0x758FE5D6L, 0x87E466D5L, 0x94B49521L, 0x66DF1622L, 0x38CC2A06L,
    0xCAA7A905L, 0xD9F75AF1L, 0x2B9CD9F2L, 0xFF56BD19L, 0x0D3D3E1AL,
    0x1E6DCDEEL, 0xEC064EEDL, 0xC38D26C4L, 0x31E6A5C7L, 0x22B65633L,
    0xD0DDD530L, 0x0417B1DBL, 0xF67C32D8L, 0xE52CC12CL, 0x1747422FL,
    0x49547E0BL, 0xBB3FFD08L, 0xA86F0EFCL, 0x5A048DFFL, 0x8ECEE914L,
    0x7CA56A17L, 0x6FF599E3L, 0x9D9E1AE0L, 0xD3D3E1ABL, 0x21B862A8L,
    0x32E8915CL, 0xC083125FL, 0x144976B4L, 0xE622F5B7L, 0xF5720643L,
    0x07198540L, 0x590AB964L, 0xAB613A67L, 0xB831C993L, 0x4A5A4A90L,
    0x9E902E7BL, 0x6CFBAD78L, 0x7FAB5E8CL, 0x8DC0DD8FL, 0xE330A81AL,
    0x115B2B19L, 0x020BD8EDL, 0xF0605BEEL, 0x24AA3F05L, 0xD6C1BC06L,
    0xC5914FF2L, 0x37FACCF1L, 0x69E9F0D5L, 0x9B8273D6L, 0x88D28022L,
    0x7AB90321L, 0xAE7367CAL, 0x5C18E4C9L, 0x4F48173DL, 0xBD23943EL,
    0xF36E6F75L, 0x0105EC76L, 0x12551F82L, 0xE03E9C81L, 0x34F4F86AL,
    0xC69F7B69L, 0xD5CF889DL, 0x27A40B9EL, 0x79B737BAL, 0x8BDCB4B9L,
    0x988C474DL, 0x6AE7C44EL, 0xBE2DA0A5L, 0x4C4623A6L, 0x5F16D052L,
    0xAD7D5351L};

uint32_t crc32_calc(uint32_t crc, int len, uint8_t *buf)
{
    while (len-- > 0) {
        crc = _crc32_table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
    }

    return crc;
}

int _write(recorder_ref self, int len, uint8_t *buf)
{
    GUARD(buf);
    GUARD(fwrite(buf, len, 1, self->fp) == 1);
    self->crc32 = crc32_calc(self->crc32, len, buf);

    return SUCCESS;
error:
    return NG;
}

static int _flush_repeat(recorder_ref self)
{
    if (self->repeat > 0) {
        if (self->repeat > 0x7E) {
            if (self->repeat > 0xFFFF) {
                uint32_t cnt = min(self->repeat, 0xFFFFFFFF);
                self->repeat -= cnt;
                GUARD(_write(self, 5,
                             (uint8_t[]){PLAYLOG_ENTRY_REPEAT32,
                                         (cnt >> 24) & 0xFF, (cnt >> 16) & 0xFF,
                                         (cnt >> 8) & 0xFF, cnt & 0xFF}));
                GUARD(_flush_repeat(self));
            } else {
                uint32_t cnt = self->repeat;
                GUARD(_write(self, 3,
                             (uint8_t[]){PLAYLOG_ENTRY_REPEAT16,
                                         (cnt >> 8) & 0xFF, cnt & 0xFF}));
            }
        } else {
            GUARD(_write(self, 1,
                         (uint8_t[]){0x80 | ((self->repeat - 1) & 0x7F)}));
        }
        self->repeat = 0;
    }

    return SUCCESS;
error:
    return NG;
}

static int _repeat_or_write(recorder_ref self, int len, uint8_t *buf)
{
    GUARD(len < RECORDER_REPEAT_BUF_SIZE);

    if (self->frames > 0 && memcmp(buf, self->prev, len) == 0) {
        ++self->repeat;
    } else {
        _flush_repeat(self);
        memcpy(self->prev, buf, len);
        GUARD(_write(self, len, buf));
    }

    return SUCCESS;
error:
    return NG;
}

int recorder_append(recorder_ref self, uint8_t system, joypad_reg_t joypad1,
                    joypad_reg_t joypad2)
{
    uint8_t buf[8];
    int idx = 1;
    uint8_t com = 0;

    if (system != 0) {
        com |= PLAYLOG_ENTRY_SYSTEM;
        buf[idx++] = system;
    }

    if (joypad1.buttons != 0) {
        com |= PLAYLOG_ENTRY_JOYPAD1;
        buf[idx++] = joypad1.buttons;
    }

    if (joypad2.buttons != 0) {
        com |= PLAYLOG_ENTRY_JOYPAD2;
        buf[idx++] = joypad2.buttons;
    }

    buf[0] = com;

    GUARD(_repeat_or_write(self, idx, buf));

    ++self->frames;

    return SUCCESS;
error:
    return NG;
}

int _write_u32(recorder_ref self, uint32_t v)
{
    return _write(self, 4,
                  (uint8_t[]){(v >> 24) & 0xFF, (v >> 16) & 0xFF,
                              (v >> 8) & 0xFF, v & 0xFF});
}

void recorder_destroy(recorder_ref self)
{
    if (self) {
        if (self->fp != NULL) {
            _flush_repeat(self);
            _write_u32(self, self->frames - 1);
            _write_u32(self, self->crc32);
            fclose(self->fp);
        }
        FREE(self->path);
        FREE(self);
    }
}

recorder_ref recorder_create(const char *path)
{
    recorder_ref self = NULL;
    int fd = -1;

    TALLOC(self);

    if (path) {
        self->path = strdup(path);
        GUARD((fd = open(path, O_WRONLY | O_EXCL | O_CREAT, 0666)) != -1);
    } else {
        self->path = strdup("/tmp/nesrec_XXXXXX");
        GUARD((fd = mkstemp(self->path)) != -1);
        PRINT("temporary rec -> %s\n", self->path);
    }

    GUARD(self->fp = fdopen(fd, "wb"));
    GUARD(_write(self, 4, (uint8_t[]){'N', 'T', 0x01, 0x00}));

    return self;
error:
    if (fd != -1) {
        close(fd);
    }
    recorder_destroy(self);
    return NULL;
}

const char *recorder_get_path(recorder_ref self)
{
    return self->path;
}

typedef struct playback {
    size_t size;
    uint8_t next;
    uint8_t *buf;
    int head;
    int tail;
    int last;
    uint32_t repeat;
    uint8_t version;
    uint32_t total_frames;
    uint32_t frames;
} playback_t;

void playback_destroy(playback_ref self)
{
    if (self) {
        FREE(self->buf);
        FREE(self);
    }
}

int _check_magic(playback_ref self, FILE *fp)
{
    uint8_t buf[4];

    GUARD(self->size > 4);
    GUARD(fread(buf, 4, 1, fp) == 1);
    GUARD(buf[0] == 'N' && buf[1] == 'T');

    uint8_t version = buf[2];

    GUARD(version == 0x01);
    self->version = version;
    rewind(fp);

    return SUCCESS;
error:
    return NG;
}

uint32_t _read_u32(uint8_t *buf)
{
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

int _check_crc32(playback_ref self, FILE *fp)
{
    GUARD(self->size > 4);

    uint8_t buf[1024];
    size_t cnt = self->size - 4;
    uint32_t crc = 0;

    while (cnt > 0) {
        size_t take = min(cnt, 1024);
        GUARD(fread(buf, take, 1, fp) == 1);
        crc = crc32_calc(crc, take, buf);
        cnt -= take;
    }

    GUARD(fread(buf, 4, 1, fp) == 1);
    rewind(fp);

    uint32_t expected_crc = _read_u32(buf);

    GUARD(crc == expected_crc);

    return SUCCESS;
error:
    return NG;
}
#define PLAYLOG_MAGIC "NT"
#define PLAYLOG_HEADER_SIZE (3)

static int _parse_meta(playback_ref self)
{
    GUARD(self->size > PLAYLOG_HEADER_SIZE + 8);
    GUARD(self->buf[PLAYLOG_HEADER_SIZE] == 0x00);
    self->head = self->tail = PLAYLOG_HEADER_SIZE + 1;
    self->total_frames = _read_u32(&self->buf[self->last]);

    return SUCCESS;
error:
    return NG;
}

playback_ref playback_create(const char *path)
{
    playback_ref self = NULL;
    struct stat stat = {};
    FILE *fp = NULL;
    int fd = -1;

    TALLOC(self);
    GUARD((fd = open(path, O_RDONLY)) != -1);
    GUARD(fstat(fd, &stat) == 0);
    GUARD(stat.st_mode & S_IFREG);
    GUARD(!(stat.st_mode & S_IFDIR));
    self->size = stat.st_size;
    self->last = self->size - 8;
    GUARD(self->size > 4);
    GUARD(fp = fdopen(fd, "rb"));
    fd = -1;
    GUARD(_check_magic(self, fp));
    GUARD(_check_crc32(self, fp));
    GUARD(self->buf = malloc(self->size));
    GUARD(fread(self->buf, self->size, 1, fp) == 1);
    GUARD(_parse_meta(self));

    fclose(fp);
    fp = NULL;

    return self;
error:
    if (fd != -1) {
        close(fd);
    }
    if (fp != NULL) {
        fclose(fp);
    }
    playback_destroy(self);
    return NULL;
}

int _step(playback_ref self, int idx, int *out_idx, uint8_t *system,
          joypad_reg_t *joypad1, joypad_reg_t *joypad2)
{
    // TODO: strict overrun check
    uint8_t com = self->buf[idx++];
    GUARD(!(self->repeat > 0 && com >= 80));

    if (com < 0x80) {
        if (self->repeat == 0) {
            self->tail = idx - 1;
        }

        if (com & PLAYLOG_ENTRY_SYSTEM) {
            *system = self->buf[idx++];
        } else {
            *system = 0;
        }

        if (com & PLAYLOG_ENTRY_JOYPAD1) {
            joypad1->buttons = self->buf[idx++];
        } else {
            joypad1->buttons = 0;
        }

        if (com & PLAYLOG_ENTRY_JOYPAD2) {
            joypad2->buttons = self->buf[idx++];
        } else {
            joypad2->buttons = 0;
        }
    } else {
        if (com == PLAYLOG_ENTRY_REPEAT32) {
            uint32_t v = 0;
            v |= self->buf[idx++] << 24;
            v |= self->buf[idx++] << 16;
            v |= self->buf[idx++] << 8;
            v |= self->buf[idx++];
            self->repeat = v;

        } else if (com == PLAYLOG_ENTRY_REPEAT16) {
            uint32_t v = 0;
            v |= self->buf[idx++] << 8;
            v |= self->buf[idx++];
            self->repeat = v;
        } else {
            self->repeat = (com & 0x7F) + 1;
        }

        GUARD(self->repeat != 0);

        GUARD(_step(self, self->tail, NULL, system, joypad1, joypad2));
        self->repeat -= 1;
    }

    if (out_idx != NULL) {
        *out_idx = idx;
    }

    return SUCCESS;
error:
    return NG;
}

int playback_step(playback_ref self, uint8_t *system, joypad_reg_t *joypad1,
                  joypad_reg_t *joypad2)
{
    GUARD(!playback_finished(self));

    if (self->repeat > 0) {
        GUARD(_step(self, self->tail, NULL, system, joypad1, joypad2));
        --self->repeat;
    } else {
        GUARD(_step(self, self->head, &self->head, system, joypad1, joypad2));
    }

    ++self->frames;

    return SUCCESS;
error:
    return NG;
}

bool playback_finished(playback_ref self)
{
    bool finished =
        self->frames == self->total_frames || self->head > self->last;

    if (finished) {
        if (!(self->head == self->last && self->repeat == 0)) {
            WARN("corrupted playback length: expected %u, actual %u\n",
                 self->total_frames, self->frames);
        }
    }

    return finished;
}
