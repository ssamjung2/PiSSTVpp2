Security fixes for pisstvpp2.c

This document summarizes recommended security fixes and provides exact replacement snippets you can review and apply.

IMPORTANT: These are suggested changes — I can apply them directly if you approve.

1) Prevent audio buffer overflow in `playtone()`

Problem:
- `playtone()` increments global `g_samples` and writes to `g_audio[g_samples]` without bounds checks.

Fix (replace current `playtone` implementation with):

```c
void playtone(uint16_t tonefreq, double tonedur) {
    size_t tonesamples;
    size_t i;
    int32_t voltage = 0;

    tonedur += g_fudge;
    tonesamples = (size_t)((tonedur / g_uspersample) + 0.5);

    if ((uint64_t)g_samples + tonesamples >= MAXSAMPLES) {
        fprintf(stderr, "[ERROR] audio buffer overflow (needed %zu, available %zu)\n",
                tonesamples, (size_t)(MAXSAMPLES - g_samples));
        exit(1);
    }

    double deltatheta = g_twopioverrate * tonefreq;
    for (i = 0; i < tonesamples; ++i) {
        g_samples++;
        if (tonefreq == 0) {
            g_audio[g_samples] = 32768; /* silence (unsigned) */
        } else {
            voltage = (int32_t)(sin(g_theta) * g_scale);
            /* clamp just in case */
            if (voltage > 0xFFFF) voltage = 0xFFFF;
            if (voltage < 0) voltage = 0;
            g_audio[g_samples] = (uint16_t)voltage;
            g_theta += deltatheta;
        }
    }

    g_fudge = tonedur - (tonesamples * g_uspersample);
}
```

Notes:
- Uses `size_t` and checks against `MAXSAMPLES` before writing.
- Exits on overflow; you may prefer a recoverable error path instead.


2) Use `strtol()` instead of `atoi()` for `-r` (sample rate) parsing

Replace the `atoi(optarg)` code with:

```c
{ char *endptr = NULL; long val = strtol(optarg, &endptr, 10);
  if (endptr == optarg || *endptr != '\0') {
      fprintf(stderr, "Error: invalid sample rate '%s'\n", optarg);
      return 1;
  }
  rate = (uint16_t)val;
  if (rate < 8000 || rate > 48000) {
      fprintf(stderr, "Error: Sample rate must be between 8000 and 48000 Hz\n");
      return 1;
  }
}
```


3) Open output file in binary mode and check write results

Replace `g_outfp = fopen(outputfile, "w");` with:

```c
g_outfp = fopen(outputfile, "wb");
if (!g_outfp) {
    fprintf(stderr, "\n[ERROR] Cannot create output file '%s'\n", outputfile);
    perror("fopen");
    g_object_unref(g_imgp);
    vips_shutdown();
    return 2;
}
```

And in `writefile_wav()` / `writefile_aiff()` ensure all `fwrite`/`fputc` return values are checked and that sizes use 64-bit arithmetic:

```c
uint64_t audiosize = (uint64_t)g_samples * CHANS * (BITS/8);
if (audiosize > UINT32_MAX) {
    fprintf(stderr, "[ERROR] audio data too large (%" PRIu64 " bytes)\n", audiosize);
    return; /* or handle error */
}
/* when writing: */
if (fwrite(buffer, 1, to_write, g_outfp) != to_write) {
    fprintf(stderr, "[ERROR] write failed\n");
    perror("fwrite");
    return;
}
```


4) Add final safety check before file write

After audio generation, before invoking the writer, verify `g_samples` is within limits:

```c
if (g_samples == 0) {
    fprintf(stderr, "[ERROR] no audio samples generated\n");
    return 1;
}
if ((uint64_t)g_samples >= MAXSAMPLES) {
    fprintf(stderr, "[ERROR] audio sample count at capacity (%u)\n", g_samples);
    return 1;
}
```


5) Misc: safer `strncpy`/`strncat` usage and path validation

- Use `snprintf()` when composing filenames to avoid subtle off-by-one mistakes.
- Optionally validate paths to avoid writing to critical system locations.

Example:
```c
if (snprintf(outputfile, sizeof(outputfile), "%s.%s", base, format) >= (int)sizeof(outputfile)) {
    fprintf(stderr, "Error: Output filename too long after adding extension\n");
    return 1;
}
```


6) Run with sanitizers and static analyzers

Recommended commands to test after applying fixes:

```bash
# Compile with ASAN and UBSAN
gcc -fsanitize=address,undefined -g pisstvpp2.c -o pisstvpp2 $(pkg-config --cflags --libs vips) -lm
# Run quick tests
./pisstvpp2 -i test.jpg -o /dev/null -p m1 -r 22050 -v

# Static analysis (optional)
cppcheck --enable=all pisstvpp2.c
clang-tidy pisstvpp2.c -- -I/opt/homebrew/include
```


Applying the changes

- I can apply the above changes automatically to `pisstvpp2.c` and run the compile and test commands, or you can review and request edits.

Would you like me to apply these changes now and run tests? If so, I'll implement the safer `playtone`, `strtol` parsing, binary fopen, and write checks, then run compile + the non-verbose/verbose tests you previously used.
