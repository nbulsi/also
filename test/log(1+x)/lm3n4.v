module top( x0 , x1 , x2 , x3 , x4 , x5 , x6 , y0 );
  input x0 , x1 , x2 , x3 , x4 , x5 , x6 ;
  output y0 ;
  wire n8 , n9 , n10 , n11 ;
  assign n8 = x1 & x5 ;
  assign n9 = ( x0 & ~x3 ) | ( x0 & x4 ) | ( ~x3 & x4 ) ;
  assign n10 = ( x0 & n8 ) | ( x0 & ~n9 ) | ( n8 & ~n9 ) ;
  assign n11 = ( ~x2 & x5 ) | ( ~x2 & n10 ) | ( x5 & n10 ) ;
  assign y0 = n11 ;
endmodule
