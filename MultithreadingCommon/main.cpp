#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <cassert>

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

void multiply_block(const Matrix& A, const Matrix& B, Matrix& C,
                    size_t row, size_t col, size_t k, size_t N, size_t block) {
    size_t row_end = std::min(row + block, N);
    size_t col_end = std::min(col + block, N);
    for (size_t i = row; i < row_end; ++i) {
        for (size_t j = col; j < col_end; ++j) {
            double sum = 0.0;
            for (size_t t = 0; t < N; ++t)
                sum += A[i][t] * B[t][j];
            C[i][j] = sum;
        }
    }
}

Matrix multiply_threaded(const Matrix& A, const Matrix& B, size_t block) {
    size_t N = A.size();
    Matrix C(N, std::vector<double>(N, 0.0));

    std::vector<std::thread> threads;
    for (size_t row = 0; row < N; row += block) {
        for (size_t col = 0; col < N; col += block) {
            threads.emplace_back(multiply_block, std::cref(A), std::cref(B), std::ref(C),
                                 row, col, block, N, block);
        }
    }
    for (auto& t : threads) t.join();
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
    auto t_naive = std::chrono::duration<double>(end - start).count();
    std::cout << "Naive multiplication time: " << t_naive << " s\n";

    std::cout << "block_size,threads,time\n";
    for (size_t block = 1; block <= N; ++block) {
        size_t thread_cnt = ((N + block - 1)/ block)*((N + block -1)/block);
        start = std::chrono::high_resolution_clock::now();
        auto C = multiply_threaded(A, B, block);
        end = std::chrono::high_resolution_clock::now();
        double t = std::chrono::duration<double>(end - start).count();
        std::cout << block << "," << thread_cnt << "," << t << "\n";
        if (N <= 10 && C != C_naive) {
            std::cerr << "Mismatch at block " << block << "\n";
            return 1;
        }
    }
    return 0;
}
