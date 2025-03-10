# Channels & Proc I/O

[TOC]

## Proc I/O Flops

## Internal vs. External Channels

## Internal FIFOs in Verilog

### External Implementation

By default, XLS will emit instantiations of a fifo module that has to be
supplied externally for each internal FIFO.

FIFOs that carry data (resp. carry elements of a non-zero width type),
will result in instantiations of the `xls_fifo_wrapper` module, with the
following signature:

```systemverilog
module xls_fifo_wrapper #(
    parameter int Width,
    parameter int Depth,
    parameter bit EnableBypass,
    parameter bit RegisterPushOutputs,
    parameter bit RegisterPopOutputs
) (
    input logic clk,
    input logic rst,

    output logic             push_ready,
    input  logic             push_valid,
    input  logic [Width-1:0] push_data,

    input  logic             pop_ready,
    output logic             pop_valid,
    output logic [Width-1:0] pop_data
);

// ...

endmodule
```

FIFOs that carry no data (resp. carry elements of a zero width type),
will result in instantiations of the `xls_nodata_fifo_wrapper` module, with the
following signature:

```systemverilog
module xls_nodata_fifo_wrapper #(
    parameter int Depth,
    parameter bit EnableBypass,
    parameter bit RegisterPushOutputs,
    parameter bit RegisterPopOutputs
) (
    input logic clk,
    input logic rst,

    output logic             push_ready,
    input  logic             push_valid,

    input  logic             pop_ready,
    output logic             pop_valid
);

// ...

endmodule
```

This separation is needed because Verilog does not support parametrization
of module I/O or zero-width values.

XLS does not provide implementations of these modules.

### Materialized Channels

If the `materialize_internal_fifos` pass is enabled (see [codegen
options](./codegen_options.md#io-behavior)), all internal FIFOs are converted
to XLS IR blocks that implement their behaviour.

FIFO materialization is also applied to internal FIFOs in the JIT compiler, allowing them to be lowered
to LLVM IR without any special consideration.

!!! note

    FIFOs produced by materialization are designed to be functionally
    correct and maintainable, but are not necessarily optimized for
    performance.


