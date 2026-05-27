#ifndef _IMAGE_SEGMENTATION_H_
#define _IMAGE_SEGMENTATION_H_

#include "rgb_image.h"
#include <string>

class ImageSegmentation {
private:
    int tolerance; // 顏色閥值

public:
    ImageSegmentation(int tol = 60);
    ~ImageSegmentation();

    void Segment(RGBImage* src, RGBImage*& subject, RGBImage*& background);
};

#endif