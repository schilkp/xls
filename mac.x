import apfloat;
import float32;

type F32 = float32::F32;

const F32_ZERO = float32::zero(false);
const F32_ONE = float32::one(false);

proc fp32_mac {

    input_a: chan<F32> in;
    input_b: chan<F32> in;
    reset: chan<bool> in;
    output: chan<F32> out;

    init { float32::zero(false) }

    config(input_a: chan<F32> in, input_b: chan<F32> in,
           reset: chan<bool> in, output: chan<F32> out) {
        (input_a, input_b, reset, output)
    }

    next(acc: F32) {
        let (tok0, a) = recv(join(), input_a);
        let (tok1, b) = recv(join(), input_b);
        let (tok2, do_reset) = recv(join(), reset);

        let acc = apfloat::fma<u32:8, u32:23>(a, b, acc);
        let zero = apfloat::zero<u32:8, u32:23>(false);
        let acc = if do_reset { zero } else { acc };

        let tok3 = join(tok0, tok1, tok2);
        send(tok3, output, acc);
        acc

    }
}

#[test_proc]
proc smoke_test {
    input_a_s: chan<F32> out;
    input_b_s: chan<F32> out;
    reset_s: chan<bool> out;
    output_r: chan<F32> in;
    terminator: chan<bool> out;

    init { () }

    config(terminator: chan<bool> out) {
        let (input_a_s, input_a_r) = chan<F32>("input_a");
        let (input_b_s, input_b_r) = chan<F32>("input_b");
        let (reset_s, reset_r) = chan<bool>("reset");
        let (output_s, output_r) = chan<F32>("output");
        spawn fp32_mac(input_a_r, input_b_r, reset_r, output_s);
        (input_a_s, input_b_s, reset_s, output_r, terminator)
    }

    next(state: ()) {

        // 0 + 0*0 == 0
        let tok = send(join(), input_a_s, F32_ZERO);
        let tok = send(tok, input_b_s, F32_ZERO);
        let tok = send(tok, reset_s, false);
        let (tok, result) = recv(tok, output_r);
        assert_eq(result, F32_ZERO);

        // 0 + 1*0 == 0
        let tok = send(tok, input_a_s, F32_ONE);
        let tok = send(tok, input_b_s, F32_ZERO);
        let tok = send(tok, reset_s, false);
        let (tok, result) = recv(tok, output_r);
        assert_eq(result, F32_ZERO);

        // 0 + 1*1 == 1
        let tok = send(tok, input_a_s, F32_ONE);
        let tok = send(tok, input_b_s, F32_ONE);
        let tok = send(tok, reset_s, false);
        let (tok, _) = recv(tok, output_r);

        // 1 + 1*1 == 2
        let tok = send(tok, input_a_s, F32_ONE);
        let tok = send(tok, input_b_s, F32_ONE);
        let tok = send(tok, reset_s, false);
        let (tok, result) = recv(tok, output_r);

        // Check that 1 < result < 3
        assert_eq(apfloat::gt_2(result, float32::cast_from_fixed_using_rne(s32:1)), true);
        assert_eq(apfloat::lt_2(result, float32::cast_from_fixed_using_rne(s32:3)), true);

        // 2 + 2*2 == 6
        let tok = send(tok, input_a_s, float32::cast_from_fixed_using_rne(s32:2));
        let tok = send(tok, input_b_s, float32::cast_from_fixed_using_rne(s32:2));
        let tok = send(tok, reset_s, false);
        let (tok, result) = recv(tok, output_r);

        // Check that 5 < result < 7
        assert_eq(apfloat::gt_2(result, float32::cast_from_fixed_using_rne(s32:5)), true);
        assert_eq(apfloat::lt_2(result, float32::cast_from_fixed_using_rne(s32:7)), true);

        // Reset:
        let tok = send(join(), input_a_s, F32_ZERO);
        let tok = send(tok, input_b_s, F32_ZERO);
        let tok = send(tok, reset_s, true);
        let (tok, result) = recv(tok, output_r);
        assert_eq(result, F32_ZERO);

        let tok = send(tok, terminator, true);
    }
}
