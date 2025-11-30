
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <functional>
#include "../include/buffered_channel.h"

using Matrix = std::vector<std::vector<double>>;

Matrix random_matrix(size_t N) {
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    Matrix M(N, std::vector<double>(N));
    for (size_t i = 0; i < N; ++i)
        for (size_t j = 0; j < N; ++j)
            M[i][j] = dist(rng);
    return M;
}

struct Task {
    size_t row, col, block;
};

void worker(const Matrix& A, const Matrix& B, Matrix& C,
            buffered_channel<Task>& ch, size_t N) {
    while (auto opt = ch.pop()) {
        Task t = *opt;
        size_t row_end = std::min(t.row + t.block, N);
        size_t col_end = std::min(t.col + t.block, N);
        for (size_t i = t.row; i < row_end; ++i)
            for (size_t j = t.col; j < col_end; ++j) {
                double sum = 0.0;
                for (size_t k = 0; k < N; ++k)
                    sum += A[i][k] * B[k][j];
                C[i][j] = sum;
            }
    }
}

Matrix multiply_channel(const Matrix& A, const Matrix& B, size_t block) {
    size_t N = A.size();
    Matrix C(N, std::vector<double>(N, 0.0));
    buffered_channel<Task> ch(64);

    size_t hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 4;
    std::vector<std::thread> workers;
    for (size_t i = 0; i < hw; ++i)
        workers.emplace_back(worker, std::cref(A), std::cref(B),
                             std::ref(C), std::ref(ch), N);

    for (size_t row = 0; row < N; row += block)
        for (size_t col = 0; col < N; col += block)
            ch.push(Task{row, col, block});
    ch.close();

    for (auto& t : workers) t.join();
    return C;
}

Matrix multiply_naive(const Matrix& A, const Matrix& B) {
    size_t N = A.size();
    Matrix C(N, std::vector<double>(N, 0.0));
    for (size_t i = 0; i < N; ++i)
        for (size_t j = 0; j < N; ++j)
            for (size_t k = 0; k < N; ++k)
                C[i][j] += A[i][k] * B[k][j];
    return C;
}

int main(int argc, char** argv) {
    size_t N = (argc > 1) ? std::stoul(argv[1]) : 10;
    if (N < 5) N = 5;
    Matrix A = random_matrix(N);
    Matrix B = random_matrix(N);

    auto start = std::chrono::high_resolution_clock::now();
    auto C_naive = multiply_naive(A, B);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Naive multiplication time: "
              << std::chrono::duration<double>(end - start).count() << " s\n";

    std::cout << "block_size,time\n";
    for (size_t block = 1; block <= N; ++block) {
        start = std::chrono::high_resolution_clock::now();
        auto C = multiply_channel(A, B, block);
        end = std::chrono::high_resolution_clock::now();
        double t = std::chrono::duration<double>(end - start).count();
        std::cout << block << "," << t << "\n";
        if (N <= 10 && C != C_naive) {
            std::cerr << "Mismatch at block " << block << "\n";
            return 1;
        }
    }
    return 0;
}
