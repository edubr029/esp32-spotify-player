#include <Arduino.h>
#include <U8g2lib.h>

// initialization for the 128x64px OLED display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

static const unsigned char image_media_btn_bits[] = {0xfc,0xff,0xff,0xff,0x0f,0xfe,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0xff,0x3f,0xff,0xff,0xff,0xff,0x3f,0xff,0xff,0xff,0xff,0x3f,0xff,0xff,0xff,0xff,0x3f,0xff,0xff,0xff,0xff,0x3f,0xff,0xff,0xff,0xff,0x3f,0xff,0xff,0xff,0xff,0x3f,0xff,0xff,0xff,0xff,0x3f};
static const unsigned char image_btn_bits[] = {0xfc,0xff,0xff,0xff,0x07,0x02,0x00,0x00,0x00,0x08,0x01,0x00,0x00,0x00,0x10,0x01,0x00,0x00,0x00,0x10,0x01,0x00,0x00,0x00,0x10,0x01,0x00,0x00,0x00,0x10,0x01,0x00,0x00,0x00,0x10,0x01,0x00,0x00,0x00,0x10,0x01,0x00,0x00,0x00,0x10,0x01,0x00,0x00,0x00,0x10};
static const unsigned char image_logo_bits[] = {0xf8,0x00,0xfc,0x01,0x8e,0x03,0x33,0x07,0x7d,0x07,0xe7,0x06,0x9b,0x06,0xbf,0x07,0x66,0x03,0xdc,0x01,0xf8,0x00};
static const unsigned char image_play_bits[] = {0x03,0x0f,0x1f,0x3f,0x1f,0x0f,0x03};

// global variables to store parsed data
int8_t progress = 2;
char volume[8];
char title[32];
char artist[32];
char position[8];
char timeRemaining[8];
char fullLength[8];
int8_t playerStatus = -1;

// safer string copy function
void safe_strcpy(char* dest, const char* src, size_t maxLen) {
    size_t i;
    for (i = 0; i < maxLen - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

void parsePlayerctlData(const char* data) {
    char* ptr = (char*)data;
    char* next;

    // parse status
    playerStatus = strtol(ptr, &next, 10);
    if (*next != ';') return;
    ptr = next + 1;

    // parse volume
    next = strchr(ptr, ';');
    if (!next) return;
    safe_strcpy(volume, ptr, min((size_t)(next - ptr + 1), sizeof(volume)));
    ptr = next + 1;

    // parse title
    next = strchr(ptr, ';');
    if (!next) return;
    safe_strcpy(title, ptr, min((size_t)(next - ptr + 1), sizeof(title)));
    ptr = next + 1;

    // parse artist
    next = strchr(ptr, ';');
    if (!next) return;
    safe_strcpy(artist, ptr, min((size_t)(next - ptr + 1), sizeof(artist)));
    ptr = next + 1;

    // parse position
    next = strchr(ptr, ';');
    if (!next) return;
    safe_strcpy(position, ptr, min((size_t)(next - ptr + 1), sizeof(position)));
    ptr = next + 1;

    // parse time remaining
    next = strchr(ptr, ';');
    if (!next) return;
    safe_strcpy(timeRemaining, ptr, min((size_t)(next - ptr + 1), sizeof(timeRemaining)));
    ptr = next + 1;

    // parse full length
    next = strchr(ptr, ';');
    if (!next) return;
    safe_strcpy(fullLength, ptr, min((size_t)(next - ptr + 1), sizeof(fullLength)));
    ptr = next + 1;

    // parse progress
    next = strchr(ptr, ';');
    if (!next) return;
    progress = strtol(ptr, &next, 10);
    if (*next != '\0') return;
}

void setup() {
    Serial.begin(9600);
    u8g2.begin();
    u8g2.clear();

    // initialize strings
    volume[0] = '\0';
    title[0] = '\0';
    artist[0] = '\0';
    position[0] = '\0';
    timeRemaining[0] = '\0';
    fullLength[0] = '\0';
}

void loop() {
    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);

    if (Serial.available()) {
        char buffer[128];
        size_t len = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
        buffer[len] = '\0';
        parsePlayerctlData(buffer);
    }

    // draw player info
    if (playerStatus >= 0) {
        // draw interface
        u8g2.drawXBM(1, 1, 11, 11, image_logo_bits);
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(14, 10, "Spotify");
        u8g2.drawLine(0, 13, 127, 13);
        u8g2.drawLine(125, 10, 109, 10);

        // play/pause icon
        u8g2.setDrawColor(2);
        if (playerStatus) {
            u8g2.drawFrame(61, 56, 2, 7);
            u8g2.drawFrame(65, 56, 2, 7);
        } else {
            u8g2.drawXBM(62, 56, 6, 7, image_play_bits);
        }

        // display info
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(110 - u8g2.getStrWidth(volume) + 15, 9, volume);

        u8g2.setFont(u8g2_font_6x10_tf);
        int titleX = (128 - u8g2.getStrWidth(title)) / 2;
        int artistX = (128 - u8g2.getStrWidth(artist)) / 2;
        u8g2.drawUTF8(titleX, 29, title);
        u8g2.drawUTF8(artistX, 39, artist);

        u8g2.setFont(u8g2_font_4x6_tn);
        u8g2.drawStr(3, 46, position);
        u8g2.drawStr(110 - u8g2.getStrWidth(timeRemaining) + 15, 46, timeRemaining);

        // draw buttons and progress bar
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(6, 63, "Menu");
        u8g2.drawStr(98, 63, "Data");
        u8g2.drawXBM(0, 54, 37, 10, image_btn_bits);
        u8g2.drawXBM(91, 54, 37, 10, image_btn_bits);
        u8g2.drawXBM(45, 54, 38, 10, image_media_btn_bits);

        u8g2.drawRFrame(3, 47, 122, 4, 2);
        u8g2.drawBox(4, 48, map(progress, 0, 100, 0, 120), 2);
    } else {
        u8g2.drawRFrame(1, 1, 126, 62, 3);
        
        u8g2.setFont(u8g2_font_NokiaSmallBold_tr);
        u8g2.drawStr(15, 26, "Initialize the System");
        u8g2.drawStr(47, 36, "Please!");

        u8g2.setFont(u8g2_font_5x7_tf);
        u8g2.drawStr(25, 60, "Send media data.");
    }

    u8g2.sendBuffer();
}

