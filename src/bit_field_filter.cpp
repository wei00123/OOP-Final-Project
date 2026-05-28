#include "bit_field_filter.h"
#include <iostream>
#include <cmath>
#include <algorithm>

Bit_Field_Filter::Bit_Field_Filter(){}

Bit_Field_Filter::~Bit_Field_Filter(){}

void Bit_Field_Filter::Filter(RGBImage *img, int bit_field){
    if (img == nullptr) return;

    // 依照 bit_field 的位元決定是否執行對應的演算法
    if (bit_field & 1){
        Apply_Box_Filter(img);
    }
    if (bit_field & 2){
        Apply_Sobel_Gradient(img);
    }
    if (bit_field & 4){
        Apply_Contrast_Stretching(img);
    }
    if (bit_field & 8){
        Apply_Mosaic_Filter(img);
    }
}

void Bit_Field_Filter::Apply_Box_Filter(RGBImage *img){
    int w = img->get_width();
    int h = img->get_height();
    
    // 建立暫存陣列(避免污染原圖)
    int*** temp = new int**[h];
    for(int i=0; i<h; i++){
        temp[i] = new int*[w];
        for(int j=0; j<w; j++) temp[i][j] = new int[3]{0};
    }

    // 執行Box Filter邏輯
    for (int y = 1; y < h - 1; y++){
        for (int x = 1; x < w - 1; x++){
            int sumR = 0, sumG = 0, sumB = 0;
            for (int dy = -1; dy <= 1; dy++){
                for (int dx = -1; dx <= 1; dx++){
                    sumR += img->getR(x + dx, y + dy);
                    sumG += img->getG(x + dx, y + dy);
                    sumB += img->getB(x + dx, y + dy);
                }
            }
            temp[y][x][0] = sumR / 9;
            temp[y][x][1] = sumG / 9;
            temp[y][x][2] = sumB / 9;
        }
    }

    // 寫回原圖並釋放記憶體
    for (int y = 0; y < h; y++){
        for (int x = 0; x < w; x++){
            img->setRGB(x, y, temp[y][x][0], temp[y][x][1], temp[y][x][2]);
            delete[] temp[y][x];
        }
        delete[] temp[y];
    }
    delete[] temp;
}

void Bit_Field_Filter::Apply_Sobel_Gradient(RGBImage *img){
    int w = img->get_width();
    int h = img->get_height();
    
    int*** temp = new int**[h];
    for(int i=0; i<h; i++){
        temp[i] = new int*[w];
        for(int j=0; j<w; j++) temp[i][j] = new int[3]{0};
    }

    // 執行Sobel邏輯
    int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int Gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

    for (int y = 1; y < h - 1; y++){
        for (int x = 1; x < w - 1; x++){
            int rx = 0, ry = 0, gx = 0, gy = 0, bx = 0, by = 0;
            for (int dy = -1; dy <= 1; dy++){
                for (int dx = -1; dx <= 1; dx++){
                    int r = img->getR(x + dx, y + dy);
                    int g = img->getG(x + dx, y + dy);
                    int b = img->getB(x + dx, y + dy);
                    
                    rx += r * Gx[dy+1][dx+1]; ry += r * Gy[dy+1][dx+1];
                    gx += g * Gx[dy+1][dx+1]; gy += g * Gy[dy+1][dx+1];
                    bx += b * Gx[dy+1][dx+1]; by += b * Gy[dy+1][dx+1];
                }
            }
            // 計算近似梯度並限制在 255 以內
            int valR = std::min(255, std::abs(rx) + std::abs(ry));
            int valG = std::min(255, std::abs(gx) + std::abs(gy));
            int valB = std::min(255, std::abs(bx) + std::abs(by));
            
            temp[y][x][0] = valR; temp[y][x][1] = valG; temp[y][x][2] = valB;
        }
    }

    // 寫回原圖並釋放記憶體
    for (int y = 0; y < h; y++){
        for (int x = 0; x < w; x++){
            img->setRGB(x, y, temp[y][x][0], temp[y][x][1], temp[y][x][2]);
            delete[] temp[y][x];
        }
        delete[] temp[y];
    }
    delete[] temp;
}

void Bit_Field_Filter::Apply_Contrast_Stretching(RGBImage *img){
    int w = img->get_width();
    int h = img->get_height();


    int minR = 255, maxR = 0, minG = 255, maxG = 0, minB = 255, maxB = 0;

    // 第一次掃描：找 min 和 max
    for (int y = 0; y < h; y++){
        for (int x = 0; x < w; x++){
            minR = std::min(minR, img->getR(x,y)); maxR = std::max(maxR, img->getR(x,y));
            minG = std::min(minG, img->getG(x,y)); maxG = std::max(maxG, img->getG(x,y));
            minB = std::min(minB, img->getB(x,y)); maxB = std::max(maxB, img->getB(x,y));
        }
    }

    // 第二次掃描：套用公式
    for (int y = 0; y < h; y++){
        for (int x = 0; x < w; x++){
            int r = img->getR(x, y);
            int g = img->getG(x, y);
            int b = img->getB(x, y);

            if (maxR > minR) r = (r - minR) * 255 / (maxR - minR);
            if (maxG > minG) g = (g - minG) * 255 / (maxG - minG);
            if (maxB > minB) b = (b - minB) * 255 / (maxB - minB);

            img->setRGB(x, y, r, g, b);
        }
    }
}

void Bit_Field_Filter::Apply_Mosaic_Filter(RGBImage *img){
    int w = img->get_width(), h = img->get_height();
    int blockSize = 4; // 馬賽克方塊大小

    // 每次跳一個 block 的大小
    for (int by = 0; by < h; by += blockSize){
        for (int bx = 0; bx < w; bx += blockSize){
            int sumR = 0, sumG = 0, sumB = 0;
            int count = 0;

            // 計算這個 block 的顏色總和
            for (int y = by; y < by + blockSize && y < h; y++){
                for (int x = bx; x < bx + blockSize && x < w; x++){
                    sumR += img->getR(x, y);
                    sumG += img->getG(x, y);
                    sumB += img->getB(x, y);
                    count++;
                }
            }

            // 算平均值
            int avgR = sumR / count;
            int avgG = sumG / count;
            int avgB = sumB / count;

            // 把整個block變成平均顏色
            for (int y = by; y < by + blockSize && y < h; y++){
                for (int x = bx; x < bx + blockSize && x < w; x++){
                    img->setRGB(x, y, avgR, avgG, avgB);
                }
            }
        }
    }
}