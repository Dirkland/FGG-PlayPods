/* FGG-PlayPods -- the PlayStation 5's sound on a Bluetooth headset.
 *
 * The PS5 has no Bluetooth audio: its own headsets use a USB dongle. This
 * payload plays the console's sound, game and system alike, on an ordinary
 * Bluetooth headset, through the console's own Bluetooth chip, while the
 * DualSense stays connected wirelessly. Nothing is patched or installed.
 *
 *   capture.c  the console's audio, as Remote Play hears it
 *   hci.c      USB transport to the chip's second Bluetooth controller
 *   bt.c       pairing, the link to the headset, L2CAP, flow control
 *   sdp.c      the service record a headset looks up
 *   a2dp.c     AVDTP, SBC encoding and the stream itself
 *   main.c     the lock and the session
 */
#include "a2dp.h"
#include "bt.h"
#include "capture.h"
#include "log.h"
#include "session_lock.h"

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#define VERSION   "1.0"

#define STATE_DIR "/data/fgg-playpods"
#define LOG_PATH  STATE_DIR "/playpods.log"
#define LOCK_PATH STATE_DIR "/playpods.lock"
#define KEY_PATH  STATE_DIR "/headset.key"

int main(void)
{
    int capturing, streamed = 0, lock_fd = -1, locked;

    if (!log_open(STATE_DIR, LOG_PATH)) return 1;
    log_line("========================================");
    log_line("FGG-PlayPods %s", VERSION);

    locked = session_lock_take(LOCK_PATH, &lock_fd);
    if (locked != 1) {
        notify(locked == 0 ? "FGG-PlayPods: already running" :
               "FGG-PlayPods: cannot acquire session lock - see the log");
        log_close();
        return 1;
    }

    /* Capture first: if it cannot work, Bluetooth is never touched. */
    capturing = capture_open();
    if (capturing < 0) {
        notify("FGG-PlayPods: audio capture unavailable - see the log");
        goto out;
    }

    if (!bt_start()) {
        notify("FGG-PlayPods: the Bluetooth controller did not answer - see the log");
        capture_close();
        goto out;
    }

    if (bt_connect(KEY_PATH) && a2dp_start())
        streamed = a2dp_stream(capturing);
    else
        notify("FGG-PlayPods: could not connect to the headset - see the log");

    a2dp_stop();
    bt_stop();
    capture_close();

    if (streamed) notify("FGG-PlayPods: stopped");

out:
    log_line("FGG-PlayPods done");
    session_lock_release(&lock_fd);
    log_close();
    return 0;
}
