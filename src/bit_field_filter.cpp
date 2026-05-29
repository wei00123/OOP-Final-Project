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

    // 控制清晰度的鍵門檻值（通常設定在 50 ~ 100 之間）
    // 數字越低：抓到的線條越多、越細節；數字越高：線條越乾淨、越銳利
    int threshold = 120; 

    for (int y = 1; y < h - 1; y++){
        for (int x = 1; x < w - 1; x++){
            int gx = 0, gy = 0;
            
            for (int dy = -1; dy <= 1; dy++){
                for (int dx = -1; dx <= 1; dx++){
                    int r = img->getR(x + dx, y + dy);
                    int g = img->getG(x + dx, y + dy);
                    int b = img->getB(x + dx, y + dy);
                    
                    // 【修改 1】利用工業標準公式，將 RGB 轉為單一灰階亮度 (Gray = 0.299R + 0.587G + 0.114B)
                    int gray = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);
                    
                    gx += gray * Gx[dy+1][dx+1];
                    gy += gray * Gy[dy+1][dx+1];
                }
            }
            
            // 使用標準歐幾里德距離 (sqrt) 計算梯度總強度，比單純相加絕對值更平滑準確
            int magnitude = static_cast<int>(std::sqrt(gx * gx + gy * gy));
            
            // 超過門檻就是白色線條(255)，否則就是黑色背景(0)
            int final_val = (magnitude > threshold) ? 255 : 0;
            
            // R, G, B 三個通道都填入相同數值，確保影像絕對是純黑白
            temp[y][x][0] = final_val; 
            temp[y][x][1] = final_val; 
            temp[y][x][2] = final_val;
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
            minR = min(minR, img->getR(x,y)); maxR = max(maxR, img->getR(x,y));
            minG = min(minG, img->getG(x,y)); maxG = max(maxG, img->getG(x,y));
            minB = min(minB, img->getB(x,y)); maxB = max(maxB, img->getB(x,y));
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