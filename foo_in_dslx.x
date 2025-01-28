//===----------------------------------------------------------------------===//
// br: BRANCH
//===----------------------------------------------------------------------===//

// br %13 {handshake.bb = 1 : ui32, handshake.name = "br2"} : <u1>
// br %14 {handshake.bb = 1 : ui32, handshake.name = "br1"} : <i32>
// br %result {handshake.bb = 1 : ui32, handshake.name = "br3"} : <>

// These are all NOPs -> fold

//===----------------------------------------------------------------------===//
// cmpi: COMPARISON
//===----------------------------------------------------------------------===//

// cmpi eq, %1#0, %5#1 {handshake.bb = 0 : ui32, handshake.name = "cmpu1"} : <s8>
// cmpi ne, %1#1, %5#0 {handshake.bb = 0 : ui32, handshake.name = "cmpi0"} : <s8>

proc cmpi_eq_s8 {
    input_lhs: chan<s8> in;
    input_rhs: chan<s8> in;
    output: chan<u1> out;

    init { () }
    config(input_lhs: chan<s8> in, input_rhs: chan<s8> in, output: chan<u1> out) {
        (input_lhs, input_rhs, output)
    }

    next(state: ()) {
        let (tok1, val_lhs) = recv(join(), input_lhs);
        let (tok2, val_rhs) = recv(join(), input_rhs);
        send(join(tok1, tok2), output, val_lhs == val_rhs);
    }
}

proc cmpi_neq_s8 {
    input_lhs: chan<s8> in;
    input_rhs: chan<s8> in;
    output: chan<u1> out;

    init { () }
    config(input_lhs: chan<s8> in, input_rhs: chan<s8> in, output: chan<u1> out) {
        (input_lhs, input_rhs, output)
    }

    next(state: ()) {
        let (tok1, val_lhs) = recv(join(), input_lhs);
        let (tok2, val_rhs) = recv(join(), input_rhs);
        send(join(tok1, tok2), output, val_lhs != val_rhs);
    }
}

//===----------------------------------------------------------------------===//
// cond_br: CONDITIONAL BRANCH
//===----------------------------------------------------------------------===//

proc cond_br_u1 {
    cond: chan<u1> in;
    input: chan<u1> in;
    out_true: chan<u1> out;
    out_false: chan<u1> out;

    init { () }
    config(cond: chan<u1> in, input: chan<u1> in, out_true: chan<u1> out, out_false: chan<u1> out) {
        (cond, input, out_true, out_false)
    }

    next(state: ()) {
        let (tok1, val) = recv(join(), input);
        let (tok2, cond_val) = recv(join(), cond);

        if (cond_val == u1:1) {
            send(join(tok1, tok2), out_true, val);
        } else {
            send(join(tok1, tok2), out_false, val);
        }
    }
}

proc cond_br_s32 {
    cond: chan<u1> in;
    input: chan<s32> in;
    out_true: chan<s32> out;
    out_false: chan<s32> out;

    init { () }
    config(cond: chan<u1> in, input: chan<s32> in, out_true: chan<s32> out, out_false: chan<s32> out) {
        (cond, input, out_true, out_false)
    }

    next(state: ()) {
        let (tok1, val) = recv(join(), input);
        let (tok2, cond_val) = recv(join(), cond);

        if (cond_val == u1:1) {
            send(join(tok1, tok2), out_true, val);
        } else {
            send(join(tok1, tok2), out_false, val);
        }
    }
}

proc cond_br_tok {
    cond: chan<u1> in;
    input: chan<()> in;
    out_true: chan<()> out;
    out_false: chan<()> out;

    init { () }
    config(cond: chan<u1> in, input: chan<()> in, out_true: chan<()> out, out_false: chan<()> out) {
        (cond, input, out_true, out_false)
    }

    next(state: ()) {
        let (tok1, val) = recv(join(), input);
        let (tok2, cond_val) = recv(join(), cond);

        if (cond_val == u1:1) {
            send(join(tok1, tok2), out_true, val);
        } else {
            send(join(tok1, tok2), out_false, val);
        }
    }
}

//===----------------------------------------------------------------------===//
// constant
//===----------------------------------------------------------------------===//

// constant %0#0 {handshake.bb = 0 : ui32, handshake.name = "constant3", value = false} : <>, <u1>
// constant %21 {handshake.bb = 2 : ui32, handshake.name = "constant4", value = false} : <>, <u1>
// constant %2 {handshake.bb = 0 : ui32, handshake.name = "constant1", value = false} : <>, <u1>

proc constant_u1_false {
    input: chan<()> in;
    output: chan<u1> out;

    init { () }
    config(input: chan<()> in, output: chan<u1> out) {
        (input, output)
    }

    next(state: ()) {
        let (tok1, _) = recv(join(), input);
        send(join(tok1), output, u1:0);
    }
}



//===----------------------------------------------------------------------===//
// control merge
//===----------------------------------------------------------------------===//

// THIS gets folded away since the index is going to a sink
// control_merge %trueResult_4  {handshake.bb = 1 : ui32, handshake.name = "control_merge0"} : <>, <u1>

// control_merge %falseResult_5, %17  {handshake.bb = 2 : ui32, handshake.name = "control_merge1"} : <>, <u1>
proc control_merge_tok_2x {
    input0: chan<()> in;
    input1: chan<()> in;
    output: chan<()> out;
    output_index: chan<u1> out;

    init { () }
    config(input0: chan<()> in, input1: chan<()> in, output: chan<()> out, output_index: chan<u1> out) {
        (input0, input1, output, output_index)
    }

    next(state: ()) {
        let (tok0, val0, got0) = recv_non_blocking(join(), input0, ());
        let (tok1, val1, got1) = recv_if_non_blocking(join(), input1, !got0, ());

        let do_send = got0 || got1;
        let send_val = if (got0) {
            val0
        } else {
            val1
        };


        let index_val = if (got0) {
            u1:0
        } else {
            u1:1
        };

        send_if(join(tok0,tok1), output, do_send, send_val);
        send_if(join(tok0,tok1), output_index, do_send, index_val);
    }
}


//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

proc extsi_u1_to_s32 {
    input: chan<u1> in;
    output: chan<s32> out;

    init { () }
    config(input: chan<u1> in, output: chan<s32> out) {
        (input, output)
    }

    next(state: ()) {
        let (tok1, val) = recv(join(), input);
        send(join(tok1), output, (val as s1) as s32);
    }
}

proc extsi_u1_to_s8 {
    input: chan<u1> in;
    output: chan<s8> out;

    init { () }
    config(input: chan<u1> in, output: chan<s8> out) {
        (input, output)
    }

    next(state: ()) {
        let (tok1, val) = recv(join(), input);
        send(join(tok1), output, (val as s1) as s8);
    }
}

// extsi %22 {handshake.bb = 2 : ui32, handshake.name = "extsi2"} : <u1> to <i32>
// extsi %falseResult_7 {handshake.bb = 0 : ui32, handshake.name = "extsi3"} : <u1> to <i32>
// extsi %3 {handshake.bb = 0 : ui32, handshake.name = "extsi0"} : <u1> to <s8>

//===----------------------------------------------------------------------===//
// fork: duplicate values
//===----------------------------------------------------------------------===//

// fork [2] %4 {handshake.bb = 0 : ui32, handshake.name = "fork2"} : <s8>
// fork [2] %arg2 {handshake.bb = 0 : ui32, handshake.name = "fork1"} : <s8>
proc fork_s8_x2 {
    input: chan<s8> in;
    out0: chan<s8> out;
    out1: chan<s8> out;

    init { () }
    config(input: chan<s8> in, out0: chan<s8> out, out1: chan<s8> out) {
        (input, out0, out1)
    }

    next(state: ()) {
        let (tok, val) = recv(join(), input);
        send(tok, out0, val);
        send(tok, out1, val);
    }
}

// fork [2] %index_9 {handshake.bb = 2 : ui32, handshake.name = "fork4"} : <u1>
proc fork_u1_x2 {
    input: chan<u1> in;
    out0: chan<u1> out;
    out1: chan<u1> out;

    init { () }
    config(input: chan<u1> in, out0: chan<u1> out, out1: chan<u1> out) {
        (input, out0, out1)
    }

    next(state: ()) {
        let (tok, val) = recv(join(), input);
        send(tok, out0, val);
        send(tok, out1, val);
    }
}

// fork [5] %7 {handshake.bb = 0 : ui32, handshake.name = "fork3"} : <u1>
proc fork_u1_x5 {
    input: chan<u1> in;
    out0: chan<u1> out;
    out1: chan<u1> out;
    out2: chan<u1> out;
    out3: chan<u1> out;
    out4: chan<u1> out;

    init { () }
    config(input: chan<u1> in, out0: chan<u1> out, out1: chan<u1> out, out2: chan<u1> out, out3: chan<u1> out, out4: chan<u1> out) {
        (input, out0, out1, out2, out3, out4)
    }

    next(state: ()) {
        let (tok, val) = recv(join(), input);
        send(tok, out0, val);
        send(tok, out1, val);
        send(tok, out2, val);
        send(tok, out3, val);
        send(tok, out4, val);
    }
}

// fork [3] %arg3 {handshake.bb = 0 : ui32, handshake.name = "fork0"} : <>
proc fork_tok_x3 {
    input: chan<()> in;
    out0: chan<()> out;
    out1: chan<()> out;
    out2: chan<()> out;

    init { () }
    config(input: chan<()> in, out0: chan<()> out, out1: chan<()> out, out2: chan<()> out) {
        (input, out0, out1, out2)
    }

    next(state: ()) {
        let (tok, val) = recv(join(), input);
        send(tok, out0, val);
        send(tok, out1, val);
        send(tok, out2, val);
    }
}

//===----------------------------------------------------------------------===//
// merge: propagate any input
//===----------------------------------------------------------------------===//

// All get folded away : only 1 input.
// merge %trueResult_0 {handshake.bb = 1 : ui32, handshake.name = "merge1"} : <i32>
// merge %trueResult {handshake.bb = 1 : ui32, handshake.name = "merge0"} : <i32>
// merge %trueResult_2 {handshake.bb = 1 : ui32, handshake.name = "merge2"} : <u1>

//===----------------------------------------------------------------------===//
// muli: Integer multiplication
//===----------------------------------------------------------------------===//

// muli %11, %12 {handshake.bb = 1 : ui32, handshake.name = "muli0"} : <i32>
proc muli_s32 {
    input_lhs: chan<s32> in;
    input_rhs: chan<s32> in;
    output: chan<s32> out;

    init { () }
    config(input_lhs: chan<s32> in, input_rhs: chan<s32> in, output: chan<s32> out) {
        (input_lhs, input_rhs, output)
    }

    next(state: ()) {
        let (tok1, val_lhs) = recv(join(), input_lhs);
        let (tok2, val_rhs) = recv(join(), input_rhs);
        send(join(tok1, tok2), output, val_lhs * val_rhs);
    }
}

//===----------------------------------------------------------------------===//
// MUX
//===----------------------------------------------------------------------===//

// mux %20#0 [%10, %15] {handshake.bb = 2 : ui32, handshake.name = "mux0"} : <u1>, <i32>

proc mux_s32_x2 {
    ctrl: chan<u1> in;
    input0: chan<s32> in;
    input1: chan<s32> in;

    output: chan<s32> out;

    init { () }
    config(ctrl: chan<u1> in, input0: chan<s32> in, input1: chan<s32> in, output: chan<s32> out) {
        (ctrl, input0, input1, output)
    }

    next(state: ()) {
        let (tok_ctrl, val_ctrl) = recv(join(), ctrl);

        let (tok0, val0) = recv_if(tok_ctrl, input0, val_ctrl == u1:0, s32:0);
        let (tok1, val1) = recv_if(tok_ctrl, input1, val_ctrl == u1:1, s32:0);

        let val_send = if (val_ctrl) {
            val1
        } else {
            val0
        };

        send(join(tok0, tok1, tok_ctrl), output, val_send);
    }
}

// mux %20#1 [%falseResult_3, %16] {handshake.bb = 2 : ui32, handshake.name = "mux1"} : <u1>, <u1>

proc mux_u1_x2 {
    ctrl: chan<u1> in;
    input0: chan<u1> in;
    input1: chan<u1> in;

    output: chan<u1> out;

    init { () }
    config(ctrl: chan<u1> in, input0: chan<u1> in, input1: chan<u1> in, output: chan<u1> out) {
        (ctrl, input0, input1, output)
    }

    next(state: ()) {
        let (tok_ctrl, val_ctrl) = recv(join(), ctrl);

        let (tok0, val0) = recv_if(tok_ctrl, input0, val_ctrl == u1:0, u1:0);
        let (tok1, val1) = recv_if(tok_ctrl, input1, val_ctrl == u1:1, u1:0);

        let val_send = if (val_ctrl) {
            val1
        } else {
            val0
        };

        send(join(tok0, tok1, tok_ctrl), output, val_send);
    }
}

//===----------------------------------------------------------------------===//
// Select
//===----------------------------------------------------------------------===//

// select %19[%23, %18] {handshake.bb = 2 : ui32, handshake.name = "select0"} : <u1>, <i32>

proc select_s32_x2 {
    ctrl: chan<u1> in;
    input0: chan<s32> in;
    input1: chan<s32> in;

    output: chan<s32> out;

    init { () }
    config(ctrl: chan<u1> in, input0: chan<s32> in, input1: chan<s32> in, output: chan<s32> out) {
        (ctrl, input0, input1, output)
    }

    next(state: ()) {
        let (tok0, val0) = recv(join(), input0);
        let (tok1, val1) = recv(join(), input1);
        let (tok_ctrl, val_ctrl) = recv(join(), ctrl);

        let val_send = if (val_ctrl) {
            val1
        } else {
            val0
        };

        send(join(tok0, tok1, tok_ctrl), output, val_send);
    }
}


//===----------------------------------------------------------------------===//
// sink
//===----------------------------------------------------------------------===//

proc sink_s32 {
    input: chan<s32> in;

    init { () }
    config(input: chan<s32> in) {
        (input,)
    }

    next(state: ()) {
        recv(join(), input);
    }
}

proc sink_u1 {
    input: chan<u1> in;

    init { () }
    config(input: chan<u1> in) {
        (input,)
    }

    next(state: ()) {
        recv(join(), input);
    }
}


proc sink_tok {
    input: chan<()> in;

    init { () }
    config(input: chan<()> in) {
        (input,)
    }

    next(state: ()) {
        recv(join(), input);
    }
}


// sink %falseResult_1 {handshake.name = "sink1"} : <i32>
// sink %falseResult {handshake.name = "sink0"} : <i32>
// sink %index {handshake.name = "sink3"} : <u1>
// sink %result_8 {handshake.name = "sink4"} : <>
// sink %trueResult_6 {handshake.name = "sink2"} : <u1>

//===----------------------------------------------------------------------===//
// source
//===----------------------------------------------------------------------===//

proc source_tok {
    output: chan<()> out;

    init { () }
    config(output: chan<()> out) {
        (output,)
    }

    next(state: ()) {
        send(join(), output, ());
    }
}

// source {handshake.bb = 0 : ui32, handshake.name = "source0"} : <>
// source {handshake.bb = 2 : ui32, handshake.name = "source1"} : <>

//===----------------------------------------------------------------------===//
// foo: Interop
//===----------------------------------------------------------------------===//

proc foo_interop {

    init { () }
    config(
        INPUT_arg0: chan<s32> in,
        INPUT_arg1: chan<s32> in,
        INPUT_arg2: chan<s8> in,
        INPUT_arg3: chan<()> in,

        OUTPUT_out0: chan<s32> out,
        OUTPUT_out1: chan<()> out
    ) {

        //    %0:3 = fork [3] %arg3 {handshake.bb = 0 : ui32, handshake.name = "fork0"} : <>
        let (ssa0_0_s, ssa0_0_r) = chan<(), u32:0>("ssa0_0");
        // NOTE: S24 == OUTPUT0
        let (ssa0_2_s, ssa0_2_r) = chan<(), u32:0>("ssa0_2");
        spawn fork_tok_x3(INPUT_arg3, ssa0_0_s, OUTPUT_out1, ssa0_2_s);

        //    %1:2 = fork [2] %arg2 {handshake.bb = 0 : ui32, handshake.name = "fork1"} : <s8, u32:0>
        let (ssa1_0_s, ssa1_0_r) = chan<s8, u32:0>("ssa1_0");
        let (ssa1_1_s, ssa1_1_r) = chan<s8, u32:0>("ssa1_1");
        spawn fork_s8_x2(INPUT_arg2, ssa1_0_s, ssa1_1_s);

        //    %2 = source {handshake.bb = 0 : ui32, handshake.name = "source0"} : <, u32:0>
        let (ssa2_s, ssa2_r) = chan<(), u32:0>("ssa2");
        spawn source_tok(ssa2_s);

        //    %3 = constant %2 {handshake.bb = 0 : ui32, handshake.name = "constant1", value = false} : <, u32:0>, <u1>
        let (ssa3_s, ssa3_r) = chan<u1, u32:0>("ssa3");
        spawn constant_u1_false(ssa2_r, ssa3_s);

        //    %4 = extsi %3 {handshake.bb = 0 : ui32, handshake.name = "extsi0"} : <u1, u32:0> to <s8>
        let (ssa4_s, ssa4_r) = chan<s8, u32:0>("ssa4");
        spawn extsi_u1_to_s8(ssa3_r, ssa4_s);

        //    %5:2 = fork [2] %4 {handshake.bb = 0 : ui32, handshake.name = "fork2"} : <s8, u32:0>
        let (ssa5_0_s, ssa5_0_r) = chan<s8, u32:0>("ssa5_0_0");
        let (ssa5_1_s, ssa5_1_r) = chan<s8, u32:0>("ssa5_1_0");
        spawn fork_s8_x2(ssa4_r, ssa5_0_s, ssa5_1_s);

        //    %6 = constant %0#0 {handshake.bb = 0 : ui32, handshake.name = "constant3", value = false} : <, u32:0>, <u1>
        let (ssa6_s, ssa6_r) = chan<u1, u32:0>("ssa6");
        spawn constant_u1_false(ssa0_0_r, ssa6_s);

        //    %7 = cmpi ne, %1#1, %5#0 {handshake.bb = 0 : ui32, handshake.name = "cmpi0"} : <s8, u32:0>
        let (ssa7_s, ssa7_r) = chan<u1, u32:0>("ssa7");
        spawn cmpi_neq_s8(ssa1_1_r, ssa5_0_r, ssa7_s);

        //    %8:5 = fork [5] %7 {handshake.bb = 0 : ui32, handshake.name = "fork3"} : <u1, u32:0>
        let (ssa8_0_s, ssa8_0_r) = chan<u1, u32:0>("ssa8_0");
        let (ssa8_1_s, ssa8_1_r) = chan<u1, u32:0>("ssa8_1");
        let (ssa8_2_s, ssa8_2_r) = chan<u1, u32:0>("ssa8_2");
        let (ssa8_3_s, ssa8_3_r) = chan<u1, u32:0>("ssa8_3");
        let (ssa8_4_s, ssa8_4_r) = chan<u1, u32:0>("ssa8_4");
        spawn fork_u1_x5(ssa7_r, ssa8_0_s, ssa8_1_s, ssa8_2_s, ssa8_3_s, ssa8_4_s);

        //    %9 = cmpi eq, %1#0, %5#1 {handshake.bb = 0 : ui32, handshake.name = "cmpu1"} : <s8, u32:0>
        let (ssa9_s, ssa9_r) = chan<u1, u32:0>("ssa9");
        spawn cmpi_neq_s8(ssa1_0_r, ssa5_1_r, ssa9_s);

        //    %trueResult, %falseResult = cond_br %8#4, %arg0 {handshake.bb = 0 : ui32, handshake.name = "cond_br1"} : <u1, u32:0>, <i32>
        let (trueResult_s, trueResult_r) = chan<s32, u32:0>("trueResult");
        let (falseResult_s, falseResult_r) = chan<s32, u32:0>("falseResult");
        spawn cond_br_s32(ssa8_4_r, INPUT_arg0, trueResult_s, falseResult_s);

        //    sink %falseResult {handshake.name = "sink0"} : <i32, u32:0>
        spawn sink_s32(falseResult_r);

        //    %trueResult_0, %falseResult_1 = cond_br %8#3, %arg1 {handshake.bb = 0 : ui32, handshake.name = "cond_br2"} : <u1, u32:0>, <i32>
        let (trueResult_0_s, trueResult_0_r) = chan<s32, u32:0>("trueResult_0");
        let (falseResult_1_s, falseResult_1_r) = chan<s32, u32:0>("falseResult_1");
        spawn cond_br_s32(ssa8_3_r, INPUT_arg1, trueResult_0_s, falseResult_1_s);

        //    sink %falseResult_1 {handshake.name = "sink1"} : <i32, u32:0>
        spawn sink_s32(falseResult_1_r);

        //    %trueResult_2, %falseResult_3 = cond_br %8#2, %9 {handshake.bb = 0 : ui32, handshake.name = "cond_br3"} : <u1, u32:0>, <u1>
        let (trueResult_2_s, trueResult_2_r) = chan<u1, u32:0>("trueResult_2");
        let (falseResult_3_s, falseResult_3_r) = chan<u1, u32:0>("falseResult_3");
        spawn cond_br_u1(ssa8_2_r, ssa9_r, trueResult_2_s, falseResult_3_s);

        //    %trueResult_4, %falseResult_5 = cond_br %8#1, %0#2 {handshake.bb = 0 : ui32, handshake.name = "cond_br4"} : <u1, u32:0>, <>
        let (trueResult_4_s, trueResult_4_r) = chan<(), u32:0>("trueResult_4");
        let (falseResult_5_s, falseResult_5_r) = chan<(), u32:0>("falseResult_5");
        spawn cond_br_tok(ssa8_1_r, ssa0_2_r, trueResult_4_s, falseResult_5_s);

        //    %trueResult_6, %falseResult_7 = cond_br %8#0, %6 {handshake.bb = 0 : ui32, handshake.name = "cond_br5"} : <, u32:0>, <u1>
        let (trueResult_6_s, trueResult_6_r) = chan<u1, u32:0>("trueResult_6");
        let (falseResult_7_s, falseResult_7_r) = chan<u1, u32:0>("falseResult_7");
        spawn cond_br_u1(ssa8_0_r, ssa6_r, trueResult_6_s, falseResult_7_s);

        //    sink %trueResult_6 {handshake.name = "sink2"} : <u1, u32:0>
        spawn sink_u1(trueResult_6_r);

        //    %10 = extsi %falseResult_7 {handshake.bb = 0 : ui32, handshake.name = "extsi3"} : <u1, u32:0> to <i32>
        let (ssa10_s, ssa10_r) = chan<s32, u32:0>("ssa10");
        spawn extsi_u1_to_s32(falseResult_7_r, ssa10_s);

        //    %11 = merge %trueResult {handshake.bb = 1 : ui32, handshake.name = "merge0"} : <i32, u32:0>
        let ssa11_r = trueResult_r; // FOLDED AWAY

        //    %12 = merge %trueResult_0 {handshake.bb = 1 : ui32, handshake.name = "merge1"} : <i32, u32:0>
        let ssa12_r = trueResult_0_r; // FOLDED AWAY

        //    %13 = merge %trueResult_2 {handshake.bb = 1 : ui32, handshake.name = "merge2"} : <u1, u32:0>
        let ssa13_r = trueResult_2_r; // FOLDED AWAY

        //    %result, %index = control_merge %trueResult_4  {handshake.bb = 1 : ui32, handshake.name = "control_merge0"} : <, u32:0>, <u1>
        let result_r = trueResult_4_r; // FOLDED AWWAY
        // INDEX GETS SUNK

        //    sink %index {handshake.name = "sink3"} : <u1, u32:0>
        // FOLDED AWAY - NO RESULT

        //    %14 = muli %11, %12 {handshake.bb = 1 : ui32, handshake.name = "muli0"} : <i32, u32:0>
        let (ssa14_s, ssa14_r) = chan<s32, u32:0>("ssa14");
        spawn muli_s32(ssa11_r, ssa12_r, ssa14_s);

        //    %15 = br %14 {handshake.bb = 1 : ui32, handshake.name = "br1"} : <i32, u32:0>
        let ssa15_r = ssa14_r; // FOLDED AWAY

        //    %16 = br %13 {handshake.bb = 1 : ui32, handshake.name = "br2"} : <u1, u32:0>
        let ssa16_r =  ssa13_r; // FOLDED AWAY

        //    %17 = br %result {handshake.bb = 1 : ui32, handshake.name = "br3"} : <, u32:0>
        let ssa17_r = result_r; // FOLDED AWAY;

        // FORWARD DECLARE: USED EARLIER
        let (ssa20_0_s, ssa20_0_r) = chan<u1, u32:0>("ssa20_0");
        let (ssa20_1_s, ssa20_1_r) = chan<u1, u32:0>("ssa20_1");

        //    %18 = mux %20#0 [%10, %15] {handshake.bb = 2 : ui32, handshake.name = "mux0"} : <u1, u32:0>, <i32>
        let (ssa18_s, ssa18_r) = chan<s32, u32:0>("ssa18");
        spawn mux_s32_x2(ssa20_0_r, ssa10_r, ssa15_r, ssa18_s);

        //    %19 = mux %20#1 [%falseResult_3, %16] {handshake.bb = 2 : ui32, handshake.name = "mux1"} : <u1, u32:0>, <u1>
        let (ssa19_s, ssa19_r) = chan<u1, u32:0>("ssa19");
        spawn mux_u1_x2(ssa20_1_r, falseResult_3_r, ssa16_r, ssa19_s);

        //    %result_8, %index_9 = control_merge %falseResult_5, %17  {handshake.bb = 2 : ui32, handshake.name = "control_merge1"} : <, u32:0>, <>
        let (result_8_s, result_8_r) = chan<(), u32:0>("result_8");
        let (index_9_s, index_9_r) = chan<u1, u32:0>("index_9");
        spawn control_merge_tok_2x(falseResult_5_r, ssa17_r, result_8_s, index_9_s);

        //    %20:2 = fork [2] %index_9 {handshake.bb = 2 : ui32, handshake.name = "fork4"} : <u1, u32:0>
        spawn fork_u1_x2(index_9_r, ssa20_0_s, ssa20_1_s);

        //    sink %result_8 {handshake.name = "sink4"} : <, u32:0>
        spawn sink_tok(result_8_r);

        //    %21 = source {handshake.bb = 2 : ui32, handshake.name = "source1"} : <, u32:0>
        let (ssa21_s, ssa21_r) = chan<(), u32:0>("ssa21");
        spawn source_tok(ssa21_s);

        //    %22 = constant %21 {handshake.bb = 2 : ui32, handshake.name = "constant4", value = false} : <, u32:0>, <u1>
        let (ssa22_s, ssa22_r) = chan<u1, u32:0>("ssa22");
        spawn constant_u1_false(ssa21_r, ssa22_s);

        //    %23 = extsi %22 {handshake.bb = 2 : ui32, handshake.name = "extsi2"} : <u1, u32:0> to <i32>
        let (ssa23_s, ssa23_r) = chan<s32, u32:0>("ssa23");
        spawn extsi_u1_to_s32(ssa22_r, ssa23_s);

        //    %24 = select %19[%23, %18] {handshake.bb = 2 : ui32, handshae.name = "select0"} : <u1, u32:0>, <i32>
        // NOTE: S24 == OUTPUT0
        spawn select_s32_x2(ssa19_r, ssa23_r, ssa18_r, OUTPUT_out0);

        //    end {handshake.bb = 2 : ui32, handshake.name = "end0"} %24, %0#1 : <i32, u32:1>, <>


    }

    next(state: ()) {
    }
}

//===----------------------------------------------------------------------===//
// foo: DSLX
//===----------------------------------------------------------------------===//

proc foo_dslx {
    INPUT_arg0: chan<s32> in;
    INPUT_arg1: chan<s32> in;
    INPUT_arg2: chan<s8> in;
    INPUT_arg3: chan<()> in;

    OUTPUT_out0: chan<s32> out;
    OUTPUT_out1: chan<()> out;

    init { () }
    config(
        INPUT_arg0: chan<s32> in,
        INPUT_arg1: chan<s32> in,
        INPUT_arg2: chan<s8> in,
        INPUT_arg3: chan<()> in,

        OUTPUT_out0: chan<s32> out,
        OUTPUT_out1: chan<()> out
    ) {
        (INPUT_arg0, INPUT_arg1, INPUT_arg2, INPUT_arg3, OUTPUT_out0, OUTPUT_out1)
    }

    next(state: ()) {
        let (tok0, a) = recv(join(), INPUT_arg0);
        let (tok1, b) = recv(join(), INPUT_arg1);
        let (tok2, ctrl1) = recv(join(), INPUT_arg2);
        let (tok3, _) = recv(join(), INPUT_arg3);

        let tok = join(tok0, tok1, tok2, tok3);

        let result = if (ctrl1 != s8:0) {
            a * b
        } else {
            s32:0
        };

        send(tok, OUTPUT_out0, result);
        send(tok, OUTPUT_out1, ());
    }
}

//===----------------------------------------------------------------------===//
// TEST
//===----------------------------------------------------------------------===//

#[test_proc]
proc test{
    input_a_s: chan<s32> out;
    input_b_s: chan<s32> out;
    input_ctrl_s: chan<s8> out;
    input_start_s: chan<()> out;

    output_r: chan<s32> in;
    output_end_r: chan<()> in;

    terminator: chan<bool> out;

    init { () }

    config(terminator: chan<bool> out) {
        // PROC IO
        let (input_a_s, input_a_r) = chan<s32>("input_a");
        let (input_b_s, input_b_r) = chan<s32>("input_b");
        let (input_ctrl_s, input_ctrl_r) = chan<s8>("input_ctrl");
        let (input_start_s, input_start_r) = chan<()>("input_start");

        let (output_s, output_r) = chan<s32>("output");
        let (output_end_s, output_end_r) = chan<()>("output_end");

        spawn foo_interop(input_a_r, input_b_r, input_ctrl_r, input_start_r, output_s, output_end_s);
        // spawn foo_dslx(input_a_r, input_b_r, input_ctrl_r, input_start_r, output_s, output_end_s);


        (input_a_s, input_b_s, input_ctrl_s, input_start_s, output_r,  output_end_r, terminator)
    }

    next(state: ()) {
        let tok = send(join(), input_a_s, s32:4);
        let tok = send(tok, input_b_s, s32:5);
        let tok = send(tok, input_ctrl_s, s8:0);
        let tok = send(tok, input_start_s, ());

        let (tok, result) = recv(tok, output_r);
        let (tok, _) = recv(tok, output_end_r);
        assert_eq(result, s32:0);
        trace_fmt!("4*5 ctrl==0 -> {}", result);

        let tok = send(join(), input_a_s, s32:4);
        let tok = send(tok, input_b_s, s32:5);
        let tok = send(tok, input_ctrl_s, s8:1);
        let tok = send(tok, input_start_s, ());

        let (tok, result) = recv(tok, output_r);
        let (tok, _) = recv(tok, output_end_r);
        assert_eq(result, s32:20);
        trace_fmt!("4*5 ctrl==1 -> {}", result);

        let tok = send(join(), input_a_s, s32:100);
        let tok = send(tok, input_b_s, s32:200);
        let tok = send(tok, input_ctrl_s, s8:0);
        let tok = send(tok, input_start_s, ());

        let (tok, result) = recv(tok, output_r);
        let (tok, _) = recv(tok, output_end_r);
        assert_eq(result, s32:0);
        trace_fmt!("100*200 ctrl==0 -> {}", result);

        let tok = send(join(), input_a_s, s32:100);
        let tok = send(tok, input_b_s, s32:200);
        let tok = send(tok, input_ctrl_s, s8:1);
        let tok = send(tok, input_start_s, ());

        let (tok, result) = recv(tok, output_r);
        let (tok, _) = recv(tok, output_end_r);
        assert_eq(result, (s32:100 * s32:200));
        trace_fmt!("100*200 ctrl==1 -> {}", result);

        let tok = send(tok, terminator, true);
    }
}

