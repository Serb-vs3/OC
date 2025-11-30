#include <windows.h>
#include <vector>
#include <iostream>
#include <random>
#include <chrono>
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

struct ThreadData {
    const Matrix* A;
    const Matrix* B;
    Matrix* C;
    size_t row;
    size_t col;
    size_t block;
    size_t N;
};

DWORD WINAPI multiply_block(LPVOID param) {
    ThreadData* d = static_cast<ThreadData*>(param);
    size_t row_end = min(d->row + d->block, d->N);
    size_t col_end = min(d->col + d->block, d->N);
    for (size_t i = d->row; i < row_end; ++i) {
        for (size_t j = d->col; j < col_end; ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < d->N; ++k)
                sum += (*d->A)[i][k] * (*d->B)[k][j];
            (*d->C)[i][j] = sum;
        }
    }
    return 0;
}

Matrix multiply_windows(const Matrix& A, const Matrix& B, size_t block) {
    size_t N = A.size();
    Matrix C(N, std::vector<double>(N, 0.0));
    std::vector<HANDLE> handles;
    std::vector<ThreadData> td;

    for (size_t row = 0; row < N; row += block)
        for (size_t col = 0; col < N; col += block) {
            td.push_back(ThreadData{&A,&B,&C,row,col,block,N});
            HANDLE h = CreateThread(nullptr, 0, multiply_block,
                                    &td.back(), 0, nullptr);
            handles.push_back(h);
        }
    WaitForMultipleObjects(static_cast<DWORD>(handles.size()), handles.data(), TRUE, INFINITE);
    for (HANDLE h : handles) CloseHandle(h);
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

    auto t0 = std::chrono::high_resolution_clock::now();
    auto C_naive = multiply_naive(A,B);
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "Naive multiplication time: "
              << std::chrono::duration<double>(t1 - t0).count() << " s\n";

    std::cout << "block_size,threads,time\n";
    for (size_t block = 1; block <= N; ++block) {
        size_t thread_cnt = ((N + block -1)/block)*((N + block -1)/block);
        auto start = std::chrono::high_resolution_clock::now();
        auto C = multiply_windows(A,B,block);
        auto end = std::chrono::high_resolution_clock::now();
        double t = std::chrono::duration<double>(end - start).count();
        std::cout << block << "," << thread_cnt << "," << t << "\n";
        if (N <= 10 && C != C_naive) {
            std::cerr << "Mismatch at block " << block << "\n";
            return 1;
        }
    }
    return 0;
}
