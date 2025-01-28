module {
  xls.chan @boundary1 {send_supported = false} : i32
  xls.chan @boundary2 {recv_supported = false} : i32
  xls.eproc @nop_0(%arg0: i32) zeroinitializer discardable {
    %0 = xls.after_all  : !xls.token
    %tkn_out, %result = xls.blocking_receive %0, @nop_arg0 : i32
    %1 = xls.send %tkn_out, %result, @nop_arg1 : i32
    xls.yield %arg0 : i32
  }
  xls.chan @nop_arg0 : i32
  xls.chan @nop_arg1 : i32
  xls.instantiate_eproc @nop_0 (@nop_arg0 as @boundary1, @nop_arg1 as @boundary2)
  xls.eproc @my_top_1(%arg0: i32) zeroinitializer discardable {
    xls.yield %arg0 : i32
  }
  xls.instantiate_eproc @my_top_1 ()
}

