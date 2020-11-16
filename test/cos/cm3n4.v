module top( x0 , x1 , x2 , x3 , x4 , x5 , x6 , y0 );
  input x0 , x1 , x2 , x3 , x4 , x5 , x6 ;
  output y0 ;
  wire n8 , n9 ;
  assign n8 = ( ~x2 & x4 ) | ( ~x2 & x6 ) | ( x4 & x6 ) ;
  assign n9 = x2 & n8 ;
  assign y0 = ~n9 ;
endmodule
