#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>

#include "KICachePolicy.h"
#include "KLfuCache.h"
#include "KLruCache.h"
#include "KArcCache/KArcCache.h"

class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// 辅助函数：打印结果
void printResults(const std::string& testName, int capacity, 
                 const std::vector<int>& get_operations, 
                 const std::vector<int>& hits) {
    std::cout << "缓存大小: " << capacity << std::endl;
    std::cout << "LRU - 命中率: " << std::fixed << std::setprecision(2) 
              << (100.0 * hits[0] / get_operations[0]) << "%" << std::endl;
    std::cout << "LFU - 命中率: " << std::fixed << std::setprecision(2) 
              << (100.0 * hits[1] / get_operations[1]) << "%" << std::endl;
    std::cout << "ARC - 命中率: " << std::fixed << std::setprecision(2) 
              << (100.0 * hits[2] / get_operations[2]) << "%" << std::endl;
    std::cout << "LRU-K - 命中率: " << std::fixed << std::setprecision(2) 
              << (100.0 * hits[3] / get_operations[3]) << "%" << std::endl;
    std::cout << "Hash-LRU - 命中率: " << std::fixed << std::setprecision(2) 
              << (100.0 * hits[4] / get_operations[4]) << "%" << std::endl;
    std::cout << "Hash-LRU-K - 命中率: " << std::fixed << std::setprecision(2) 
              << (100.0 * hits[5] / get_operations[5]) << "%" << std::endl;
    std::cout << "Hash-LFU - 命中率: " << std::fixed << std::setprecision(2) 
              << (100.0 * hits[6] / get_operations[6]) << "%" << std::endl;
}

void testHotDataAccess(const int CAPACITY, const int OPERATIONS,const int HOT_KEYS,const int COLD_KEYS,int sliceNum,const int k1,const int historyCapacity1,const int k2,const int historyCapacity2) {
    std::cout << "\n=== 测试场景1：热点数据访问测试 ===" << std::endl;
    
 
    
    KamaCache::KLruCache<int, std::string> lru(CAPACITY);
    KamaCache::KLfuCache<int, std::string> lfu(CAPACITY);
    KamaCache::KArcCache<int, std::string> arc(CAPACITY);
    KamaCache::KLruKCache<int, std::string> lruk(CAPACITY, historyCapacity1, k1); // 假设 historyCapacity=10, k=3
    KamaCache::KHashLruCaches<int, std::string> hash_lru(CAPACITY, sliceNum); // 假设分片数量=4
    KamaCache::KHashLruKCache<int, std::string> hash_lruk(CAPACITY, 10, historyCapacity2, k2); // 假设 historyCapacity=10, k=3, 分片数量=4
    KamaCache::KHashLfuCache<int, std::string> hash_lfu(CAPACITY, sliceNum); // 假设分片数量=4

    std::random_device rd;
    std::mt19937 gen(rd());
    
    std::array<KamaCache::KICachePolicy<int, std::string>*, 7> caches = {&lru, &lfu, &arc, &lruk, &hash_lru, &hash_lruk, &hash_lfu};
    std::vector<int> hits(7, 0);
    std::vector<int> get_operations(7, 0);

    // 先进行一系列put操作
    for (int i = 0; i < caches.size(); ++i) {
        for (int op = 0; op < OPERATIONS; ++op) {
            int key;
            if (op % 100 < 70) {  // 70%热点数据
                key = gen() % HOT_KEYS;
            } else {  // 30%冷数据
                key = HOT_KEYS + (gen() % COLD_KEYS);
            }
            std::string value = "value" + std::to_string(key);
            caches[i]->put(key, value);
        }
        
        // 然后进行随机get操作
        for (int get_op = 0; get_op < OPERATIONS; ++get_op) {
            int key;
            if (get_op % 100 < 70) {  // 70%概率访问热点
                key = gen() % HOT_KEYS;
            } else {  // 30%概率访问冷数据
                key = HOT_KEYS + (gen() % COLD_KEYS);
            }
            
            std::string result;
            get_operations[i]++;
            if (caches[i]->get(key, result)) {
                hits[i]++;
            }
        }
    }

    printResults("热点数据访问测试", CAPACITY, get_operations, hits);
}

void testLoopPattern(const int CAPACITY, const int LOOP_SIZE,const int OPERATIONS,int sliceNum,const int k1,const int historyCapacity1,const int k2,const int historyCapacity2) {
    std::cout << "\n=== 测试场景2：循环扫描测试 ===" << std::endl;
    
    
    KamaCache::KLruCache<int, std::string> lru(CAPACITY);
    KamaCache::KLfuCache<int, std::string> lfu(CAPACITY);
    KamaCache::KArcCache<int, std::string> arc(CAPACITY);
    KamaCache::KLruKCache<int, std::string> lruk(CAPACITY, historyCapacity1, k1); // 假设 historyCapacity=10, k=3
    KamaCache::KHashLruCaches<int, std::string> hash_lru(CAPACITY, sliceNum); // 假设分片数量=4
    KamaCache::KHashLruKCache<int, std::string> hash_lruk(CAPACITY, 10, historyCapacity2, k2); // 假设 historyCapacity=10, k=3, 分片数量=4
    KamaCache::KHashLfuCache<int, std::string> hash_lfu(CAPACITY, sliceNum); // 假设分片数量=4

    std::array<KamaCache::KICachePolicy<int, std::string>*, 7> caches = {&lru, &lfu, &arc, &lruk, &hash_lru, &hash_lruk, &hash_lfu};
    std::vector<int> hits(7, 0);
    std::vector<int> get_operations(7, 0);

    std::random_device rd;
    std::mt19937 gen(rd());

    // 先填充数据
    for (int i = 0; i < caches.size(); ++i) {
        for (int key = 0; key < LOOP_SIZE; ++key) {  // 只填充 LOOP_SIZE 的数据
            std::string value = "loop" + std::to_string(key);
            caches[i]->put(key, value);
        }
        
        // 然后进行访问测试
        int current_pos = 0;
        for (int op = 0; op < OPERATIONS; ++op) {
            int key;
            if (op % 100 < 60) {  // 60%顺序扫描
                key = current_pos;
                current_pos = (current_pos + 1) % LOOP_SIZE;
            } else if (op % 100 < 90) {  // 30%随机跳跃
                key = gen() % LOOP_SIZE;
            } else {  // 10%访问范围外数据
                key = LOOP_SIZE + (gen() % LOOP_SIZE);
            }
            
            std::string result;
            get_operations[i]++;
            if (caches[i]->get(key, result)) {
                hits[i]++;
            }
        }
    }

    printResults("循环扫描测试", CAPACITY, get_operations, hits);
}

void testWorkloadShift(const int CAPACITY, const int OPERATIONS,const int PHASE_LENGTH,int sliceNum,const int k1,const int historyCapacity1,const int k2,const int historyCapacity2) {
    std::cout << "\n=== 测试场景3：工作负载剧烈变化测试 ===" << std::endl;

    
    KamaCache::KLruCache<int, std::string> lru(CAPACITY);
    KamaCache::KLfuCache<int, std::string> lfu(CAPACITY);
    KamaCache::KArcCache<int, std::string> arc(CAPACITY);
    KamaCache::KLruKCache<int, std::string> lruk(CAPACITY, historyCapacity1, k1); // 假设 historyCapacity=10, k=3
    KamaCache::KHashLruCaches<int, std::string> hash_lru(CAPACITY, sliceNum); // 假设分片数量=4
    KamaCache::KHashLruKCache<int, std::string> hash_lruk(CAPACITY, 10, historyCapacity2, k2); // 假设 historyCapacity=10, k=3, 分片数量=4
    KamaCache::KHashLfuCache<int, std::string> hash_lfu(CAPACITY, sliceNum); // 假设分片数量=4

    std::random_device rd;
    std::mt19937 gen(rd());
    std::array<KamaCache::KICachePolicy<int, std::string>*, 7> caches = {&lru, &lfu, &arc, &lruk, &hash_lru, &hash_lruk, &hash_lfu};
    std::vector<int> hits(7, 0);
    std::vector<int> get_operations(7, 0);

    // 先填充一些初始数据
    for (int i = 0; i < caches.size(); ++i) {
        for (int key = 0; key < 1000; ++key) {
            std::string value = "init" + std::to_string(key);
            caches[i]->put(key, value);
        }
        
        // 然后进行多阶段测试
        for (int op = 0; op < OPERATIONS; ++op) {
            int key;
            // 根据不同阶段选择不同的访问模式
            if (op < PHASE_LENGTH) {  // 热点访问
                key = gen() % 5;
            } else if (op < PHASE_LENGTH * 2) {  // 大范围随机
                key = gen() % 1000;
            } else if (op < PHASE_LENGTH * 3) {  // 顺序扫描
                key = (op - PHASE_LENGTH * 2) % 100;
            } else if (op < PHASE_LENGTH * 4) {  // 局部性随机
                int locality = (op / 1000) % 10;
                key = locality * 20 + (gen() % 20);
            } else {  // 混合访问
                int r = gen() % 100;
                if (r < 30) {
                    key = gen() % 5;
                } else if (r < 60) {
                    key = 5 + (gen() % 95);
                } else {
                    key = 100 + (gen() % 900);
                }
            }
            
            std::string result;
            get_operations[i]++;
            if (caches[i]->get(key, result)) {
                hits[i]++;
            }
            
            // 随机进行put操作，更新缓存内容
            if (gen() % 100 < 30) {  // 30%概率进行put
                std::string value = "new" + std::to_string(key);
                caches[i]->put(key, value);
            }
        }
    }

    printResults("工作负载剧烈变化测试", CAPACITY, get_operations, hits);
}

void test1() {
    std::cout<<std::endl<<"测试用例 1 - 标准负载"<<std::endl;

    testHotDataAccess(50, 500000, 20, 5000, 4, 3, 10, 5, 10);  // 热点数据访问测试
    testLoopPattern(50, 500, 200000, 4, 3, 10, 5, 10);          // 循环扫描测试
    testWorkloadShift(50, 80000, 16000, 4, 3, 10, 5, 10);       // 工作负载变化测试
}

void test2() {
    std::cout<<std::endl<<"测试用例 2 - 小容量高操作"<<std::endl;

    testHotDataAccess(20, 1000000, 50, 10000, 4, 3, 5, 3, 5);  // 热点数据访问测试
    testLoopPattern(20, 200, 100000, 4, 3, 5, 3, 5);           // 循环扫描测试
    testWorkloadShift(20, 60000, 12000, 4, 3, 5, 3, 5);        // 工作负载变化测试
}

void test3() {
    std::cout<<std::endl<<"测试用例 3 - 大容量低操作"<<std::endl;

    testHotDataAccess(200, 200000, 40, 3000, 4, 4, 15, 6, 15);  // 热点数据访问测试
    testLoopPattern(200, 1000, 50000, 4, 4, 15, 6, 15);         // 循环扫描测试
    testWorkloadShift(200, 30000, 6000, 4, 4, 15, 6, 15);       // 工作负载变化测试
}

void test4() {
    std::cout<<std::endl<<"测试用例 4 - 高热点比例"<<std::endl;

    testHotDataAccess(100, 500000, 100, 2000, 4, 3, 10, 5, 10);  // 热点数据访问测试
    testLoopPattern(100, 500, 200000, 4, 3, 10, 5, 10);          // 循环扫描测试
    testWorkloadShift(100, 80000, 16000, 4, 3, 10, 5, 10);       // 工作负载变化测试
}

void test5() {
    std::cout<<std::endl<<"测试用例 5 - 高冷数据比例"<<std::endl;

    testHotDataAccess(50, 500000, 10, 10000, 4, 3, 10, 5, 10);   // 热点数据访问测试
    testLoopPattern(50, 500, 200000, 4, 3, 10, 5, 10);           // 循环扫描测试
    testWorkloadShift(50, 80000, 16000, 4, 3, 10, 5, 10);        // 工作负载变化测试
}

void test6() {
    std::cout<<std::endl<<"测试用例 6 - 大容量高操作"<<std::endl;

    testHotDataAccess(500, 1000000, 100, 10000, 4, 4, 20, 7, 20);  // 热点数据访问测试
    testLoopPattern(500, 2000, 100000, 4, 4, 20, 7, 20);          // 循环扫描测试
    testWorkloadShift(500, 40000, 8000, 4, 4, 20, 7, 20);         // 工作负载变化测试
}

void test7() {
    std::cout<<std::endl<<"测试用例 7 - 高频率变化"<<std::endl;

    testHotDataAccess(50, 500000, 50, 2000, 4, 2, 5, 3, 5);  // 热点数据访问测试
    testLoopPattern(50, 500, 500000, 4, 2, 5, 3, 5);         // 循环扫描测试
    testWorkloadShift(50, 200000, 50000, 4, 2, 5, 3, 5);     // 工作负载变化测试
}

void test8() {
    std::cout<<std::endl<<"测试用例 8 - 长时间访问测试"<<std::endl;

    testHotDataAccess(150, 1000000, 30, 5000, 4, 5, 15, 7, 15);   // 热点数据访问测试
    testLoopPattern(150, 1000, 500000, 4, 5, 15, 7, 15);           // 循环扫描测试
    testWorkloadShift(150, 200000, 50000, 4, 5, 15, 7, 15);        // 工作负载变化测试
}

void test9() {
    std::cout<<std::endl<<"测试用例 9 - 高并发访问"<<std::endl;
    testHotDataAccess(200, 1000000, 50, 5000, 8, 3, 10, 5, 10);  // 热点数据访问测试
    testLoopPattern(200, 1000, 100000, 8, 3, 10, 5, 10);         // 循环扫描测试
    testWorkloadShift(200, 200000, 50000, 8, 3, 10, 5, 10);       // 工作负载变化测试
}

void test10() {
    std::cout<<std::endl<<"测试用例 10 - 更长操作次数与更低容量"<<std::endl;
    testHotDataAccess(30, 2000000, 20, 2000, 4, 3, 8, 4, 8);  // 热点数据访问测试
    testLoopPattern(30, 500, 2000000, 4, 3, 8, 4, 8);         // 循环扫描测试
    testWorkloadShift(30, 1000000, 250000, 4, 3, 8, 4, 8);     // 工作负载变化测试
}


int main() {
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();

    return 0;
}
