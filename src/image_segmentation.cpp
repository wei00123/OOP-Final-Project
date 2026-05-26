#include "image_segmentation.h"
#include "gray_image.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <queue>   // 👈 新增：用於 BFS 廣度優先搜尋
#include <vector>  // 👈 新增：用於儲存座標與拜訪紀錄

using namespace std;

ImageSegmentation::ImageSegmentation(int tol) : tolerance(tol) {}
ImageSegmentation::~ImageSegmentation() {}

void ImageSegmentation::Segment(RGBImage* src, RGBImage*& subject, RGBImage*& background) {
    if (!src) return;
    int w = src->get_width();
    int h = src->get_height();
    subject = new RGBImage(w, h);
    background = new RGBImage(w, h);

    // 1. 自動採樣背景色
    int cornersX[4] = {0, w - 1, 0, w - 1};
    int cornersY[4] = {0, 0, h - 1, h - 1};
    long long bgR_sum = 0, bgG_sum = 0, bgB_sum = 0;
    for (int i = 0; i < 4; i++) {
        bgR_sum += src->getR(cornersX[i], cornersY[i]);
        bgG_sum += src->getG(cornersX[i], cornersY[i]);
        bgB_sum += src->getB(cornersX[i], cornersY[i]);
    }
    int bgR = bgR_sum / 4, bgG = bgG_sum / 4, bgB = bgB_sum / 4;

    // 2. 雙遮罩決策系統：產生初始遮罩
    int tol_for_hole = tolerance - 20; 
    int tol_for_edge = tolerance + 20; 
    GrayImage* final_mask = new GrayImage(w, h);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            long long dR = src->getR(x, y) - bgR;
            long long dG = src->getG(x, y) - bgG;
            long long dB = src->getB(x, y) - bgB;
            double dist = std::sqrt(dR * dR + dG * dG + dB * dB);

            bool is_subject_by_hole = (dist >= tol_for_hole); 
            bool is_subject_by_edge = (dist >= tol_for_edge); 

            if (is_subject_by_hole) {
                (*final_mask)(x, y) = 255;
            } else if (!is_subject_by_edge) {
                (*final_mask)(x, y) = 0;
            } else {
                (*final_mask)(x, y) = 0;
            }
        }
    }

    // =================================================================
    // 3. 核心升級：尋找最大連通區塊 (Largest Connected Component)
    // =================================================================
    size_t max_area = 0;
    vector<pair<int, int>> largest_component;
    
    // 建立二維 boolean 陣列，紀錄該像素是否已經走訪過
    vector<vector<bool>> visited(h, vector<bool>(w, false));

    // 掃描整張圖片尋找白色的主體區塊
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // 如果找到白色的像素且還沒被走訪過，代表發現一個新區塊！
            if ((*final_mask)(x, y) == 255 && !visited[y][x]) {
                vector<pair<int, int>> current_component;
                queue<pair<int, int>> q;
                
                // 開始 BFS (廣度優先搜尋)
                q.push({x, y});
                visited[y][x] = true;

                while (!q.empty()) {
                    auto p = q.front();
                    q.pop();
                    current_component.push_back(p);

                    // 檢查上下左右四個方向
                    int dx[] = {1, -1, 0, 0};
                    int dy[] = {0, 0, 1, -1};
                    for(int i = 0; i < 4; i++) {
                        int nx = p.first + dx[i];
                        int ny = p.second + dy[i];
                        
                        // 確保不越界
                        if(nx >= 0 && nx < w && ny >= 0 && ny < h) {
                            if((*final_mask)(nx, ny) == 255 && !visited[ny][nx]) {
                                visited[ny][nx] = true;
                                q.push({nx, ny});
                            }
                        }
                    }
                }
                
                // BFS 走訪完畢，這一個區塊的面積如果大於歷史最大值，就把它記錄下來
                if (current_component.size() > max_area) {
                    max_area = current_component.size();
                    largest_component = std::move(current_component);
                }
            }
        }
    }

    // 將遮罩「全部清空」變為黑色
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            (*final_mask)(x, y) = 0;
        }
    }

    // 只把面積「最大」的那個區塊重新塗白
    for (auto& p : largest_component) {
        (*final_mask)(p.first, p.second) = 255;
    }

    // =================================================================
    // 4. 像素分流 (只會處理最終保留的最大區塊)
    // =================================================================
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if ((*final_mask)(x, y) == 255) {
                subject->setRGB(x, y, src->getR(x, y), src->getG(x, y), src->getB(x, y));
                background->setRGB(x, y, 0, 0, 0);
            } else {
                background->setRGB(x, y, src->getR(x, y), src->getG(x, y), src->getB(x, y));
                subject->setRGB(x, y, 0, 0, 0);
            }
        }
    }

    // 清理記憶體
    delete final_mask;
}