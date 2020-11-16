module top( x0 , x1 , x2 , x3 , x4 , x5 , y0 );
  input x0 , x1 , x2 , x3 , x4 , x5 ;
  output y0 ;
  wire n7 , n8 , n9 , n10 , n11 , n12 ;
  assign n9 = x3 & x4 ;
  assign n7 = ( x0 & x1 ) | ( x0 & ~x2 ) | ( x1 & ~x2 ) ;
  assign n8 = ~x0 & n7 ;
  assign n10 = ( x4 & ~x5 ) | ( x4 & n8 ) | ( ~x5 & n8 ) ;
  assign n11 = x3 | x5 ;
  assign n12 = ( ~n9 & n10 ) | ( ~n9 & n11 ) | ( n10 & n11 ) ;
  assign y0 = n12 ;
endmodule
