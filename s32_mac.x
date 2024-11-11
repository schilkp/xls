
proc s32_mac<N:u32> {

    input_a: chan<s32> in;
    input_b: chan<s32> in;
    output: chan<s32> out;

    init { (s32:0, u32:0) }

    config(input_a: chan<s32> in, input_b: chan<s32> in, output: chan<s32> out) {
        (input_a, input_b, output)
    }

    next(state: (s32, u32)) {

        let (acc, count) = state;

        let (tok0, a) = recv(join(), input_a);
        let (tok1, b) = recv(join(), input_b);

        // trace_fmt!("[DUT]: Got ({},{})", a, b);

        let count = count + u32:1;
        let acc = acc + a * b;

        if (count == N) {
            trace_fmt!("[DUT]: Done. Sending {}", acc);
            send(join(tok0, tok1), output, acc);
            (s32:0, u32:0)
        } else {
            trace_fmt!("[DUT]: Counting. count: {}", count, acc);
            (acc, count)
        }
    }
}


proc s32_mac_wrapper {

    init { () }

    config(input_a: chan<s32> in, input_b: chan<s32> in, output: chan<s32> out) {
        spawn s32_mac<u32:100>(input_a, input_b, output);
        ()
    }

    next(state: ()) { }
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

        spawn s32_mac<u32:3>(input_a_r, input_b_r, output_s);
        (input_a_s, input_b_s, output_r, terminator)
    }

    next(state: ()) {
        // 0 + 0*0 == 0
        trace_fmt!("[TB ]: Sending data (0,0)");
        let tok = send(join(), input_a_s, s32:0);
        let tok = send(tok, input_b_s, s32:0);

        // 0 + 1*1 == 1
        trace_fmt!("[TB ]: Sending data (1,1)");
        let tok = send(tok, input_a_s, s32:1);
        let tok = send(tok, input_b_s, s32:1);

        // 1 + 2*2 == 5
        trace_fmt!("[TB ]: Sending data (2,2)");
        let tok = send(tok, input_a_s, s32:2);
        let tok = send(tok, input_b_s, s32:2);

        // Receive + check:
        trace_fmt!("[TB ]: Receiving..");
        let (tok, result) = recv(tok, output_r);

        trace_fmt!("[TB ]: Got {}.", result);

        assert_eq(result, s32:5);

        let tok = send(tok, terminator, true);
    }
}
