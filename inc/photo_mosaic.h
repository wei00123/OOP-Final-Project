#ifndef _PHOTO_MOSAIC_H_
#define _PHOTO_MOSAIC_H_

#include "rgb_image.h"
#include <vector>
#include <string>

using namespace std;

struct TileInfo {
    RGBImage* tile_img; 
    int avgR, avgG, avgB;
};

class Photo_Mosaic {
private:
    vector<TileInfo> tile_db;
    int tileSize;

    void CalculateAverageColor(RGBImage* img, int &r, int &g, int &b);

public:
    Photo_Mosaic(int size = 16);
    ~Photo_Mosaic();

    void LoadTiles(string folder_path);
    void Process(RGBImage *target);
};

#endif