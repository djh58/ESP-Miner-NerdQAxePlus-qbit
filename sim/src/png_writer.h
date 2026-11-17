#pragma once
#include <stdint.h>
#include <string>

// Writes an 8-bit RGB PNG. Returns false and fills `error` on failure.
bool write_png_rgb(const std::string &path, const uint8_t *rgb, int width, int height, std::string &error);
