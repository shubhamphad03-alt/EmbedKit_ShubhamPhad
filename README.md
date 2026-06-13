# ringbuff.c — Explanation

This repository contains a single C99 source file implementing an 8-byte
circular (ring) buffer for `uint8_t` data: [ringbuff.c](ringbuff.c).

Purpose
- Provide a small, portable ring buffer implementation suitable for
	embedded firmware examples and unit tests.
- Demonstrates non-overwriting writes, proper success/failure return codes,
	and efficient index wrapping using bitwise operations.

Build & Run
- Compile with GCC (Windows / MSYS2 / MinGW or Linux):

```bash
gcc -Wall -std=c99 "ringbuff.c" -o ringbuff
./ringbuff
```

If you prefer MSYS2 on Windows, use the MinGW-w64 `gcc` from MSYS2 and run
the produced executable as shown above.

Overview of the implementation
- Configuration constants
	- `BUFFER_SIZE` is defined as `8u`. The code uses `BUFFER_MASK` defined as
		`(BUFFER_SIZE - 1u)` to implement wrap-around via `index = (index + 1) & BUFFER_MASK`.
	- This bitwise wrap is only correct when `BUFFER_SIZE` is a power of two.

	Why bitwise AND is faster than modulo on some MCUs
	- Many small microcontrollers lack a hardware integer division unit.
		The modulo operator (`%`) compiles to a software divide, which is
		relatively slow and code-size heavy. Using `& BUFFER_MASK` performs a
		single fast bitwise operation and is therefore much cheaper in CPU
		cycles and ROM footprint.
	- Limitation: this optimization requires `BUFFER_SIZE` to be a power of two
		(so that `BUFFER_MASK` is all-one bits for the low index bits).

- Data structure
	- `typedef struct { uint8_t buffer[BUFFER_SIZE]; uint8_t head; uint8_t tail; uint8_t count; } ringbuf_t;`
	- `head`: index where the next write will store a byte.
	- `tail`: index where the next read will retrieve a byte.
	- `count`: number of bytes currently stored (0..BUFFER_SIZE).

- API functions (all operate in O(1) time)
	- `void ringbuf_init(ringbuf_t *rb)`
		- Initialize the structure to an empty state, zeroes the storage for
			deterministic behavior in examples.

	- `int ringbuf_is_full(const ringbuf_t *rb)`
		- Returns non-zero (true) if the buffer contains `BUFFER_SIZE` bytes.

	- `int ringbuf_is_empty(const ringbuf_t *rb)`
		- Returns non-zero (true) if `count == 0`.

	- `uint8_t ringbuf_count(const ringbuf_t *rb)`
		- Returns the current stored-byte count (0..BUFFER_SIZE).

	- `int ringbuf_write(ringbuf_t *rb, uint8_t data)`
		- Writes a byte to the buffer at `head` and advances `head` with
			bitwise wrapping. If the buffer is full, it returns `RINGBUF_FULL`
			and does not overwrite any unread data.
		- Return codes: `RINGBUF_OK` (0) on success, `RINGBUF_FULL` (-1) on failure.

	- `int ringbuf_read(ringbuf_t *rb, uint8_t *data)`
		- Reads a byte from `tail`, advances `tail` with bitwise wrapping, and
			decrements `count`. If the buffer is empty, returns `RINGBUF_EMPTY` (-2).

Return codes
- `RINGBUF_OK`  (0)   : success
- `RINGBUF_FULL` (-1) : write failed because buffer is full
- `RINGBUF_EMPTY`(-2) : read failed because buffer is empty

Behavior guarantees
- Writes never overwrite unread data — write fails when the buffer is full.
- Reads fail when the buffer is empty.
- All indexes and counters are `uint8_t` because the buffer size is small and
	fits in a byte; this minimizes memory on embedded targets.

Demo sequence and sample output
- The `main()` function in [ringbuff.c](ringbuff.c) performs the exact
	sequence requested in the problem statement: fill the buffer, show the
	failed write when full, read a few bytes, write to force wrap-around,
	then drain the buffer and show a final failed read.

Sample output (produced by the included demo):

```
[WRITE] 0x41 -> OK (count=1)
[WRITE] 0x42 -> OK (count=2)
[WRITE] 0x43 -> OK (count=3)
[WRITE] 0x44 -> OK (count=4)
[WRITE] 0x45 -> OK (count=5)
[WRITE] 0x46 -> OK (count=6)
[WRITE] 0x47 -> OK (count=7)
[WRITE] 0x48 -> OK (count=8) FULL
[WRITE] 0x99 -> FAIL (buffer full)
[READ] -> 0x41 (count=7)
... (reads/writes showing wrap-around)
[READ] (empty) -> FAIL (buffer empty)
```

Embedded firmware notes
- The implementation is not thread-safe or interrupt-safe by itself. If
	the buffer is accessed from both an ISR and main code, you must protect
	accesses — typically by disabling interrupts briefly around writes/reads
	or by using atomic primitives provided by your platform.
- For larger buffers or multi-producer/multi-consumer scenarios, consider
	using proper lock-free schemes or hardware synchronization primitives.

Why no dynamic allocation
- The code deliberately uses a fixed-size array to avoid dynamic memory
	allocation in constrained embedded environments. This keeps behavior
	deterministic and eliminates runtime allocation failures.

Where to look in the source
- Implementation and demo: [ringbuff.c](ringbuff.c)

If you'd like, I can:
- Add a minimal test harness that asserts behavior.
- Make a thread/ISR-safe variant (with optional critical section macros).
- Increase buffer size while preserving the power-of-two optimization.

