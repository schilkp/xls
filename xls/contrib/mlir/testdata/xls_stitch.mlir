// RUN: xls_translate --mlir-xls-stitch %s -split-input-file 2>&1 | FileCheck %s

// CHECK: module top(
// CHECK:  eproc eproc_0 (
// CHECK:    .template_send(send),
// CHECK:    .template_send_rdy(send_rdy),
// CHECK:    .template_send_vld(send_vld),
// CHECK:    .template_recv(recv),
// CHECK:    .template_recv_rdy(recv_rdy),
// CHECK:    .template_recv_vld(recv_vld),
// CHECK:    .clk(clk),
// CHECK:    .rst(rst)
// CHECK:  );
xls.chan @send {recv_supported = false} : i32
xls.chan @recv {send_supported = false} : i32
xls.instantiate_eproc @eproc(@template_send as @send, @template_recv as @recv)

xls.chan @template_recv : i32
xls.chan @template_send : i32
xls.eproc @eproc(%state: i32) zeroinitializer discardable {
  xls.proc.yield %state : i32
}

// -----

// CHECK: module top(
// CHECK:  eproc eproc_0 (
// CHECK:    .template_send(send),
// CHECK:    .template_send_rdy(send_rdy),
// CHECK:    .template_send_vld(send_vld),
// CHECK:    .template_recv(recv),
// CHECK:    .template_recv_rdy(recv_rdy),
// CHECK:    .template_recv_vld(recv_vld),
// CHECK:    .clk(clk),
// CHECK:    .rst(rst)
// CHECK:  );
xls.chan @send {recv_supported = false} : i32
xls.chan @recv {send_supported = false} : i32
xls.instantiate_extern_eproc "eproc" ("template_send" as @send, "template_recv" as @recv)
