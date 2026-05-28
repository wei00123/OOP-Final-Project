#ifndef _PHOTO_MOSAIC_H_
#define _PHOTO_MOSAIC_H_

#include "rgb_image.h"
#include <vector>
#include <string>

using namespace std;

class Photo_Mosaic {
protected:
    int tileSize;
    struct TileInfo {
        RGBImage* tile_img;
        int avgR, avgG, avgB;
    };
    vector<TileInfo> tile_db;

public:
    Photo_Mosaic(int size);
    virtual ~Photo_Mosaic();
    void CalculateAverageColor(RGBImage* img, int &r, int &g, int &b);
    void LoadTiles(const string& folder_path);
    
    virtual void Process(RGBImage *target);
};

#endif