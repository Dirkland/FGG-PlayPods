# Offline message and packet checks

Run `make check-offline` on Linux with GCC and AddressSanitizer/UndefinedBehaviorSanitizer. This target builds and runs only host fixtures with mocked Bluetooth transport. It needs no PS5 SDK or console.

The three fixtures exercise truncated HCI events and ACL messages, signaling echo and configuration extents, foreign handles, malformed SDP lengths, and packet capacity versus negotiated MTU. They also cover zero, negative and excessive send lengths.

For compile-only target validation, set `PS5_PAYLOAD_SDK` to an extracted SDK and run `make fgg-playpods.elf BUILD=build/native`. Do not use `make test`: that is the upstream deployment target.

These checks establish local bounds handling, not headset interoperability, native API behavior, shared Bluetooth-controller safety or audio quality. Timing, completion policy, native capture parameters and receive scheduling are unchanged in this patch.
