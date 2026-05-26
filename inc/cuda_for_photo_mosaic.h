#ifndef CUDA_FOR_PHOTO_MOSAIC_H
#define CUDA_FOR_PHOTO_MOSAIC_H

#include "rgb_image.h"
#include "data_loader.h"
#include <vector>
#include <string>

class CUDA_for_Photo_Mosaic {
private:
    int tileSize;
    struct TileInfo {
        RGBImage* tile_img;
        int avgR, avgG, avgB;
    };
    std::vector<TileInfo> tile_db;

public:
    CUDA_for_Photo_Mosaic(int size);
    ~CUDA_for_Photo_Mosaic();
    void CalculateAverageColor(RGBImage* img, int &r, int &g, int &b);
    void LoadTiles(std::string folder_path);
    void Process(RGBImage *target);
};

#endif