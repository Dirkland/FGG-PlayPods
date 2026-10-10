# Offline message and packet checks

Run `make check-offline` on Linux with GCC and AddressSanitizer/UndefinedBehaviorSanitizer. This target builds and runs only host fixtures with mocked Bluetooth transport. It needs no PS5 SDK or console.

The three fixtures exercise truncated HCI events and ACL messages, signaling echo and configuration extents, foreign handles, malformed SDP lengths, and packet capacity versus negotiated MTU. They also cover zero, negative and excessive send lengths.

For compile-only target validation, set `PS5_PAYLOAD_SDK` to an extracted SDK and run `make fgg-playpods.elf BUILD=build/native`. Do not use `make test`: that is the upstream deployment target.

These checks establish local bounds handling, not headset interoperability, native API behavior, shared Bluetooth-controller safety or audio quality. Timing, completion policy, native capture parameters and receive scheduling are unchanged in this patch.

## Session and capture boundaries

The five-suite `check-offline` target additionally runs real Linux file-lock contention checks and mocked capture API tests. It covers duplicate launches, lock-open/acquire/release errors, null or invalid capture boundaries and failures at stop/close/terminate. Resource handles are consumed before cleanup so each is attempted once; secondary cleanup failures remain separately reported.

A descriptor lock lasts for the process lifetime. It does not coordinate with older timestamp-lock builds. Linux flock behavior and successful SDK compilation do not establish PS5 filesystem flock semantics. Native capture ABI, cleanup success, wireless input, and long-running audio require console validation; restarting capture after a failed native cleanup is outside these tests.
