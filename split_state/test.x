proc user_module {
    output_l: chan<s32> out;
    output_r: chan<s32> out;

    config(output_l: chan<s32> out, output_r: chan<s32> out) { (output_l, output_r) }

    init { (s32:0, s32:0) }

    next(state: (s32, s32)) {
        let (state_l, state_r) = state;

        let next_state_l = state_l + s32:1;

        let temp1 = state_r * s32:100;
        let temp2 = temp1 - s32:100;
        let temp3 = temp2 << 3;
        let next_state_r = temp3 + s32:30;

        send(join(), output_l, next_state_l);
        send(join(), output_r, next_state_r);

        (next_state_l, next_state_r)
    }
}
