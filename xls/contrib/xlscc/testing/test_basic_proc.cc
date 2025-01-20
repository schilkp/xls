//===----------------------------------------------------------------------===//
// Basic proc
//===----------------------------------------------------------------------===//

class TestBlock {
public:
    __xls_channel<int, __xls_channel_dir_In> in;
    __xls_channel<int, __xls_channel_dir_Out> out;

    #pragma hls_top
    void Run() {
        auto x = in.read();
        out.write(2*x);
    }
};


