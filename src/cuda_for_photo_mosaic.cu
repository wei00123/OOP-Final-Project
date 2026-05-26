#include "cuda_for_photo_mosaic.h"
#include <iostream>
#include <algorithm>
#include <cuda_runtime.h>

using namespace std;

// CUDA 錯誤檢查巨集
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            cerr << "CUDA Error: " << cudaGetErrorString(err) << " at line " << __LINE__ << endl; \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

// 磁磚的輕量結構體，方便傳給 GPU
struct GPUTileInfo {
    int avgR, avgG, avgB;
    int db_index; 
};

CUDA_for_Photo_Mosaic::CUDA_for_Photo_Mosaic(int size) : tileSize(size){}

CUDA_for_Photo_Mosaic::~CUDA_for_Photo_Mosaic(){
    for (auto &tile : tile_db){
        delete tile.tile_img;
    }
    tile_db.clear();
}

// 保持與原版一致，用於 CPU 端初始化
void CUDA_for_Photo_Mosaic::CalculateAverageColor(RGBImage* img, int &r, int &g, int &b){
    if (!img) return;
    long long sumR = 0, sumG = 0, sumB = 0;
    int w = img->get_width();
    int h = img->get_height();
    if (w <= 0 || h <= 0) { r = 0; g = 0; b = 0; return; }

    for (int y = 0; y < h; y++){
        for (int x = 0; x < w; x++){
            sumR += img->getR(x, y);
            sumG += img->getG(x, y);
            sumB += img->getB(x, y);
        }
    }
    r = (int)(sumR / (w * h));
    g = (int)(sumG / (w * h));
    b = (int)(sumB / (w * h));
}

void CUDA_for_Photo_Mosaic::LoadTiles(string folder_path){
    vector<string> all_files;
    Data_Loader loader;
    if(loader.List_Directory(folder_path, all_files) != 0) return;

    for(const string& full_path : all_files){
        string ext = "";
        size_t dot_idx = full_path.find_last_of(".");
        if(dot_idx != string::npos){
            ext = full_path.substr(dot_idx + 1);
            for(auto &c : ext) c = tolower(c);
        }
        if(ext == "jpg" || ext == "jpeg" || ext == "png"){
            RGBImage* raw_tile = new RGBImage();
            if(raw_tile->LoadImage(full_path)){
                RGBImage* resized_tile = raw_tile->Resize(tileSize, tileSize);
                if (resized_tile == nullptr) { delete raw_tile; continue; }
                int ar, ag, ab;
                CalculateAverageColor(resized_tile, ar, ag, ab);
                TileInfo info = {resized_tile, ar, ag, ab};
                tile_db.push_back(info);
            }
            delete raw_tile; 
        }
    }
    cout << "\n[CUDA] 磁磚載入完成，共 " << tile_db.size() << " 張。\n";
}

// =================================================================
// 3. CUDA Kernel：平行處理每個像素的著色
// =================================================================
__global__ void mosaicKernel(unsigned char* targetR, unsigned char* targetG, unsigned char* targetB,
                             unsigned char* tilesR, unsigned char* tilesG, unsigned char* tilesB,
                             GPUTileInfo* d_tile_info, int num_tiles,
                             int width, int height, int tileSize, int* block_best_indices) 
{
    // 算出當前 Thread 負責的像素坐標 (x, y)
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height) return;

    // 1. 找出這個像素屬於哪一個馬賽克區塊 (Block Coordinates)
    int bx = x / tileSize;
    int by = y / tileSize;
    int num_blocks_x = (width + tileSize - 1) / tileSize;
    int block_idx = by * num_blocks_x + bx;

    // 2. 取得該區塊應該使用的最佳磁磚索引
    int best_tile_idx = block_best_indices[block_idx];

    // 3. 算出當前像素在該磁磚內的相對坐標 (tx, ty)
    int tx = x % tileSize;
    int ty = y % tileSize;

    // 4. 從大水庫中取出該磁磚對應坐標的顏色，寫入目標圖片
    int pixel_idx_in_target = y * width + x;
    int pixel_idx_in_tile = best_tile_idx * (tileSize * tileSize) + ty * tileSize + tx;

    targetR[pixel_idx_in_target] = tilesR[pixel_idx_in_tile];
    targetG[pixel_idx_in_target] = tilesG[pixel_idx_in_tile];
    targetB[pixel_idx_in_target] = tilesB[pixel_idx_in_tile];
}

// =================================================================
// 4. Host 控制端：配置 GPU 記憶體與啟動 Kernel
// =================================================================
void CUDA_for_Photo_Mosaic::Process(RGBImage *target) {
    if(tile_db.empty()) return;

    int width = target->get_width();
    int height = target->get_height();
    int num_tiles = tile_db.size();

    int num_blocks_x = (width + tileSize - 1) / tileSize;
    int num_blocks_y = (height + tileSize - 1) / tileSize;
    int total_blocks = num_blocks_x * num_blocks_y;

    cout << "CUDA 加速中：計算區塊色彩匹配...\n";

    // ====== [CPU 端先行作業] 預先算出每個 Block 的最佳拼貼磁磚 ======
    // (這段若移至 GPU 會涉及大量的 Reduction 運算，在 CPU 算反而乾淨高效)
    int* h_block_best_indices = new int[total_blocks];
    
    for (int by = 0; by < num_blocks_y; by++) {
        for (int bx = 0; bx < num_blocks_x; bx++) {
            int px = bx * tileSize;
            int py = by * tileSize;
            int currentW = min(tileSize, width - px);
            int currentH = min(tileSize, height - py);

            // 算圖塊平均色
            long long sumR = 0, sumG = 0, sumB = 0;
            for (int y = 0; y < currentH; y++) {
                for (int x = 0; x < currentW; x++) {
                    sumR += target->getR(px + x, py + y);
                    sumG += target->getG(px + x, py + y);
                    sumB += target->getB(px + x, py + y);
                }
            }
            int count = currentW * currentH;
            int tarR = sumR / count, tarG = sumG / count, tarB = sumB / count;

            // 比對最接近的磁磚
            int best_idx = 0;
            long long min_dist = -1;
            for (int i = 0; i < num_tiles; i++) {
                long long dr = tarR - tile_db[i].avgR;
                long long dg = tarG - tile_db[i].avgG;
                long long db = tarB - tile_db[i].avgB;
                long long dist = dr*dr + dg*dg + db*db;
                if (min_dist == -1 || dist < min_dist) {
                    min_dist = dist;
                    best_idx = i;
                }
            }
            h_block_best_indices[by * num_blocks_x + bx] = best_idx;
        }
    }

    // ====== [GPU 記憶體配置與資料扁平化] ======
    int img_pixels = width * height;
    int tile_pixels = tileSize * tileSize;

    // 分配 Host 攤平記憶體
    unsigned char* h_targetR = new unsigned char[img_pixels];
    unsigned char* h_targetG = new unsigned char[img_pixels];
    unsigned char* h_targetB = new unsigned char[img_pixels];
    
    unsigned char* h_tilesR = new unsigned char[num_tiles * tile_pixels];
    unsigned char* h_tilesG = new unsigned char[num_tiles * tile_pixels];
    unsigned char* h_tilesB = new unsigned char[num_tiles * tile_pixels];

    // 填入資料
    for(int i=0; i<img_pixels; i++) {
        h_targetR[i] = target->getR(i % width, i / width);
        h_targetG[i] = target->getG(i % width, i / width);
        h_targetB[i] = target->getB(i % width, i / width);
    }

    for(int t=0; t<num_tiles; t++) {
        RGBImage* img = tile_db[t].tile_img;
        for(int p=0; p<tile_pixels; p++) {
            h_tilesR[t * tile_pixels + p] = img->getR(p % tileSize, p / tileSize);
            h_tilesG[t * tile_pixels + p] = img->getG(p % tileSize, p / tileSize);
            h_tilesB[t * tile_pixels + p] = img->getB(p % tileSize, p / tileSize);
        }
    }

    // 分配 GPU 顯示記憶體 (VRAM)
    unsigned char *d_targetR, *d_targetG, *d_targetB;
    unsigned char *d_tilesR, *d_tilesG, *d_tilesB;
    int *d_block_best_indices;

    CUDA_CHECK(cudaMalloc(&d_targetR, img_pixels));
    CUDA_CHECK(cudaMalloc(&d_targetG, img_pixels));
    CUDA_CHECK(cudaMalloc(&d_targetB, img_pixels));
    CUDA_CHECK(cudaMalloc(&d_tilesR, num_tiles * tile_pixels));
    CUDA_CHECK(cudaMalloc(&d_tilesG, num_tiles * tile_pixels));
    CUDA_CHECK(cudaMalloc(&d_tilesB, num_tiles * tile_pixels));
    CUDA_CHECK(cudaMalloc(&d_block_best_indices, total_blocks * sizeof(int)));

    // 將資料拷貝至 GPU (Host -> Device)
    CUDA_CHECK(cudaMemcpy(d_tilesR, h_tilesR, num_tiles * tile_pixels, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_tilesG, h_tilesG, num_tiles * tile_pixels, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_tilesB, h_tilesB, num_tiles * tile_pixels, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_block_best_indices, h_block_best_indices, total_blocks * sizeof(int), cudaMemcpyHostToDevice));

    // ====== [啟動 CUDA 影分身軍團] ======
    // 定義 2D Block 與 Grid 尺寸 (每個 Block 內含 16x16 = 256 個執行緒)
    dim3 blockSize(16, 16);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x, (height + blockSize.y - 1) / blockSize.y);

    mosaicKernel<<<gridSize, blockSize>>>(d_targetR, d_targetG, d_targetB,
                                         d_tilesR, d_tilesG, d_tilesB,
                                         nullptr, num_tiles, width, height, tileSize,
                                         d_block_best_indices);
    
    // 等待 GPU 算完
    CUDA_CHECK(cudaDeviceSynchronize());

    // 將結果拷貝回 CPU (Device -> Host)
    CUDA_CHECK(cudaMemcpy(h_targetR, d_targetR, img_pixels, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_targetG, d_targetG, img_pixels, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_targetB, d_targetB, img_pixels, cudaMemcpyDeviceToHost));

    // 回寫回原本的 RGBImage 物件
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            target->setRGB(x, y, h_targetR[idx], h_targetG[idx], h_targetB[idx]);
        }
    }

    // ====== 釋放所有記憶體 ======
    CUDA_CHECK(cudaFree(d_targetR)); CUDA_CHECK(cudaFree(d_targetG)); CUDA_CHECK(cudaFree(d_targetB));
    CUDA_CHECK(cudaFree(d_tilesR));  CUDA_CHECK(cudaFree(d_tilesG));  CUDA_CHECK(cudaFree(d_tilesB));
    CUDA_CHECK(cudaFree(d_block_best_indices));

    delete[] h_targetR; delete[] h_targetG; delete[] h_targetB;
    delete[] h_tilesR;  delete[] h_tilesG;  delete[] h_tilesB;
    delete[] h_block_best_indices;

    cout << "CUDA Photo Mosaic completed!\n";
}