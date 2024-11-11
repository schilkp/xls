import std;
import float32;

type F32 = float32::F32;

const F32_ZERO = float32::zero(false);
const F32_ONE = float32::one(false);

proc if_add_proc {

    input_a: chan<F32> in;
    input_b: chan<F32> in;
    output: chan<F32> out;

    init { F32_ZERO } // (accum, count)

    config(input_a: chan<F32> in, input_b: chan<F32> in, output: chan<F32> out) {
        (input_a, input_b, output)
    }

    next(acc: F32) {

        let (_, a) = recv(join(), input_a);
        let (_, b) = recv(join(), input_b);

        let diff = float32::sub(a,b);

        let acc = if (float32::gte_2(diff, F32_ZERO)) {
            float32::add(acc, diff)
        } else {
            acc
        };

        send(join(), output, acc);

        acc
    }
}

#[test_proc]
proc smoke_test {
    input_a_s: chan<F32> out;
    input_b_s: chan<F32> out;
    output_r: chan<F32> in;
    terminator: chan<bool> out;

    init { () }

    config(terminator: chan<bool> out) {
        let (input_a_s, input_a_r) = chan<F32>("input_a");
        let (input_b_s, input_b_r) = chan<F32>("input_b");
        let (output_s, output_r) = chan<F32>("output");

        spawn if_add_proc(input_a_r, input_b_r, output_s);
        (input_a_s, input_b_s, output_r, terminator)
    }

    next(state: ()) {
        // (0, 0) -> 0
        trace_fmt!("[TB ]: Sending data (0,0)");
        let tok = send(join(), input_a_s, F32_ZERO);
        let tok = send(tok, input_b_s, F32_ZERO);

        let (tok, result) = recv(tok, output_r);
        trace_fmt!("[TB ]: Got {}.", result);
        assert!(result == F32_ZERO, "result == 0");

        // (1, 0) -> +1 --> == 1
        trace_fmt!("[TB ]: Sending data (1,1)");
        let tok = send(tok, input_a_s, F32_ONE);
        let tok = send(tok, input_b_s, F32_ZERO);

        let (tok, result) = recv(tok, output_r);
        trace_fmt!("[TB ]: Got {}.", result);
        assert!(float32::gt_2(result, F32_ZERO), "result > 0");
        assert!(float32::lt_2(result, float32::from_int32(s32:2)), "result < 2");

        // (1, 0) -> +1 --> == 2
        trace_fmt!("[TB ]: Sending data (2,2)");
        let tok = send(tok, input_a_s, F32_ONE);
        let tok = send(tok, input_b_s, F32_ZERO);

        let (tok, result) = recv(tok, output_r);
        trace_fmt!("[TB ]: Got {}.", result);
        assert!(float32::gt_2(result, F32_ONE), "result > 1");
        assert!(float32::lt_2(result, float32::from_int32(s32:3)), "result < 3");

        let tok = send(tok, terminator, true);
    }
}

