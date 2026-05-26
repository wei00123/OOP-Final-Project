#ifndef SINGLE_IMAGE_MOSAIC_H
#define SINGLE_IMAGE_MOSAIC_H

#include "rgb_image.h"
#include <vector>
#include <string>

class SingleImageMosaic {
private:
    int tileSize;
    
    struct TileInfo {
        RGBImage* tile_img;
        int avgR, avgG, avgB;
    };
    
    std::vector<TileInfo> tile_db;

    // 輔助函式：計算區塊平均顏色
    void CalculateAverageColor(RGBImage* img, int &r, int &g, int &b);

public:
    SingleImageMosaic(int size);
    ~SingleImageMosaic();

    // 核心功能一：從單一張圖片中利用重疊滑動視窗切碎並載入磁磚
    void LoadTilesFromSingleImage(RGBImage* source_img);
    
    // 核心功能二：執行單圖馬賽克處理 (包含色彩遷移與微幅混合演算法)
    void Process(RGBImage* target, double blend_ratio = 0.2);
};

#endif