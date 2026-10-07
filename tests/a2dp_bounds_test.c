#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/a2dp.c"

/* Only frames_that_fit is retained by the host linker. No capture,
 * encoder or Bluetooth hardware implementation is linked or executed. */
static int maximum;
int bt_max_frame(void) { return maximum; }

int main(void)
{
    maximum = 1096;
    g_media.remote_mtu = 1100;
    assert(frames_that_fit(119) == 8);
    assert(frames_that_fit(0) == 0);
    assert(frames_that_fit(SIZE_MAX) == 0);
    for (int mtu = 0; mtu <= 1200; mtu++) {
        g_media.remote_mtu = (unsigned)mtu;
        for (size_t frame = 1; frame <= 1024; frame++) {
            int n = frames_that_fit(frame);
            if (n > 0) {
                size_t bytes = RTP_HEADER + (size_t)n * frame;
                assert(bytes <= MEDIA_PACKET_BYTES);
                assert(bytes <= g_media.remote_mtu);
                assert(bytes <= (size_t)(maximum - 4));
                assert(n <= 15);
            }
        }
    }
    maximum = 4;
    assert(frames_that_fit(119) == 0);
    puts("Audio packet capacity checks passed (no hardware)");
    return 0;
}
