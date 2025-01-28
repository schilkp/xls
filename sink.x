proc sink_s32 {
    input: chan<s32> in;

    init { () }
    config(input_ch: chan<s32> in) {
        (input_ch,)
    }

    next(state: ()) {
        recv(join(), input);
    }
}

#[test_proc]
proc test{
    input_a_s: chan<s32> out;

    terminator: chan<bool> out;

    init { () }

    config(terminator: chan<bool> out) {
        // PROC IO
        let (input_a_s, input_a_r) = chan<s32>("input_a");
        spawn sink_s32(input_a_r);

        (input_a_s,terminator)
    }

    next(state: ()) {
        let tok = send(join(), input_a_s, s32:1);
        let tok = send(tok, input_a_s, s32:2);
        let tok = send(tok, input_a_s, s32:3);
        let tok = send(tok, input_a_s, s32:4);
        let tok = send(tok, input_a_s, s32:5);
        let tok = send(tok, input_a_s, s32:6);
        let tok = send(tok, input_a_s, s32:7);
        let tok = send(tok, input_a_s, s32:8);
        let tok = send(tok, input_a_s, s32:9);
        let tok = send(tok, input_a_s, s32:10);

        let tok = send(tok, terminator, true);
    }
}

