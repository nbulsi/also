module top( x0 , x1 , x2 , x3 , x4 , x5 , y0 );
  input x0 , x1 , x2 , x3 , x4 , x5 ;
  output y0 ;
  wire n7 , n8 , n9 , n10 ;
  assign n7 = ( ~x0 & x4 ) | ( ~x0 & x5 ) | ( x4 & x5 ) ;
  assign n8 = ( ~x0 & x2 ) | ( ~x0 & n7 ) | ( x2 & n7 ) ;
  assign n9 = ~x0 & n8 ;
  assign n10 = ( x1 & n7 ) | ( x1 & n9 ) | ( n7 & n9 ) ;
  assign y0 = n10 ;
endmodule
