#include <print>

#include <BinaryStream/BinaryStream.hpp>
int main(int /*argc*/, char* /*argv*/[]) {
    cubix::BinaryStream stream{};

    stream.writeString("Hello, world!");

    std::println("{}", stream.readString());
}