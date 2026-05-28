#include "cuda_for_photo_mosaic.h"
#include <iostream>
#include <algorithm>
#include <cuda_runtime.h>
#include <pthread.h>

using namespace std;

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            cerr << "CUDA Error: " << cudaGetErrorString(err) << " at line " << __LINE__ << endl; \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

struct GPUTileInfo {
    int avgR, avgG, avgB;
    int db_index; 
};

CUDA_for_Photo_Mosaic::CUDA_for_Photo_Mosaic(int size) : Photo_Mosaic(size) {}
CUDA_for_Photo_Mosaic::~CUDA_for_Photo_Mosaic() {}

// CUDA Kernel: 為每個像素分配一個執行緒，根據預先計算的最佳磚索引從對應的磚圖像中取色
__global__ void mosaicKernel(unsigned char* targetR, unsigned char* targetG, unsigned char* targetB,
                             unsigned char* tilesR, unsigned char* tilesG, unsigned char* tilesB,
                             GPUTileInfo* d_tile_info, int num_tiles,
                             int width, int height, int tileSize, int* block_best_indices) 
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height) return;

    int bx = x / tileSize;
    int by = y / tileSize;
    int num_blocks_x = (width + tileSize - 1) / tileSize;
    int block_idx = by * num_blocks_x + bx;

    int best_tile_idx = block_best_indices[block_idx];

    int tx = x % tileSize;
    int ty = y % tileSize;

    int pixel_idx_in_target = y * width + x;
    int pixel_idx_in_tile = best_tile_idx * (tileSize * tileSize) + ty * tileSize + tx;

    targetR[pixel_idx_in_target] = tilesR[pixel_idx_in_tile];
    targetG[pixel_idx_in_target] = tilesG[pixel_idx_in_tile];
    targetB[pixel_idx_in_target] = tilesB[pixel_idx_in_tile];
}

void* CUDA_for_Photo_Mosaic::PreprocessWorker(void* arg) {
    PreprocessArg* p_arg = (PreprocessArg*)arg;
    CUDA_for_Photo_Mosaic* mosaic = p_arg->instance;
    RGBImage* target = p_arg->target;
    int* h_best_idx = p_arg->h_block_best_indices;
    
    int width = p_arg->width;
    int height = p_arg->height;
    int num_blocks_x = p_arg->num_blocks_x;
    int num_tiles = mosaic->tile_db.size();
    int size = mosaic->tileSize;

    // 每個執行緒精準處理分配到的 block 列範圍
    for (int by = p_arg->start_row; by < p_arg->end_row; by++) {
        for (int bx = 0; bx < num_blocks_x; bx++) {
            int px = bx * size;
            int py = by * size;
            int currentW = min(size, width - px);
            int currentH = min(size, height - py);

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

            int best_idx = 0;
            long long min_dist = -1;
            for (int i = 0; i < num_tiles; i++) {
                long long dr = tarR - mosaic->tile_db[i].avgR;
                long long dg = tarG - mosaic->tile_db[i].avgG;
                long long db = tarB - mosaic->tile_db[i].avgB;
                long long dist = dr*dr + dg*dg + db*db;
                if (min_dist == -1 || dist < min_dist) {
                    min_dist = dist;
                    best_idx = i;
                }
            }
            h_best_idx[by * num_blocks_x + bx] = best_idx;
        }
    }

    delete p_arg;
    return nullptr;
}

void CUDA_for_Photo_Mosaic::Process(RGBImage *target) {
    if(tile_db.empty()) return;

    int width = target->get_width();
    int height = target->get_height();
    int num_tiles = tile_db.size();

    int num_blocks_x = (width + tileSize - 1) / tileSize;
    int num_blocks_y = (height + tileSize - 1) / tileSize;
    int total_blocks = num_blocks_x * num_blocks_y;

    cout << "Calculating block color matching using Pthreads...\n";

    int* h_block_best_indices = new int[total_blocks];
    
    int num_threads = 4; 
    pthread_t threads[num_threads];

    for(int i = 0; i < num_threads; i++) {
        // 🛠️ 修正 1：改用 Heap（new）配置參數，確保多執行緒執行期間生命週期安全
        PreprocessArg* arg = new PreprocessArg();
        arg->instance = this;
        arg->target = target;
        arg->h_block_best_indices = h_block_best_indices;
        arg->num_blocks_x = num_blocks_x;
        arg->num_blocks_y = num_blocks_y;
        arg->width = width;
        arg->height = height;
        
        // 區間切分邏輯
        arg->start_row = i * num_blocks_y / num_threads;
        arg->end_row = (i + 1) * num_blocks_y / num_threads;

        pthread_create(&threads[i], nullptr, CUDA_for_Photo_Mosaic::PreprocessWorker, arg);
    }

    // 等待所有 CPU 預處理執行緒完成
    for(int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
    }

    // GPU 記憶體配置與資料扁平化
    int img_pixels = width * height;
    int tile_pixels = tileSize * tileSize;

    unsigned char* h_targetR = new unsigned char[img_pixels];
    unsigned char* h_targetG = new unsigned char[img_pixels];
    unsigned char* h_targetB = new unsigned char[img_pixels];
    
    unsigned char* h_tilesR = new unsigned char[num_tiles * tile_pixels];
    unsigned char* h_tilesG = new unsigned char[num_tiles * tile_pixels];
    unsigned char* h_tilesB = new unsigned char[num_tiles * tile_pixels];

    // 資料扁平化拷貝
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

    CUDA_CHECK(cudaMemcpy(d_tilesR, h_tilesR, num_tiles * tile_pixels, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_tilesG, h_tilesG, num_tiles * tile_pixels, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_tilesB, h_tilesB, num_tiles * tile_pixels, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_block_best_indices, h_block_best_indices, total_blocks * sizeof(int), cudaMemcpyHostToDevice));

    // 啟動 CUDA Kernel
    dim3 blockSize(16, 16);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x, (height + blockSize.y - 1) / blockSize.y);

    mosaicKernel<<<gridSize, blockSize>>>(d_targetR, d_targetG, d_targetB,
                                         d_tilesR, d_tilesG, d_tilesB,
                                         nullptr, num_tiles, width, height, tileSize,
                                         d_block_best_indices);
    
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(h_targetR, d_targetR, img_pixels, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_targetG, d_targetG, img_pixels, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h_targetB, d_targetB, img_pixels, cudaMemcpyDeviceToHost));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            target->setRGB(x, y, h_targetR[idx], h_targetG[idx], h_targetB[idx]);
        }
    }

    // 釋放記憶體
    CUDA_CHECK(cudaFree(d_targetR)); CUDA_CHECK(cudaFree(d_targetG)); CUDA_CHECK(cudaFree(d_targetB));
    CUDA_CHECK(cudaFree(d_tilesR));  CUDA_CHECK(cudaFree(d_tilesG));  CUDA_CHECK(cudaFree(d_tilesB));
    CUDA_CHECK(cudaFree(d_block_best_indices));

    delete[] h_targetR; delete[] h_targetG; delete[] h_targetB;
    delete[] h_tilesR;  delete[] h_tilesG;  delete[] h_tilesB;
    delete[] h_block_best_indices;

    cout << "CUDA Photo Mosaic completed (with Pthread Preprocessing)!\n";
}