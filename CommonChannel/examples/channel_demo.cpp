#include <iostream>
#include <thread>
#include "buffered_channel.h"

int main() {
    BufferedChannel<int> ch(2);

    std::thread producer([&] {
        for (int i = 0; i < 5; ++i) {
            ch.Send(i);
        }
        ch.Close();
    });

    std::thread consumer([&] {
        while (true) {
            auto [v, ok] = ch.Recv();
            if (!ok) break;
            std::cout << v << '\\n';
        }
    });

    producer.join();
    consumer.join();
}
