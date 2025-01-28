import std;

proc s32_add {

    in: chan<s32> in;
    out: chan<s32> out;

    init { s32:10 }

    config(input_a: chan<s32> in, result: chan<s32> out) {
        (input_a, result)
    }

    next(proc_state : (s32, s32, s32, s32)) {
        let (exec_state, loop_i, loop_sum, max) = proc_state;

        if (exec_state == BEGINNING) {
            // Receive:
            let (_, new_max) = recv(join(), input);
            (IN_LOOP, /*i=*/1, /*sum=*/0, /*max=*/new_max)

        } else if (exec_state == IN_LOOP) {
            // Loop Body:
            let new_sum = loop_i + loop_sum;
            let new_i = loop_i + 1;

            if (new_i < max) {
                // Loop cont.
                (IN_LOOP, /*i=*/new_i, /*sum=*/new_sum, /*max=*/max)
            } else {
                // Loop break.
                (AFTER_LOOP, /*i=*/new_i, /*sum=*/new_sum, /*max=*/max)
            }
        } else if (exec_state == AFTER_LOOP) {
            // Send:
            send recv(join(), loop_sum);
            (BEGINNING, /*i=*/1, /*sum=*/0, /*max=*/max)
        }
    }
}
