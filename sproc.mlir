xls.sproc @nop(%req: !xls.schan<i32, in>, %resp: !xls.schan<i32, out>) {
  spawns {
    xls.yield %req, %resp : !xls.schan<i32, in>, !xls.schan<i32, out>
  }
  next (%req: !xls.schan<i32, in>, %resp: !xls.schan<i32, out>, %state: i32) zeroinitializer {
    %tok0 = xls.after_all : !xls.token
    %tok1, %val = xls.sblocking_receive %tok0, %req : (!xls.token, !xls.schan<i32, in>) -> (!xls.token, i32)
    %tok2 = xls.ssend %tok1, %val, %resp : (!xls.token, i32, !xls.schan<i32, out>) -> !xls.token
    xls.yield %state : i32
  }
}

xls.sproc @my_top(%req: !xls.schan<i32, in>, %resp: !xls.schan<i32, out>) top attributes {boundary_channel_names = ["boundary1", "boundary2"]} {
  spawns {
    xls.spawn @nop(%req, %resp) : !xls.schan<i32, in>, !xls.schan<i32, out>
    xls.yield
  }
  next (%state: i32) zeroinitializer {
    xls.yield %state : i32
  }
}
