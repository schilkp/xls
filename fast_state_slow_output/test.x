proc user_module {
    output_s: chan<s32> out;

    config(output_s: chan<s32> out) { (output_s,) }

    init { (s32:0) }

    next(state: s32) {
        let next_state = state + s32:1;

        let temp1 = next_state * s32:100;
        let temp2 = temp1 - s32:100;
        let temp3 = temp2 << 3;

        send(join(), output_s, temp3);

        next_state
    }
}
