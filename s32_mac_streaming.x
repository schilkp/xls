
proc s32_mac_streaming {

    // Channels
    input_a: chan<s32> in;
    input_b: chan<s32> in;
    output: chan<s32> out;

    // State
    init { s32:0 }

    // Setup
    config(input_a: chan<s32> in, input_b: chan<s32> in, output: chan<s32> out) {
        (input_a, input_b, output)
    }

    // Transition
    next(acc: s32) {

        let (tok0, a) = recv(join(), input_a);
        let (tok1, b) = recv(join(), input_b);

        trace_fmt!("[DUT]: Got ({},{})", a, b);

        let acc = acc + a * b;

        trace_fmt!("[DUT]: Sending {}", acc);
        send(join(tok0, tok1), output, acc);

        acc
    }
}


#[test_proc]
proc smoke_test {
    input_a_s: chan<s32> out;
    input_b_s: chan<s32> out;
    output_r: chan<s32> in;
    terminator: chan<bool> out;

    init { () }

    config(terminator: chan<bool> out) {
        let (input_a_s, input_a_r) = chan<s32>("input_a");
        let (input_b_s, input_b_r) = chan<s32>("input_b");
        let (output_s, output_r) = chan<s32>("output");

        spawn s32_mac_streaming(input_a_r, input_b_r, output_s);
        (input_a_s, input_b_s, output_r, terminator)
    }

    next(state: ()) {
        // 0 + 0*0 == 0
        trace_fmt!("[TB ]: Sending data (0,0)");
        let tok = send(join(), input_a_s, s32:0);
        let tok = send(tok, input_b_s, s32:0);
        let (tok, result) = recv(tok, output_r);
        assert_eq(result, s32:0);

        // 0 + 1*1 == 1
        trace_fmt!("[TB ]: Sending data (1,1)");
        let tok = send(tok, input_a_s, s32:1);
        let tok = send(tok, input_b_s, s32:1);
        let (tok, result) = recv(tok, output_r);
        assert_eq(result, s32:1);

        // 1 + 2*2 == 5
        trace_fmt!("[TB ]: Sending data (2,2)");
        let tok = send(tok, input_a_s, s32:2);
        let tok = send(tok, input_b_s, s32:2);
        let (tok, result) = recv(tok, output_r);
        assert_eq(result, s32:5);

        let tok = send(tok, terminator, true);
    }
}
