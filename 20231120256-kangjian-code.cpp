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
#define TIMEOUT_MS 900000 // 15���ӳ�ʱ

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

// ���������Ʒ����
void generate_items(Item items[], int n) {
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        items[i].id = i + 1;
        items[i].weight = (rand() % MAX_WEIGHT) + 1;
        items[i].value = MIN_VALUE + (rand() % (MAX_VALUE - MIN_VALUE + 1)) + (rand() % 100) / 100.0;
    }
}

// ����Ʒ����д��Excel�ļ�
void write_items_to_excel(Item items[], int n, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("�޷�����Excel�ļ�\n");
        return;
    }
    
    fprintf(fp, "��Ʒ���,��Ʒ����,��Ʒ��ֵ\n");
    for (int i = 0; i < n && i < 1000; i++) {
        fprintf(fp, "%d,%.2f,%.2f\n", items[i].id, items[i].weight, items[i].value);
    }
    
    fclose(fp);
    printf("��Ʒ�����ѱ��浽 %s\n", filename);
}

// �����д��Excel�ļ�
void write_results_to_excel(Result results[], const char *methods[], int method_count, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("�޷���������ļ�\n");
        return;
    }
    
    fprintf(fp, "�㷨,�ܼ�ֵ,������,ִ��ʱ��(ms),�Ƿ�ʱ\n");
    for (int i = 0; i < method_count; i++) {
        if (results[i].timeout) {
            fprintf(fp, "%s,�����������޷�����,�����������޷�����,%.2f,��\n", 
                   methods[i], results[i].execution_time);
        } else {
            fprintf(fp, "%s,%.2f,%.2f,%.2f,��\n", 
                   methods[i], results[i].total_value, results[i].total_weight, results[i].execution_time);
        }
    }
    
    fclose(fp);
    printf("��������ѱ��浽 %s\n", filename);
}

// ��ȡ��ǰʱ�䣨���룩
long long get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// ����Ƿ�ʱ
bool check_timeout(long long start_time) {
    long long current_time = get_current_time_ms();
    return (current_time - start_time) > TIMEOUT_MS;
}

// ���������0-1��������
Result brute_force_knapsack(Item items[], int n, double capacity) {
    Result result = {0};
    result.selected = (bool *)calloc(n, sizeof(bool));
    result.timeout = false;
    
    long long start_time = get_current_time_ms();
    
    double max_value = 0;
    bool *best_selection = (bool *)calloc(n, sizeof(bool));
    
    // �������п��ܵ��Ӽ�
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

// ��̬�滮�����0-1��������
Result dp_knapsack(Item items[], int n, double capacity) {
    Result result = {0};
    result.selected = (bool *)calloc(n, sizeof(bool));
    result.timeout = false;
    
    long long start_time = get_current_time_ms();
    
    // ������ת��Ϊ����������100������λС����
    int int_capacity = (int)(capacity * 100);
    int *weights = (int *)malloc(n * sizeof(int));
    int *values = (int *)malloc(n * sizeof(int));
    
    for (int i = 0; i < n; i++) {
        weights[i] = (int)(items[i].weight * 100);
        values[i] = (int)(items[i].value * 100);
    }
    
    // ����DP��
    int **dp = (int **)malloc((n + 1) * sizeof(int *));
    for (int i = 0; i <= n; i++) {
        dp[i] = (int *)malloc((int_capacity + 1) * sizeof(int));
    }
    
    // ��ʼ��DP��
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
            // �ͷ��ڴ�
            for (int i = 0; i <= n; i++) {
                free(dp[i]);
            }
            free(dp);
            free(weights);
            free(values);
            return result;
        }
    }
    
    // �����ҳ�ѡ�����Ʒ
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
    
    // �������������ܼ�ֵ
    result.total_value = dp[n][int_capacity] / 100.0;
    result.total_weight = 0;
    for (int i = 0; i < n; i++) {
        if (result.selected[i]) {
            result.total_weight += items[i].weight;
        }
    }
    
    // �ͷ��ڴ�
    for (int i = 0; i <= n; i++) {
        free(dp[i]);
    }
    free(dp);
    free(weights);
    free(values);
    
    result.execution_time = (double)(get_current_time_ms() - start_time);
    
    return result;
}

// �ȽϺ���������̰���㷨������
int compare_ratio(const void *a, const void *b) {
    Item *item_a = (Item *)a;
    Item *item_b = (Item *)b;
    double ratio_a = item_a->value / item_a->weight;
    double ratio_b = item_b->value / item_b->weight;
    
    if (ratio_a > ratio_b) return -1;
    if (ratio_a < ratio_b) return 1;
    return 0;
}

// ̰�ķ����0-1��������
Result greedy_knapsack(Item items[], int n, double capacity) {
    Result result = {0};
    result.selected = (bool *)calloc(n, sizeof(bool));
    result.timeout = false;
    
    long long start_time = get_current_time_ms();
    
    // ������Ʒ���飬�����޸�ԭʼ����
    Item *sorted_items = (Item *)malloc(n * sizeof(Item));
    memcpy(sorted_items, items, n * sizeof(Item));
    
    // ����ֵ/����������
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

// ���ݷ���������
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
    
    // ��֦�������ǰ��ֵ����ʣ����Ʒ�������ܼ�ֵС����֪���ֵ�����֦
    double remaining_value = 0;
    for (int i = level; i < n; i++) {
        remaining_value += items[i].value;
    }
    if (current_value + remaining_value <= *max_value) {
        return;
    }
    
    // ��ѡ��ǰ��Ʒ
    backtrack(items, n, capacity, level + 1, current_weight, current_value, 
              current_selection, max_value, best_selection, start_time, timeout);
    
    // ѡ��ǰ��Ʒ�����������������
    if (current_weight + items[level].weight <= capacity) {
        current_selection[level] = true;
        backtrack(items, n, capacity, level + 1, current_weight + items[level].weight, 
                  current_value + items[level].value, current_selection, max_value, 
                  best_selection, start_time, timeout);
        current_selection[level] = false;
    }
}

// ���ݷ����0-1��������
Result backtrack_knapsack(Item items[], int n, double capacity) {
    Result result = {0};
    result.selected = (bool *)calloc(n, sizeof(bool));
    result.timeout = false;
    
    long long start_time = get_current_time_ms();
    
    double max_value = 0;
    bool *best_selection = (bool *)calloc(n, sizeof(bool));
    bool *current_selection = (bool *)calloc(n, sizeof(bool));
    bool timeout = false;
    
    // ����ֵ/������������߼�֦Ч��
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
    
    printf("0-1��������������\n");
    printf("��������Ʒ����(1-%d): ", MAX_ITEMS);
    scanf("%d", &n);
    
    if (n < 1 || n > MAX_ITEMS) {
        printf("��Ʒ����������Χ\n");
        return 1;
    }
    
    printf("�����뱳������: ");
    scanf("%lf", &capacity);
    
    // ������Ʒ����
    Item *items = (Item *)malloc(n * sizeof(Item));
    generate_items(items, n);
    
    // ����Ʒ����д��Excel�ļ�
    write_items_to_excel(items, n, "��Ʒ����.csv");
    
    // �����㷨����
    const char *methods[] = {"������", "��̬�滮��", "̰�ķ�", "���ݷ�"};
    const int method_count = 4;
    
    // �洢���
    Result *results = (Result *)malloc(method_count * sizeof(Result));
    
    // ʹ�ò�ͬ�㷨���
    printf("\nʹ�����������...\n");
    results[0] = brute_force_knapsack(items, n, capacity);
    
    printf("\nʹ�ö�̬�滮�����...\n");
    results[1] = dp_knapsack(items, n, capacity);
    
    printf("\nʹ��̰�ķ����...\n");
    results[2] = greedy_knapsack(items, n, capacity);
    
    printf("\nʹ�û��ݷ����...\n");
    results[3] = backtrack_knapsack(items, n, capacity);
    
    // �����д��Excel�ļ�
    write_results_to_excel(results, methods, method_count, "�㷨���.csv");
    
    // ��ӡ���ժҪ
    printf("\n���ժҪ:\n");
    printf("------------------------------------------------------------\n");
    printf("%-12s%-12s%-12s%-15s%s\n", "�㷨", "�ܼ�ֵ", "������", "ִ��ʱ��(ms)", "״̬");
    printf("------------------------------------------------------------\n");
    
    for (int i = 0; i < method_count; i++) {
        if (results[i].timeout) {
            printf("%-12s%-12s%-12s%-15.2f%s\n", methods[i], "N/A", "N/A", 
                   results[i].execution_time, "��ʱ");
        } else {
            printf("%-12s%-12.2f%-12.2f%-15.2f%s\n", methods[i], results[i].total_value, 
                   results[i].total_weight, results[i].execution_time, "���");
        }
    }
    
    // �ͷ��ڴ�
    for (int i = 0; i < method_count; i++) {
        if (results[i].selected != NULL) {
            free(results[i].selected);
        }
    }
    free(results);
    free(items);
    
    return 0;
}
