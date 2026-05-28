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
    void Process(RGBImage *target) override;

    struct PreprocessArg {
        CUDA_for_Photo_Mosaic* instance;
        RGBImage* target;
        int* h_block_best_indices;
        int num_blocks_x;
        int num_blocks_y;
        int start_row;
        int end_row;
        int width;
        int height;
    };
    static void* PreprocessWorker(void* arg);
};


#endif