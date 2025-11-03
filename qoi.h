#ifndef QOI_FORMAT_CODEC_QOI_H_
#define QOI_FORMAT_CODEC_QOI_H_

#include "utils.h"

constexpr uint8_t QOI_OP_INDEX_TAG = 0x00;
constexpr uint8_t QOI_OP_DIFF_TAG  = 0x40;
constexpr uint8_t QOI_OP_LUMA_TAG  = 0x80;
constexpr uint8_t QOI_OP_RUN_TAG   = 0xc0; 
constexpr uint8_t QOI_OP_RGB_TAG   = 0xfe;
constexpr uint8_t QOI_OP_RGBA_TAG  = 0xff;
constexpr uint8_t QOI_PADDING[8] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u};
constexpr uint8_t QOI_MASK_2 = 0xc0;

/**
 * @brief encode the raw pixel data of an image to qoi format.
 *
 * @param[in] width image width in pixels
 * @param[in] height image height in pixels
 * @param[in] channels number of color channels, 3 = RGB, 4 = RGBA
 * @param[in] colorspace image color space, 0 = sRGB with linear alpha, 1 = all channels linear
 *
 * @return bool true if it is a valid qoi format image, false otherwise
 */
bool QoiEncode(uint32_t width, uint32_t height, uint8_t channels, uint8_t colorspace = 0);

/**
 * @brief decode the qoi format of an image to raw pixel data
 *
 * @param[out] width image width in pixels
 * @param[out] height image height in pixels
 * @param[out] channels number of color channels, 3 = RGB, 4 = RGBA
 * @param[out] colorspace image color space, 0 = sRGB with linear alpha, 1 = all channels linear
 *
 * @return bool true if it is a valid qoi format image, false otherwise
 */
bool QoiDecode(uint32_t &width, uint32_t &height, uint8_t &channels, uint8_t &colorspace);


bool QoiEncode(uint32_t width, uint32_t height, uint8_t channels, uint8_t colorspace) {

    // qoi-header part

    // write magic bytes "qoif"
    QoiWriteChar('q');
    QoiWriteChar('o');
    QoiWriteChar('i');
    QoiWriteChar('f');
    // write image width
    QoiWriteU32(width);
    // write image height
    QoiWriteU32(height);
    // write channel number
    QoiWriteU8(channels);
    // write color space specifier
    QoiWriteU8(colorspace);

    /* qoi-data part */
    int run = 0;
    int px_num = width * height;

    uint8_t history[64][4];
    memset(history, 0, sizeof(history));

    uint8_t r, g, b, a;
    a = 255u;
    uint8_t pre_r, pre_g, pre_b, pre_a;
    pre_r = 0u;
    pre_g = 0u;
    pre_b = 0u;
    pre_a = 255u;

    for (int i = 0; i < px_num; ++i) {
        r = QoiReadU8();
        g = QoiReadU8();
        b = QoiReadU8();
        if (channels == 4) a = QoiReadU8();

        // Check if current pixel matches previous pixel (for RUN encoding)
        if (r == pre_r && g == pre_g && b == pre_b && a == pre_a) {
            run++;
            // If run reaches maximum (62) or last pixel, encode it
            if (run == 62 || i == px_num - 1) {
                QoiWriteU8(QOI_OP_RUN_TAG | (run - 1));
                run = 0;
            }
        } else {
            // Encode pending run if any
            if (run > 0) {
                QoiWriteU8(QOI_OP_RUN_TAG | (run - 1));
                run = 0;
            }

            // Calculate hash index for current pixel
            int index = QoiColorHash(r, g, b, a);

            // Check if pixel exists in history (INDEX)
            if (history[index][0] == r && history[index][1] == g &&
                history[index][2] == b && history[index][3] == a) {
                QoiWriteU8(QOI_OP_INDEX_TAG | index);
            } else {
                // Calculate differences
                int dr = (int)r - (int)pre_r;
                int dg = (int)g - (int)pre_g;
                int db = (int)b - (int)pre_b;
                int da = (int)a - (int)pre_a;

                // Try DIFF encoding (-2 to +1 range for each channel)
                if (da == 0 && dr >= -2 && dr <= 1 && dg >= -2 && dg <= 1 && db >= -2 && db <= 1) {
                    QoiWriteU8(QOI_OP_DIFF_TAG | ((dr + 2) << 4) | ((dg + 2) << 2) | (db + 2));
                }
                // Try LUMA encoding
                else if (da == 0 && dg >= -32 && dg <= 31) {
                    int dr_dg = dr - dg;
                    int db_dg = db - dg;
                    if (dr_dg >= -8 && dr_dg <= 7 && db_dg >= -8 && db_dg <= 7) {
                        QoiWriteU8(QOI_OP_LUMA_TAG | (dg + 32));
                        QoiWriteU8(((dr_dg + 8) << 4) | (db_dg + 8));
                    } else {
                        // Use RGB
                        QoiWriteU8(QOI_OP_RGB_TAG);
                        QoiWriteU8(r);
                        QoiWriteU8(g);
                        QoiWriteU8(b);
                    }
                }
                // Use RGB or RGBA
                else {
                    if (da != 0) {
                        QoiWriteU8(QOI_OP_RGBA_TAG);
                        QoiWriteU8(r);
                        QoiWriteU8(g);
                        QoiWriteU8(b);
                        QoiWriteU8(a);
                    } else {
                        QoiWriteU8(QOI_OP_RGB_TAG);
                        QoiWriteU8(r);
                        QoiWriteU8(g);
                        QoiWriteU8(b);
                    }
                }
            }

            // Update history for every new pixel (not during RUN)
            history[index][0] = r;
            history[index][1] = g;
            history[index][2] = b;
            history[index][3] = a;
        }

        pre_r = r;
        pre_g = g;
        pre_b = b;
        pre_a = a;
    }

    // qoi-padding part
    for (int i = 0; i < sizeof(QOI_PADDING) / sizeof(QOI_PADDING[0]); ++i) {
        QoiWriteU8(QOI_PADDING[i]);
    }

    return true;
}

bool QoiDecode(uint32_t &width, uint32_t &height, uint8_t &channels, uint8_t &colorspace) {

    char c1 = QoiReadChar();
    char c2 = QoiReadChar();
    char c3 = QoiReadChar();
    char c4 = QoiReadChar();
    if (c1 != 'q' || c2 != 'o' || c3 != 'i' || c4 != 'f') {
        return false;
    }

    // read image width
    width = QoiReadU32();
    // read image height
    height = QoiReadU32();
    // read channel number
    channels = QoiReadU8();
    // read color space specifier
    colorspace = QoiReadU8();

    int run = 0;
    int px_num = width * height;

    uint8_t history[64][4];
    memset(history, 0, sizeof(history));

    uint8_t r, g, b, a;
    a = 255u;

    for (int i = 0; i < px_num; ++i) {
        if (run > 0) {
            // Continue with the run
            run--;
        } else {
            // Read next operation tag
            uint8_t byte1 = QoiReadU8();

            if (byte1 == QOI_OP_RGB_TAG) {
                // QOI_OP_RGB
                r = QoiReadU8();
                g = QoiReadU8();
                b = QoiReadU8();
            } else if (byte1 == QOI_OP_RGBA_TAG) {
                // QOI_OP_RGBA
                r = QoiReadU8();
                g = QoiReadU8();
                b = QoiReadU8();
                a = QoiReadU8();
            } else {
                uint8_t tag = byte1 & QOI_MASK_2;

                if (tag == QOI_OP_INDEX_TAG) {
                    // QOI_OP_INDEX
                    int index = byte1 & 0x3f;
                    r = history[index][0];
                    g = history[index][1];
                    b = history[index][2];
                    a = history[index][3];
                } else if (tag == QOI_OP_DIFF_TAG) {
                    // QOI_OP_DIFF
                    int dr = ((byte1 >> 4) & 0x03) - 2;
                    int dg = ((byte1 >> 2) & 0x03) - 2;
                    int db = (byte1 & 0x03) - 2;
                    r = r + dr;
                    g = g + dg;
                    b = b + db;
                } else if (tag == QOI_OP_LUMA_TAG) {
                    // QOI_OP_LUMA
                    int dg = (byte1 & 0x3f) - 32;
                    uint8_t byte2 = QoiReadU8();
                    int dr_dg = ((byte2 >> 4) & 0x0f) - 8;
                    int db_dg = (byte2 & 0x0f) - 8;
                    g = g + dg;
                    r = r + dg + dr_dg;
                    b = b + dg + db_dg;
                } else if (tag == QOI_OP_RUN_TAG) {
                    // QOI_OP_RUN
                    run = (byte1 & 0x3f);
                }
            }

            // Update history with current pixel
            int index = QoiColorHash(r, g, b, a);
            history[index][0] = r;
            history[index][1] = g;
            history[index][2] = b;
            history[index][3] = a;
        }

        QoiWriteU8(r);
        QoiWriteU8(g);
        QoiWriteU8(b);
        if (channels == 4) QoiWriteU8(a);
    }

    bool valid = true;
    for (int i = 0; i < sizeof(QOI_PADDING) / sizeof(QOI_PADDING[0]); ++i) {
        if (QoiReadU8() != QOI_PADDING[i]) valid = false;
    }

    return valid;
}

#endif // QOI_FORMAT_CODEC_QOI_H_
