// EH3D-NODE-HMI-GUI - host tests of the GSL3680 logic: the check of the table the user provides and the reading of a
// data packet. The tables and packets here are made up. GPL-3.0-or-later.
#include <cstdio>
#include <cstdint>
#include <string>

#include "../core/gsl3680.hpp"

static int g_checks = 0;
static int g_failed = 0;
#define CHECK(c)                                                              \
    do {                                                                      \
        ++g_checks;                                                           \
        if (!(c)) {                                                           \
            ++g_failed;                                                       \
            std::printf("FAILED line %d: %s\n", __LINE__, #c);               \
        }                                                                     \
    } while (0)

using namespace hmi::gsl;

static std::string record(std::uint8_t reg) {
    std::string r(kRecordBytes, '\0');
    r[0] = static_cast<char>(reg);
    return r;
}

static void tables() {
    CHECK(!table_ok(""));
    CHECK(table_ok(record(0x00)));
    CHECK(table_ok(record(kPageRegister) + record(0x7c)));
    CHECK(!table_ok(record(0x00).substr(1)));           // not whole records
    CHECK(!table_ok(record(kDataRegister)));            // the data register
    CHECK(!table_ok(record(0x02)));                     // not aligned
    CHECK(!table_ok(record(0x00) + record(0xe0)));      // a control register
    CHECK(!table_ok(std::string(kMaxTableBytes + kRecordBytes - kMaxTableBytes % kRecordBytes, '\0')));
}

static void packets() {
    std::uint8_t p[kPacketBytes] = {};
    CHECK(!decode(p, sizeof p).down);  // no contact
    p[0] = 1;
    CHECK(!decode(p, sizeof p - 1).down);  // short
    CHECK(!decode(nullptr, sizeof p).down);
    // the corner of the raw coordinates: one pixel in from the mirrored side
    Touch t = decode(p, sizeof p);
    CHECK(t.down && t.x == 799 && t.y == 1);
    // far beyond the range stays inside the panel
    p[4] = 0xff;
    p[5] = 0x0f;
    p[6] = 0xff;
    p[7] = 0x0f;
    t = decode(p, sizeof p);
    CHECK(t.down && t.x == 1 && t.y == 1279);
    // the middle: raw 448 of 14 x 64 and 832 of 26 x 64
    p[4] = 0xc0;
    p[5] = 0x01;
    p[6] = 0x40;
    p[7] = 0x03;
    t = decode(p, sizeof p);
    CHECK(t.down && t.x > 390 && t.x < 410 && t.y > 630 && t.y < 650);
    // key data is skipped and the next contact is taken
    p[0] = 2;
    p[5] = 0x10;
    p[8] = 0x02;
    t = decode(p, sizeof p);
    CHECK(t.down && t.x == 799 && t.y == 1);
    p[0] = 1;
    CHECK(!decode(p, sizeof p).down);
    p[0] = kMaxContacts + 1;
    CHECK(!decode(p, sizeof p).down);  // impossible count
}

int main() {
    tables();
    packets();
    std::printf("test_touch: %d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
