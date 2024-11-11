pub proc if_recv {
  A: chan<u32> in;
  B: chan<u32> in;
  C: chan<u32> out;

  // The initial value of the proc's state (empty in this case).
  init { () }

  // The interface used by anything that spawns this proc, which will need to
  // configure its inputs & outputs.
  config (A: chan<u32> in, B: chan<u32> in, C: chan<u32> out) {
    (A, B, C)
  }

  // The description of how this proc actually acts when running.
  next(st: ()) {
    let (tok_A, data_A) = recv(join(), A);
    let (tok_B, data_B) = recv(join(), B);

    let rslt = if (data_A > u32:10) {
        data_A + data_B
    } else {
        data_A
    };
    send(join(tok_A, tok_B), C, rslt);
  }
}


