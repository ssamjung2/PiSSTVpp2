// OPTIMIZATION STRATEGY:
// Instead of per-pixel subprocess calls OR complex libvips API usage,
// use the Python approach: one subprocess call per file to extract header region
// as PPM, then parse the PPM locally in C for fast pixel access.
//
// This matches Python's vips extract_area + PIL approach.
