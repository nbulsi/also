// Benchmark "top" written by ABC on Sat Nov 14 18:12:15 2020

module top ( 
    x0, x1, x2, x3, x4,
    y0  );
  input  x0, x1, x2, x3, x4;
  output y0;
  assign y0 = (~x4 | (~x0 & (~x1 | ~x2 | ~x3))) & (~x0 | (x3 ? ~x1 : ~x2));
endmodule


