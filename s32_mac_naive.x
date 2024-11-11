
pub fn s32_mac_naive<N:u32>(
    input_a: s32[N],
    input_b: s32[N]) -> s32 {
    for (idx, accum): (u32, s32) in range(u32:0, N) {
        accum + input_a[idx] * input_b[idx]
    }(s32:0)
}

pub fn s32_mac_naive_wrapper(input_a: s32[10], input_b: s32[10]) -> s32 {
    s32_mac_naive(input_a, input_b)
}

#[test]
fn fir_filter_fixed_test() {
   let input_a = s32[4]:[0, 1, 2, 3];
   let input_b = s32[4]:[0, 1, 2, 3];
   let result = s32_mac_naive(input_a, input_b);
   assert_eq(result, s32:14);
}
