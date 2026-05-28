#ifndef _BIT_FIELD_FILTER_H_
#define _BIT_FIELD_FILTER_H_

#include "rgb_image.h"

class Bit_Field_Filter {
private:
    static void Apply_Box_Filter(RGBImage *img);
    static void Apply_Sobel_Gradient(RGBImage *img);
    static void Apply_Contrast_Stretching(RGBImage *img);
    static void Apply_Mosaic_Filter(RGBImage *img);

public:
    Bit_Field_Filter();
    ~Bit_Field_Filter();
    
    void Filter(RGBImage *img, int bit_field);
};

#endif