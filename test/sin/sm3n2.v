module top( x0 , x1 , x2 , x3 , x4 , y0 );
  input x0 , x1 , x2 , x3 , x4 ;
  output y0 ;
  wire n6 , n7 , n8 , n9 ;
  assign n6 = x0 & x4 ;
  assign n7 = ( ~x3 & x4 ) | ( ~x3 & n6 ) | ( x4 & n6 ) ;
  assign n8 = ( x1 & x2 ) | ( x1 & ~n7 ) | ( x2 & ~n7 ) ;
  assign n9 = ( x2 & x4 ) | ( x2 & ~n8 ) | ( x4 & ~n8 ) ;
  assign y0 = n9 ;
endmodule
