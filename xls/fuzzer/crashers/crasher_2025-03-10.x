// Copyright 2020 The XLS Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// BEGIN_CONFIG
// sample_options {
//   input_is_dslx: true
//   sample_type: SAMPLE_TYPE_PROC
//   ir_converter_args: "--top=main"
//   convert_to_ir: true
//   optimize_ir: true
//   use_jit: true
//   codegen: true
//   codegen_args: "--use_system_verilog"
//   codegen_args: "--output_block_ir_path=sample.block.ir"
//   codegen_args: "--generator=combinational"
//   codegen_args: "--reset_data_path=false"
//   simulate: false
//   use_system_verilog: true
//   calls_per_sample: 0
//   proc_ticks: 100
//   with_valid_holdoff: false
//   codegen_ng: false
// }

fn x22(x23: bool, x24: token, x25: bool, x26: bool) -> (bool, bool, bool) {
    {
        let x27: bool = x25 >> if x26 >= bool:false { bool:false } else { x26 };
        (x27, x27, x27)
    }
}

proc main {
    x0: chan<(u15, u48)> out;
    x1: chan<(u28, u32)> in;
    x2: chan<u38> out;
    x3: chan<u22> out;
    config(x0: chan<(u15, u48)> out, x1: chan<(u28, u32)> in, x2: chan<u38> out, x3: chan<u22> out) {
        (x0, x1, x2, x3)
    }
    init {
        ()
    }
    next(x4: ()) {
        {
            let x5: token = join();
            let x6: bool = x4 == x4;
            let x7: token = send_if(x5, x3, x6, u22:4194303);
            let x8: bool = bit_slice_update(x6, x6, x6);
            let x10: bool = {
                let x9: (bool, bool) = umulp(x8, x6);
                x9.0 + x9.1
            };
            let x11: xN[bool:0x0][1] = x10[:1];
            let x12: u4 = x11 ++ x6 ++ x8 ++ x11;
            let x13: bool = x4 == x4;
            let x14: bool = x10[0+:bool];
            let x15: token = join();
            let x16: bool = and_reduce(x6);
            let x17: bool = x10 * x12 as bool;
            let x18: bool = -x14;
            let x19: token = send_if(x7, x0, x16, (u15:8, u48:0));
            let x20: bool = x13 * x13;
            let x21: bool = or_reduce(x6);
            let x28: (bool, bool, bool) = x22(x14, x15, x16, x17);
            let (x29, x30, x31): (bool, bool, bool) = x22(x14, x15, x16, x17);
            let x32: bool = x20 as bool * x16;
            let x33: bool = x10 | x10;
            let x34: bool = x30 | x30;
            let x35: (token, (u28, u32)) = recv(x15, x1);
            let x36: token = x35.0;
            let x37: (u28, u32) = x35.1;
            let x38: u2 = one_hot(x17, bool:0x1);
            let x39: bool = x4 != x4;
            let x40: u10 = x12 ++ x13 ++ x30 ++ x39 ++ x38 ++ x11;
            let x41: token = send(x5, x2, u38:57051044351);
            x4
        }
    }
}



