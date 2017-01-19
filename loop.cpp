extern "C" int _kill(int pid, int sig) {return 0;}
extern "C" int _getpid(void) { return 1;}

//#include <OctoWS2811.h>

#include "nanoflann.hpp"

const int ledsPerStrip = 150;

inline static uint32_t h2rgb(unsigned int v1, unsigned int v2, unsigned int hue);

extern "C" void loop() {

}

extern "C" void setup() {

}


// Convert HSL (Hue, Saturation, Lightness) to RGB (Red, Green, Blue)
//
//   hue:        0 to 359 - position on the color wheel, 0=red, 60=orange,
//                            120=yellow, 180=green, 240=blue, 300=violet
//
//   saturation: 0 to 100 - how bright or dull the color, 100=full, 0=gray
//
//   lightness:  0 to 100 - how light the color is, 100=white, 50=color, 0=black
//
int makeColor(unsigned int hue, int saturation, unsigned int lightness)
{
    uint32_t red, green, blue;
    unsigned int var1, var2;

    if (hue > 359) hue = hue % 360;
    if (saturation > 100) saturation = 100;
    if (lightness > 100) lightness = 100;

    // algorithm from: http://www.easyrgb.com/index.php?X=MATH&H=19#text19
    if (saturation == 0) {
        red = green = blue = lightness * 255 / 100;
    } else {
        if (lightness < 50) {
            var2 = lightness * (100 + saturation);
        } else {
            var2 = ((lightness + saturation) * 100) - (saturation * lightness);
        }
        var1 = lightness * 200 - var2;
        red = h2rgb(var1, var2, (hue < 240) ? hue + 120 : hue - 240) * 255 / static_cast<uint32_t>(600000);
        green = h2rgb(var1, var2, hue) * 255 / static_cast<uint32_t>(600000);
        blue = h2rgb(var1, var2, (hue >= 120) ? hue - 120 : hue + 240) * 255 / static_cast<uint32_t>(600000);
    }
    return (red << 16) | (green << 8) | blue;
}

uint32_t h2rgb(unsigned int v1, unsigned int v2, unsigned int hue)
{
    if (hue < 60) return v1 * 60 + (v2 - v1) * hue;
    if (hue < 180) return v2 * 60;
    if (hue < 240) return v1 * 60 + (v2 - v1) * (240 - hue);
    return v1 * 60;
}

// alternate code:
// http://forum.pjrc.com/threads/16469-looking-for-ideas-on-generating-RGB-colors-from-accelerometer-gyroscope?p=37170&viewfull=1#post37170

