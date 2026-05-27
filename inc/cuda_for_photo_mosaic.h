#ifndef CUDA_FOR_PHOTO_MOSAIC_H
#define CUDA_FOR_PHOTO_MOSAIC_H

#include "rgb_image.h"
#include "data_loader.h"
#include "photo_mosaic.h"
#include <vector>
#include <string>

class CUDA_for_Photo_Mosaic: public Photo_Mosaic{
public:
    CUDA_for_Photo_Mosaic(int size);
    ~CUDA_for_Photo_Mosaic();
    void Process(RGBImage *target);
};

#endif