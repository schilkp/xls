//===----------------------------------------------------------------------===//
// Proc with memory
//===----------------------------------------------------------------------===//
//
// template<typename T>
// using InputChannel = __xls_channel<T, __xls_channel_dir_In>;
// template<typename T>
// using OutputChannel = __xls_channel<T, __xls_channel_dir_Out>;
// template<typename T, int Size>
// using Memory = __xls_memory<T, Size>;
//
// class TestBlock {
// public:
//     InputChannel<int> in;
//     OutputChannel<int> out;
//     Memory<short, 32> store;
//
//     int addr = 0;
//
//     #pragma hls_top
//     void Run() {
//         const int next_addr = (addr + 1) & 0b11111;
//         store[addr] = in.read();
//         out.write(store[next_addr]);
//         addr = next_addr;
//     }
// };
