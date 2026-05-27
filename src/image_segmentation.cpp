#include "image_segmentation.h"
#include "gray_image.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <queue>
#include <vector>

using namespace std;

ImageSegmentation::ImageSegmentation(int tol): tolerance(tol){}
ImageSegmentation::~ImageSegmentation(){}

void ImageSegmentation::Segment(RGBImage* src, RGBImage*& subject, RGBImage*& background) {
    if (!src) return;
    int w = src->get_width();
    int h = src->get_height();

    // 配置記憶體給輸出的主體與背景圖
    subject = new RGBImage(w, h);
    background = new RGBImage(w, h);

    // 自動採樣背景色
    int cornersX[4] = {0, w - 1, 0, w - 1};
    int cornersY[4] = {0, 0, h - 1, h - 1};
    long long bgR_sum = 0, bgG_sum = 0, bgB_sum = 0;
    for (int i = 0; i < 4; i++){
        bgR_sum += src->getR(cornersX[i], cornersY[i]);
        bgG_sum += src->getG(cornersX[i], cornersY[i]);
        bgB_sum += src->getB(cornersX[i], cornersY[i]);
    }
    int bgR = bgR_sum / 4, bgG = bgG_sum / 4, bgB = bgB_sum / 4;

    // 將向素分為三類：確定主體(255)、確定背景(0)、模糊地帶(128)
    int tol_for_hole = tolerance - 20; // 低閥值
    int tol_for_edge = tolerance + 20; // 高閥值
    GrayImage* final_mask = new GrayImage(w, h);

    for(int y = 0; y < h; y++){
        for(int x = 0; x < w; x++){
            long long dR = src->getR(x, y) - bgR;
            long long dG = src->getG(x, y) - bgG;
            long long dB = src->getB(x, y) - bgB;
            double dist = sqrt(dR * dR + dG * dG + dB * dB);

            if(dist >= tol_for_edge){
                (*final_mask)(x, y) = 255; // 判斷為確定主體
            }
            else if(dist < tol_for_hole) {
                (*final_mask)(x, y) = 0;   // 判斷為確定背景
            }
            else{
                (*final_mask)(x, y) = 128; // 判斷為模糊地帶
            }
        }
    }

    // BFS尋找最大連通區塊，並且把它當作真正的主體
    size_t max_area = 0; // 用來記錄目前找到的最大主體的像素數量
    vector<pair<int, int>> largest_component; // 用來儲存最大主體的所有(x, y)座標
    vector<vector<bool>> visited(h, vector<bool>(w, false));// 建立一個陣列記錄每個像素是否已經被搜尋過

    for(int y = 0; y < h; y++){
        for(int x = 0; x < w; x++){
            // 必須是絕對是主體才允許初始化一個新區塊
            if ((*final_mask)(x, y) == 255 && !visited[y][x]) {
                vector<pair<int, int>> current_component; // 暫存這塊大陸的所有座標
                queue<pair<int, int>> q; // BFS需要用的佇列
                
                q.push({x, y}); // 將起點加入佇列
                visited[y][x] = true; // 標記為已拜訪

                // 開始廣度搜尋
                while (!q.empty()){
                    auto p = q.front();
                    q.pop();
                    current_component.push_back(p); // 加入佇列中

                    // 定義上下左右四個方向的位移量
                    int dx[] = {1, -1, 0, 0};
                    int dy[] = {0, 0, 1, -1};

                    // 檢查相鄰的四個格子
                    for(int i = 0; i < 4; i++){
                        int nx = p.first + dx[i];
                        int ny = p.second + dy[i];
                        
                        if(nx >= 0 && nx < w && ny >= 0 && ny < h) { // 確保座標沒有超出圖片邊界
                            int neighbor_val = (*final_mask)(nx, ny);
                            // 只有當相鄰像素是確定主體(255)或模糊地帶(128)，且尚未被拜訪過，才將它加入佇列繼續搜尋
                            if((neighbor_val == 255 || neighbor_val == 128) && !visited[ny][nx]){
                                visited[ny][nx] = true;
                                q.push({nx, ny});
                            }
                        }
                    }
                }
                
                // BFS結束後，檢查這塊新找到的主體是不是目前最大的
                if(current_component.size() > max_area){
                    max_area = current_component.size();
                    largest_component = move(current_component);
                }
            }
        }
    }

    // 先將遮罩圖層全部清空抹黑
    for(int y = 0; y < h; y++){
        for(int x = 0; x < w; x++){
            (*final_mask)(x, y) = 0;
        }
    }

    // 只把最終面積最大的主體圖層重繪為白色(255)
    for(auto& p : largest_component){
        (*final_mask)(p.first, p.second) = 255;
    }

    // 最後將原圖的像素分配到主體圖與背景圖中
    for(int y = 0; y < h; y++){
        for(int x = 0; x < w; x++){
            if((*final_mask)(x, y) == 255){
                subject->setRGB(x, y, src->getR(x, y), src->getG(x, y), src->getB(x, y));
                background->setRGB(x, y, 0, 0, 0);
            }
            else{
                background->setRGB(x, y, src->getR(x, y), src->getG(x, y), src->getB(x, y));
                subject->setRGB(x, y, 0, 0, 0);
            }
        }
    }

    delete final_mask;
}