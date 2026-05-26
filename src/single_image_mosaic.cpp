#include "single_image_mosaic.h"
#include <iostream>
#include <algorithm>
#include <cmath>

using namespace std;

SingleImageMosaic::SingleImageMosaic(int size) : tileSize(size) {}

SingleImageMosaic::~SingleImageMosaic() {
    for (auto &tile : tile_db) {
        delete tile.tile_img;
    }
    tile_db.clear();
}

void SingleImageMosaic::CalculateAverageColor(RGBImage* img, int &r, int &g, int &b) {
    if (!img) return;
    long long sumR = 0, sumG = 0, sumB = 0;
    int w = img->get_width();
    int h = img->get_height();
    
    if (w <= 0 || h <= 0) {
        r = g = b = 0;
        return;
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            sumR += img->getR(x, y);
            sumG += img->getG(x, y);
            sumB += img->getB(x, y);
        }
    }
    r = (int)(sumR / (w * h));
    g = (int)(sumG / (w * h));
    b = (int)(sumB / (w * h));
}

// 利用重疊滑動視窗 (Sliding Window) 從單張圖片榨出幾千張磁磚
void SingleImageMosaic::LoadTilesFromSingleImage(RGBImage* source_img) {
    if (!source_img) return;

    int srcW = source_img->get_width();
    int srcH = source_img->get_height();
    
    // 為了讓磁磚庫夠豐富，我們讓視窗重疊滑動 (步長為 tileSize 的一半)
    int step = tileSize / 2;
    if (step < 1) step = 1;

    cout << "正在分析單一圖片並切碎結構...\n";

    for (int y = 0; y <= srcH - tileSize; y += step) {
        for (int x = 0; x <= srcW - tileSize; x += step) {
            // 裁剪出局部方塊
            RGBImage* raw_tile = source_img->Crop(x, y, tileSize, tileSize);
            if (!raw_tile) continue;

            int ar, ag, ab;
            CalculateAverageColor(raw_tile, ar, ag, ab);

            TileInfo info;
            info.tile_img = raw_tile; // 儲存切下來的方塊
            info.avgR = ar; 
            info.avgG = ag; 
            info.avgB = ab;
            
            tile_db.push_back(info);
        }
    }
    cout << "成功從單張圖片中提煉出 " << tile_db.size() << " 個結構磁磚！\n";
}

// 執行融合色彩遷移的馬賽克渲染
void SingleImageMosaic::Process(RGBImage* target, double blend_ratio) {
    if (tile_db.empty()) {
        cerr << "錯誤：沒有載入任何單圖磁磚！\n";
        return;
    }

    int width = target->get_width();
    int height = target->get_height();

    cout << "正在進行單圖馬賽克演算法渲染...\n";

    for (int by = 0; by < height; by += tileSize) {
        for (int bx = 0; bx < width; bx += tileSize) {
            
            int currentW = min(tileSize, width - bx);
            int currentH = min(tileSize, height - by);
            
            // 1. 取得目標區塊與平均顏色
            RGBImage* block = target->Crop(bx, by, currentW, currentH);
            int targetR, targetG, targetB;
            CalculateAverageColor(block, targetR, targetG, targetB);
            delete block;

            // 2. 尋找結構最合適的母體磁磚 (以顏色最接近者為基底，減少色彩遷移時的溢位)
            int best_idx = 0;
            long long min_dist = -1;

            for (size_t i = 0; i < tile_db.size(); i++) {
                long long dr = targetR - tile_db[i].avgR;
                long long dg = targetG - tile_db[i].avgG;
                long long db = targetB - tile_db[i].avgB;
                long long dist = dr*dr + dg*dg + db*db;

                if (min_dist == -1 || dist < min_dist) {
                    min_dist = dist;
                    best_idx = i;
                }
            }

            RGBImage* best_tile = tile_db[best_idx].tile_img;
            int tileAvgR = tile_db[best_idx].avgR;
            int tileAvgG = tile_db[best_idx].avgG;
            int tileAvgB = tile_db[best_idx].avgB;

            // 3. 色彩遷移與渲染
            for (int ty = 0; ty < currentH; ty++) {
                for (int tx = 0; tx < currentW; tx++) {
                    // 原汁原味的論文色彩遷移公式：
                    // 新像素 = 磁磚像素 - 磁磚平均色 + 目標平均色
                    int shiftedR = best_tile->getR(tx, ty) - tileAvgR + targetR;
                    int shiftedG = best_tile->getG(tx, ty) - tileAvgG + targetG;
                    int shiftedB = best_tile->getB(tx, ty) - tileAvgB + targetB;

                    // 取得原本目標像素 (用於進階微幅混合，提升整體輪廓清晰度)
                    int origTargetR = target->getR(bx + tx, by + ty);
                    int origTargetG = target->getG(bx + tx, by + ty);
                    int origTargetB = target->getB(bx + tx, by + ty);

                    // 混合公式：讓最終成像既有磁磚的紋理，又保留一點原本大圖的清晰線條
                    int finalR = (1.0 - blend_ratio) * shiftedR + blend_ratio * origTargetR;
                    int finalG = (1.0 - blend_ratio) * shiftedG + blend_ratio * origTargetG;
                    int finalB = (1.0 - blend_ratio) * shiftedB + blend_ratio * origTargetB;

                    // 防禦性寫法：防止色彩爆出 [0, 255] 區間
                    finalR = max(0, min(255, finalR));
                    finalG = max(0, min(255, finalG));
                    finalB = max(0, min(255, finalB));

                    target->setRGB(bx + tx, by + ty, finalR, finalG, finalB);
                }
            }
        }
    }
}