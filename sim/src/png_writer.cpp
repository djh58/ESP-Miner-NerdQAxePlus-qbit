#include "png_writer.h"

#include <cstdio>
#include <vector>
#include <zlib.h>

namespace {

void put_u32(std::vector<uint8_t> &out, uint32_t v)
{
    out.push_back((v >> 24) & 0xff);
    out.push_back((v >> 16) & 0xff);
    out.push_back((v >> 8) & 0xff);
    out.push_back(v & 0xff);
}

void put_chunk(std::vector<uint8_t> &out, const char type[4], const std::vector<uint8_t> &data)
{
    put_u32(out, (uint32_t) data.size());
    size_t type_at = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    uLong crc = crc32(0L, out.data() + type_at, (uInt) (4 + data.size()));
    put_u32(out, (uint32_t) crc);
}

} // namespace

bool write_png_rgb(const std::string &path, const uint8_t *rgb, int width, int height, std::string &error)
{
    // Each scanline starts with filter type 0 (none).
    std::vector<uint8_t> raw;
    raw.reserve((size_t) height * (1 + (size_t) width * 3));
    for (int y = 0; y < height; y++) {
        raw.push_back(0);
        raw.insert(raw.end(), rgb + (size_t) y * width * 3, rgb + (size_t) (y + 1) * width * 3);
    }

    uLongf packed_len = compressBound((uLong) raw.size());
    std::vector<uint8_t> packed(packed_len);
    if (compress2(packed.data(), &packed_len, raw.data(), (uLong) raw.size(), 9) != Z_OK) {
        error = "zlib compression failed";
        return false;
    }
    packed.resize(packed_len);

    std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    std::vector<uint8_t> ihdr;
    put_u32(ihdr, (uint32_t) width);
    put_u32(ihdr, (uint32_t) height);
    ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0}); // 8-bit depth, truecolor, deflate, no filter, no interlace
    put_chunk(png, "IHDR", ihdr);
    put_chunk(png, "IDAT", packed);
    put_chunk(png, "IEND", {});

    FILE *f = fopen(path.c_str(), "wb");
    if (!f) {
        error = "cannot open " + path;
        return false;
    }
    bool ok = fwrite(png.data(), 1, png.size(), f) == png.size();
    ok = (fclose(f) == 0) && ok;
    if (!ok) error = "short write to " + path;
    return ok;
}
