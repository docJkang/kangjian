#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/time.h>

#define MAX_ITEMS 320000
#define MAX_WEIGHT 100
#define MIN_VALUE 100
#define MAX_VALUE 1000
#define TIMEOUT_MS 900000 // 15分钟超时

typedef struct {
    int id;
    double weight;
    double value;
} Item;

typedef struct {
    double total_value;
    double total_weight;
    bool *selected;
    double execution_time;
    bool timeout;
} Result;

// 生成随机物品数据
void generate_items(Item items[], int n) {
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        items[i].id = i + 1;
        items[i].weight = (rand() % MAX_WEIGHT) + 1;
        items[i].value = MIN_VALUE + (rand() % (MAX_VALUE - MIN_VALUE + 1)) + (rand() % 100) / 100.0;
    }
}

// 将物品数据写入Excel文件
void write_items_to_excel(Item items[], int n, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("无法创建Excel文件\n");
        return;
    }
    
    fprintf(fp, "物品编号,物品重量,物品价值\n");
    for (int i = 0; i < n && i < 1000; i++) {
        fprintf(fp, "%d,%.2f,%.2f\n", items[i].id, items[i].weight, items[i].value);
    }
    
    fclose(fp);
    printf("物品数据已保存到 %s\n", filename);
}

// 将结果写入Excel文件
void write_results_to_excel(Result results[], const char *methods[], int method_count, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("无法创建结果文件\n");
        return;
    }
    
    fprintf(fp, "算法,总价值,总重量,执行时间(ms),是否超时\n");
    for (int i = 0; i < method_count; i++) {
        if (results[i].timeout) {
            fprintf(fp, "%s,数据量过大无法计算,数据量过大无法计算,%.2f,是\n", 
                   methods[i], results[i].execution_time);
        } else {
            fprintf(fp, "%s,%.2f,%.2f,%.2f,否\n", 
                   methods[i], results[i].total_value, results[i].total_weight, results[i].execution_time);
        }
    }
    
    fclose(fp);
    printf("结果数据已保存到 %s\n", filename);
}

// 获取当前时间（毫秒）
long long get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// 检查是否超时
bool check_timeout(long long start_time) {
    long long current_time = get_current_time_ms();
    return (current_time - start_time) > TIMEOUT_MS;
}

// 蛮力法求解0-1背包问题
Result brute_force_knapsack(Item items[], int n, double capacity) {
    Result result = {0};
    result.selected = (bool *)calloc(n, sizeof(bool));
    result.timeout = false;
    
    long long start_time = get_current_time_ms();
    
    double max_value = 0;
    bool *best_selection = (bool *)calloc(n, sizeof(bool));
    
    // 生成所有可能的子集
    unsigned long long total = 1ULL << n;
    
    for (unsigned long long i = 0; i < total; i++) {
        if (check_timeout(start_time)) {
            result.timeout = true;
            free(best_selection);
            return result;
        }
        
        double current_weight = 0;
        double current_value = 0;
        
        for (int j = 0; j < n; j++) {
            if (i & (1ULL << j)) {
                current_weight += items[j].weight;
                current_value += items[j].value;
            }
        }
        
        if (current_weight <= capacity && current_value > max_value) {
            max_value = current_value;
            for (int j = 0; j < n; j++) {
                best_selection[j] = (i & (1ULL << j)) != 0;
            }
        }
    }
    
    result.total_value = max_value;
    result.total_weight = 0;
    for (int i = 0; i < n; i++) {
        if (best_selection[i]) {
            result.total_weight += items[i].weight;
        }
        result.selected[i] = best_selection[i];
    }
    
    free(best_selection);
    result.execution_time = (double)(get_current_time_ms() - start_time);
    
    return result;
}

// 动态规划法求解0-1背包问题
Result dp_knapsack(Item items[], int n, double capacity) {
    Result result = {0};
    result.selected = (bool *)calloc(n, sizeof(bool));
    result.timeout = false;
    
    long long start_time = get_current_time_ms();
    
    // 将容量转换为整数（乘以100保留两位小数）
    int int_capacity = (int)(capacity * 100);
    int *weights = (int *)malloc(n * sizeof(int));
    int *values = (int *)malloc(n * sizeof(int));
    
    for (int i = 0; i < n; i++) {
        weights[i] = (int)(items[i].weight * 100);
        values[i] = (int)(items[i].value * 100);
    }
    
    // 创建DP表
    int **dp = (int **)malloc((n + 1) * sizeof(int *));
    for (int i = 0; i <= n; i++) {
        dp[i] = (int *)malloc((int_capacity + 1) * sizeof(int));
    }
    
    // 初始化DP表
    for (int i = 0; i <= n; i++) {
        for (int w = 0; w <= int_capacity; w++) {
            if (i == 0 || w == 0) {
                dp[i][w] = 0;
            } else if (weights[i - 1] <= w) {
                dp[i][w] = (values[i - 1] + dp[i - 1][w - weights[i - 1]]) > dp[i - 1][w] ? 
                           (values[i - 1] + dp[i - 1][w - weights[i - 1]]) : dp[i - 1][w];
            } else {
                dp[i][w] = dp[i - 1][w];
            }
        }
        
        if (check_timeout(start_time)) {
            result.timeout = true;
            // 释放内存
            for (int i = 0; i <= n; i++) {
                free(dp[i]);
            }
            free(dp);
            free(weights);
            free(values);
            return result;
        }
    }
    
    // 回溯找出选择的物品
    int res = dp[n][int_capacity];
    int w = int_capacity;
    for (int i = n; i > 0 && res > 0; i--) {
        if (res == dp[i - 1][w]) {
            continue;
        } else {
            result.selected[i - 1] = true;
            res -= values[i - 1];
            w -= weights[i - 1];
        }
    }
    
    // 计算总重量和总价值
    result.total_value = dp[n][int_capacity] / 100.0;
    result.total_weight = 0;
    for (int i = 0; i < n; i++) {
        if (result.selected[i]) {
            result.total_weight += items[i].weight;
        }
    }
    
    // 释放内存
    for (int i = 0; i <= n; i++) {
        free(dp[i]);
    }
    free(dp);
    free(weights);
    free(values);
    
    result.execution_time = (double)(get_current_time_ms() - start_time);
    
    return result;
}

// 比较函数，用于贪心算法的排序
int compare_ratio(const void *a, const void *b) {
    Item *item_a = (Item *)a;
    Item *item_b = (Item *)b;
    double ratio_a = item_a->value / item_a->weight;
    double ratio_b = item_b->value / item_b->weight;
    
    if (ratio_a > ratio_b) return -1;
    if (ratio_a < ratio_b) return 1;
    return 0;
}

// 贪心法求解0-1背包问题
Result greedy_knapsack(Item items[], int n, double capacity) {
    Result result = {0};
    result.selected = (bool *)calloc(n, sizeof(bool));
    result.timeout = false;
    
    long long start_time = get_current_time_ms();
    
    // 复制物品数组，避免修改原始数据
    Item *sorted_items = (Item *)malloc(n * sizeof(Item));
    memcpy(sorted_items, items, n * sizeof(Item));
    
    // 按价值/重量比排序
    qsort(sorted_items, n, sizeof(Item), compare_ratio);
    
    double current_weight = 0;
    double current_value = 0;
    
    for (int i = 0; i < n; i++) {
        if (check_timeout(start_time)) {
            result.timeout = true;
            free(sorted_items);
            return result;
        }
        
        if (current_weight + sorted_items[i].weight <= capacity) {
            current_weight += sorted_items[i].weight;
            current_value += sorted_items[i].value;
            result.selected[sorted_items[i].id - 1] = true;
        }
    }
    
    result.total_value = current_value;
    result.total_weight = current_weight;
    result.execution_time = (double)(get_current_time_ms() - start_time);
    
    free(sorted_items);
    
    return result;
}

// 回溯法辅助函数
void backtrack(Item items[], int n, double capacity, int level, 
               double current_weight, double current_value, 
               bool current_selection[], double *max_value, 
               bool best_selection[], long long start_time, bool *timeout) {
    if (*timeout || check_timeout(start_time)) {
        *timeout = true;
        return;
    }
    
    if (level == n) {
        if (current_value > *max_value && current_weight <= capacity) {
            *max_value = current_value;
            memcpy(best_selection, current_selection, n * sizeof(bool));
        }
        return;
    }
    
    // 剪枝：如果当前价值加上剩余物品的最大可能价值小于已知最大值，则剪枝
    double remaining_value = 0;
    for (int i = level; i < n; i++) {
        remaining_value += items[i].value;
    }
    if (current_value + remaining_value <= *max_value) {
        return;
    }
    
    // 不选当前物品
    backtrack(items, n, capacity, level + 1, current_weight, current_value, 
              current_selection, max_value, best_selection, start_time, timeout);
    
    // 选当前物品（如果不超过容量）
    if (current_weight + items[level].weight <= capacity) {
        current_selection[level] = true;
        backtrack(items, n, capacity, level + 1, current_weight + items[level].weight, 
                  current_value + items[level].value, current_selection, max_value, 
                  best_selection, start_time, timeout);
        current_selection[level] = false;
    }
}

// 回溯法求解0-1背包问题
Result backtrack_knapsack(Item items[], int n, double capacity) {
    Result result = {0};
    result.selected = (bool *)calloc(n, sizeof(bool));
    result.timeout = false;
    
    long long start_time = get_current_time_ms();
    
    double max_value = 0;
    bool *best_selection = (bool *)calloc(n, sizeof(bool));
    bool *current_selection = (bool *)calloc(n, sizeof(bool));
    bool timeout = false;
    
    // 按价值/重量比排序，提高剪枝效率
    Item *sorted_items = (Item *)malloc(n * sizeof(Item));
    memcpy(sorted_items, items, n * sizeof(Item));
    qsort(sorted_items, n, sizeof(Item), compare_ratio);
    
    backtrack(sorted_items, n, capacity, 0, 0, 0, current_selection, 
              &max_value, best_selection, start_time, &timeout);
    
    if (timeout) {
        result.timeout = true;
    } else {
        result.total_value = max_value;
        result.total_weight = 0;
        for (int i = 0; i < n; i++) {
            if (best_selection[i]) {
                result.total_weight += sorted_items[i].weight;
                result.selected[sorted_items[i].id - 1] = true;
            }
        }
    }
    
    free(sorted_items);
    free(best_selection);
    free(current_selection);
    
    result.execution_time = (double)(get_current_time_ms() - start_time);
    
    return result;
}

int main() {
    int n;
    double capacity;
    
    printf("0-1背包问题求解程序\n");
    printf("请输入物品数量(1-%d): ", MAX_ITEMS);
    scanf("%d", &n);
    
    if (n < 1 || n > MAX_ITEMS) {
        printf("物品数量超出范围\n");
        return 1;
    }
    
    printf("请输入背包容量: ");
    scanf("%lf", &capacity);
    
    // 生成物品数据
    Item *items = (Item *)malloc(n * sizeof(Item));
    generate_items(items, n);
    
    // 将物品数据写入Excel文件
    write_items_to_excel(items, n, "物品数据.csv");
    
    // 定义算法名称
    const char *methods[] = {"蛮力法", "动态规划法", "贪心法", "回溯法"};
    const int method_count = 4;
    
    // 存储结果
    Result *results = (Result *)malloc(method_count * sizeof(Result));
    
    // 使用不同算法求解
    printf("\n使用蛮力法求解...\n");
    results[0] = brute_force_knapsack(items, n, capacity);
    
    printf("\n使用动态规划法求解...\n");
    results[1] = dp_knapsack(items, n, capacity);
    
    printf("\n使用贪心法求解...\n");
    results[2] = greedy_knapsack(items, n, capacity);
    
    printf("\n使用回溯法求解...\n");
    results[3] = backtrack_knapsack(items, n, capacity);
    
    // 将结果写入Excel文件
    write_results_to_excel(results, methods, method_count, "算法结果.csv");
    
    // 打印结果摘要
    printf("\n结果摘要:\n");
    printf("------------------------------------------------------------\n");
    printf("%-12s%-12s%-12s%-15s%s\n", "算法", "总价值", "总重量", "执行时间(ms)", "状态");
    printf("------------------------------------------------------------\n");
    
    for (int i = 0; i < method_count; i++) {
        if (results[i].timeout) {
            printf("%-12s%-12s%-12s%-15.2f%s\n", methods[i], "N/A", "N/A", 
                   results[i].execution_time, "超时");
        } else {
            printf("%-12s%-12.2f%-12.2f%-15.2f%s\n", methods[i], results[i].total_value, 
                   results[i].total_weight, results[i].execution_time, "完成");
        }
    }
    
    // 释放内存
    for (int i = 0; i < method_count; i++) {
        if (results[i].selected != NULL) {
            free(results[i].selected);
        }
    }
    free(results);
    free(items);
    
    return 0;
}
