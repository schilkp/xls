import std;

proc user_module {

    input_a: chan<s32> in;
    input_b: chan<s32> in;
    output: chan<s32> out;

    init { s32:0 }

    config(input_a: chan<s32> in, input_b: chan<s32> in, output: chan<s32> out) {
        (input_a, input_b, output)
    }

    next(acc: s32) {

        let (_, a) = recv(join(), input_a);
        let (_, b) = recv(join(), input_b);

        let diff = a - b;

        let acc = if (diff >= s32:0) {
            let acc = acc + diff;
            let acc = acc + diff;
            let acc = acc + diff;
            let acc = acc + diff;
            let acc = acc + diff;
            let acc = acc + diff;
            let acc = acc + diff;
            acc
        } else {
            acc
        };

        send(join(), output, acc);

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

        spawn user_module(input_a_r, input_b_r, output_s);
        (input_a_s, input_b_s, output_r, terminator)
    }

    next(state: ()) {
        // (0, 0) -> 0
        trace_fmt!("[TB ]: Sending data (0,0)");
        let tok = send(join(), input_a_s, s32:0);
        let tok = send(tok, input_b_s, s32:0);

        let (tok, result) = recv(tok, output_r);
        trace_fmt!("[TB ]: Got {}.", result);
        // assert!(result == s32:0, "result == 0");

        // (1, 0) -> +1 -> +1*5 --> == s32:5
        trace_fmt!("[TB ]: Sending data (1,0)");
        let tok = send(tok, input_a_s, s32:1);
        let tok = send(tok, input_b_s, s32:0);

        let (tok, result) = recv(tok, output_r);
        trace_fmt!("[TB ]: Got {}.", result);
        // assert!(result == s32:5, "result == 5");

        // (1, 0) -> +1 -> +1*5 --> == s32:10
        trace_fmt!("[TB ]: Sending data (1,0)");
        let tok = send(tok, input_a_s, s32:1);
        let tok = send(tok, input_b_s, s32:0);

        let (tok, result) = recv(tok, output_r);
        trace_fmt!("[TB ]: Got {}.", result);
        // assert!(result == s32:10, "result == 10");

        // (0, 1) -> -1 --> == s32:10
        trace_fmt!("[TB ]: Sending data (0,1)");
        let tok = send(tok, input_a_s, s32:0);
        let tok = send(tok, input_b_s, s32:1);

        let (tok, result) = recv(tok, output_r);
        trace_fmt!("[TB ]: Got {}.", result);
        // assert!(result == s32:10, "result == 10");

        let tok = send(tok, terminator, true);
    }
}


