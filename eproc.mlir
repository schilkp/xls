module {
  xls.chan @boundary1 {send_supported = false} : i32
  xls.chan @boundary2 {recv_supported = false} : i32
  xls.eproc @nop_0_1(%arg0: i32) zeroinitializer {
    %0 = xls.after_all  : !xls.token
    %tkn_out, %result = xls.blocking_receive %0, @boundary1 : i32
    %1 = xls.send %tkn_out, %result, @boundary2 : i32
    xls.yield %arg0 : i32
  }
  xls.eproc @my_top_1_0(%arg0: i32) zeroinitializer {
    xls.yield %arg0 : i32
  }
}

