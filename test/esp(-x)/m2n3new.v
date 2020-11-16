// Benchmark "/home/hexiang/abc/build/test/esp(-x)/m2n3" written by ABC on Sat Nov 14 18:11:48 2020

module \/home/hexiang/abc/build/test/esp(-x)/m2n3  ( 
    x0, x1, x2, x3, x4,
    z0  );
  input  x0, x1, x2, x3, x4;
  output z0;
  assign z0 = ~x2 | (~x0 & ~x1 & (x4 | x3));
endmodule


