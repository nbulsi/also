

#ifndef a_HPP
#define a_HPP

#include <mockturtle/mockturtle.hpp>

#include <sstream>
//#include "../core/aig2xmg.hpp"

namespace alice
{
  class a_command : public command
  {
    public:
      explicit a_command( const environment::ptr& env ) : command( env, "" )
      {
      
      }
    
    protected:
      void execute()
      {
        mig_network mig1,mig2,mig3,mig4,mig5,mig6,mig7,mig8,mig9,mig10,mig11,mig12,mig13,mig14,mig15,mig16,mig17,mig18;
        
       // auto a=mig.create_pi();
       // auto b=mig.create_pi();
       // auto c=mig.create_pi();
        //auto d=mig.create_pi();
        
       // auto e=mig.create_maj(mig.get_constant( false ),c,d);
       // auto f=mig.create_maj(mig.get_constant( false ),b,e);
        //  mig.create_po(e);
         // mig.create_po(!f);
      
        //std::string s="file.v";
       //write_verilog( mig, s.c_str() );

       //sin m2n4
       auto a1 = mig1.create_pi();
       auto b1= mig1.create_pi();
       auto c1 = mig1.create_pi();
       auto d1 = mig1.create_pi();
       auto e1 = mig1.create_pi();
       auto f1 = mig1.create_pi();

       auto g1 = mig1.create_maj(!a1, e1, f1);
       auto h1 = mig1.create_maj(!a1, c1, g1);
       auto i1 = mig1.create_maj(mig1.get_constant( false ), !a1, h1);
       auto j1 = mig1.create_maj(b1, g1, i1);
        //mig1.create_po(g1);
        //mig1.create_po(h1);
        //mig1.create_po(i1);
        mig1.create_po(j1);

       std::string s1 = "sm2n4.v";
       write_verilog(mig1, s1.c_str());

       //sin m3n2
       auto a2 = mig2.create_pi();
       auto b2 = mig2.create_pi();
       auto c2 = mig2.create_pi();
       auto d2 = mig2.create_pi();
       auto e2 = mig2.create_pi();

       auto f2 = mig2.create_maj(mig2.get_constant(false), a2, e2);
       auto g2 = mig2.create_maj(!d2, e2, f2);
       auto h2 = mig2.create_maj(b2, c2, !g2);
       auto i2 = mig2.create_maj(c2, e2, !h2);
       //mig2.create_po(f2);
       //mig2.create_po(g2);
       //mig2.create_po(h2);
       mig2.create_po(i2);

       std::string s2 = "sm3n2.v";
       write_verilog(mig2, s2.c_str());

       //sin m3n3
       auto a3 = mig3.create_pi();
       auto b3 = mig3.create_pi();
       auto c3 = mig3.create_pi();
       auto d3 = mig3.create_pi();
       auto e3 = mig3.create_pi();
       auto f3 = mig3.create_pi();

       auto g3 = mig3.create_maj(mig3.get_constant(false), !b3, c3);
       auto h3 = mig3.create_maj(!a3, b3, e3);
       auto i3 = mig3.create_maj(f3, g3, h3);
       auto j3 = mig3.create_maj(e3, !f3, i3);
       auto k3 = mig3.create_maj(!d3, f3, j3);
       //mig3.create_po(g3);
       //mig3.create_po(h3);
       //mig3.create_po(i3);
       //mig3.create_po(j3);
       mig3.create_po(k3);

       std::string s3 = "sm3n3.v";
       write_verilog(mig3, s3.c_str());

       //cos m3n3
       auto a4 = mig4.create_pi();
       auto b4 = mig4.create_pi();
       auto c4 = mig4.create_pi();
       auto d4 = mig4.create_pi();
       auto e4 = mig4.create_pi();
       auto f4 = mig4.create_pi();

       auto g4 = mig4.create_maj(mig4.get_constant(false), e4, f4);
       auto h4 = mig4.create_maj(mig4.get_constant(false), c4, g4);
       //mig4.create_po(g4);
       mig4.create_po(!h4);

       std::string s4 = "cm3n3.v";
       write_verilog(mig4, s4.c_str());

       //cos m3n4
       auto a5 = mig5.create_pi();
       auto b5 = mig5.create_pi();
       auto c5 = mig5.create_pi();
       auto d5 = mig5.create_pi();
       auto e5 = mig5.create_pi();
       auto f5 = mig5.create_pi();
       auto g5 = mig5.create_pi();

       auto h5 = mig5.create_maj(!c5, e5, g5);
       auto i5 = mig5.create_maj(mig5.get_constant(false), c5, h5);
       //mig5.create_po(h5);
       mig5.create_po(!i5);

       std::string s5 = "cm3n4.v";
       write_verilog(mig5, s5.c_str());

       //cos m4n2
       auto a6 = mig6.create_pi();
       auto b6 = mig6.create_pi();
       auto c6 = mig6.create_pi();
       auto d6 = mig6.create_pi();
       auto e6 = mig6.create_pi();
       auto f6 = mig6.create_pi();

       auto g6 = mig6.create_maj(mig6.get_constant(false), b6, !d6);
       auto h6 = mig6.create_maj(!e6, f6, g6);
       auto i6 = mig6.create_maj(!a6, b6, c6);
       auto j6 = mig6.create_maj(f6, h6, i6);
       auto k6 = mig6.create_maj(mig6.get_constant(false), f6,!j6);
       //mig6.create_po(g6);
       //mig6.create_po(h6);
       //mig6.create_po(i6);
       //mig6.create_po(j6);
       mig6.create_po(!k6);

       std::string s6 = "cm4n2.v";
       write_verilog(mig6, s6.c_str());

       //esp(-x) m2n3
       auto a7 = mig7.create_pi();
       auto b7 = mig7.create_pi();
       auto c7 = mig7.create_pi();
       auto d7 = mig7.create_pi();
       auto e7 = mig7.create_pi();

       auto f7 = mig7.create_maj(a7, !c7, d7);
       auto g7 = mig7.create_maj(mig7.get_constant(false), b7, d7);
       auto h7 = mig7.create_maj(e7, f7,!g7);
       auto i7 = mig7.create_maj(a7, e7, !h7);
       //mig7.create_po(f7);
       //mig7.create_po(g7);
       //mig7.create_po(h7);
       mig7.create_po(!i7);

       std::string s7 = "em2n3.v";
       write_verilog(mig7, s7.c_str());

       //esp(-x) m2n4
       auto a8 = mig8.create_pi();
       auto b8 = mig8.create_pi();
       auto c8 = mig8.create_pi();
       auto d8 = mig8.create_pi();
       auto e8 = mig8.create_pi();
       auto f8 = mig8.create_pi();

       auto g8 = mig8.create_maj(mig8.get_constant(false), !a8, c8);
       auto h8 = mig8.create_maj(a8, b8, d8);
       auto i8 = mig8.create_maj(b8, f8, g8);
       auto j8 = mig8.create_maj(d8, !h8, i8);
       //mig8.create_po(g8);
       //mig8.create_po(h8);
       //mig8.create_po(i8);
       mig8.create_po(!j8);

       std::string s8 = "em2n4.v";
       write_verilog(mig8, s8.c_str());

       //esp(-x) m3n2
       auto a9 = mig9.create_pi();
       auto b9 = mig9.create_pi();
       auto c9 = mig9.create_pi();
       auto d9 = mig9.create_pi();
       auto e9 = mig9.create_pi();

       auto f9 = mig9.create_maj(mig9.get_constant(false), !a9, c9);
       auto g9 = mig9.create_maj(mig9.get_constant(false), b9, f9);
       auto h9 = mig9.create_maj(a9, d9, g9);
       auto i9 = mig9.create_maj(!d9, e9, h9);
       //mig9.create_po(f9);
       //mig9.create_po(g9);
       //mig9.create_po(h9);
       mig9.create_po(!i9);

       std::string s9 = "em3n2.v";
       write_verilog(mig9, s9.c_str());

       //log(1+x) m3n4
       auto a10 = mig10.create_pi();
       auto b10 = mig10.create_pi();
       auto c10 = mig10.create_pi();
       auto d10 = mig10.create_pi();
       auto e10 = mig10.create_pi();
       auto f10 = mig10.create_pi();
       auto g10 = mig10.create_pi();

       auto h10 = mig10.create_maj(mig10.get_constant(false), b10, f10);
       auto i10 = mig10.create_maj(a10, !d10, e10);
       auto j10 = mig10.create_maj(a10, h10, !i10);
       auto k10 = mig10.create_maj(!c10, f10, j10);
       //mig10.create_po(h10);
       //mig10.create_po(i10);
       //mig10.create_po(j10);
       mig10.create_po(k10);

       std::string s10 = "lm3n4.v";
       write_verilog(mig10, s10.c_str());

       //log(1+x) m4n3
       auto a11 = mig11.create_pi();
       auto b11 = mig11.create_pi();
       auto c11 = mig11.create_pi();
       auto d11 = mig11.create_pi();
       auto e11 = mig11.create_pi();
       auto f11 = mig11.create_pi();
       auto g11 = mig11.create_pi();

       auto h11 = mig11.create_maj(mig11.get_constant(false), !a11, d11);
       auto i11 = mig11.create_maj(c11, e11, h11);
       auto j11 = mig11.create_maj(mig11.get_constant(false), g11, !i11);
       auto k11 = mig11.create_maj(b11, f11, j11);
       //mig11.create_po(h11);
       //mig11.create_po(i11);
       //mig11.create_po(j11);
       mig11.create_po(k11);

       std::string s11 = "lm4n3.v";
       write_verilog(mig11, s11.c_str());

       //log(1+x) m5n2
       auto a12 = mig12.create_pi();
       auto b12 = mig12.create_pi();
       auto c12 = mig12.create_pi();
       auto d12 = mig12.create_pi();
       auto e12 = mig12.create_pi();
       auto f12 = mig12.create_pi();
       auto g12 = mig12.create_pi();

       auto h12 = mig12.create_maj(!b12, e12, f12);
       auto i12 = mig12.create_maj(c12, !g12, h12);
       auto j12 = mig12.create_maj(mig12.get_constant(false), !a12, f12);
       auto k12 = mig12.create_maj(f12, i12, j12);
       //mig12.create_po(h12);
       //mig12.create_po(i12);
       //mig12.create_po(j12);
       mig12.create_po(k12);

       std::string s12 = "lm5n2.v";
       write_verilog(mig12, s12.c_str());

       //sinpix m3n3
       auto a13 = mig13.create_pi();
       auto b13 = mig13.create_pi();
       auto c13 = mig13.create_pi();
       auto d13 = mig13.create_pi();
       auto e13 = mig13.create_pi();
       auto f13 = mig13.create_pi();

       auto g13 = mig13.create_maj(a13, b13, !c13);
       auto h13 = mig13.create_maj(mig13.get_constant(false), !a13, g13);
       auto i13 = mig13.create_maj(mig13.get_constant(false), d13, e13);
       auto j13 = mig13.create_maj(e13, !f13, h13);
       auto k13 = mig13.create_maj(mig13.get_constant(true), d13, f13);
       auto l13 = mig13.create_maj(!i13, j13, k13);
       //mig13.create_po(g13);
       //mig13.create_po(h13);
       //mig13.create_po(i13);
       //mig13.create_po(j13);
       //mig13.create_po(k13);
       mig13.create_po(l13);

       std::string s13 = "spm3n3.v";
       write_verilog(mig13, s13.c_str());

       //sinpix m4n4
       auto a14 = mig14.create_pi();
       auto b14 = mig14.create_pi();
       auto c14 = mig14.create_pi();
       auto d14 = mig14.create_pi();
       auto e14 = mig14.create_pi();
       auto f14 = mig14.create_pi();
       auto g14 = mig14.create_pi();
       auto h14 = mig14.create_pi();

       auto i14 = mig14.create_maj(e14, !f14, h14);
       auto j14 = mig14.create_maj(mig14.get_constant(false), !f14, g14);
       auto k14 = mig14.create_maj(!g14, i14, j14);
       auto l14 = mig14.create_maj(f14, !i14, j14);
       auto m14 = mig14.create_maj(mig14.get_constant(true), k14, l14);
       //mig14.create_po(i14);
       //mig14.create_po(j14);
       //mig14.create_po(k14);
       //mig14.create_po(l14);
       mig14.create_po(m14);

       std::string s14 = "spm4n4.v";
       write_verilog(mig14, s14.c_str());

       //sinpix m5n2
       auto a15 = mig15.create_pi();
       auto b15 = mig15.create_pi();
       auto c15 = mig15.create_pi();
       auto d15 = mig15.create_pi();
       auto e15 = mig15.create_pi();
       auto f15 = mig15.create_pi();
       auto g15 = mig15.create_pi();

       auto h15 = mig15.create_maj(mig15.get_constant(true), f15, g15);
       auto i15 = mig15.create_maj(mig15.get_constant(false), a15, c15);
       auto j15 = mig15.create_maj(mig15.get_constant(false), d15, !i15);
       auto k15 = mig15.create_maj(mig15.get_constant(false), f15, g15);
       auto l15 = mig15.create_maj(h15, j15, !k15);
       //mig15.create_po(h15);
       //mig15.create_po(i15);
       //mig15.create_po(j15);
       //mig15.create_po(k15);
       mig15.create_po(l15);

       std::string s15 = "spm5n2.v";
       write_verilog(mig15, s15.c_str());

       //tanh m2n4
       auto a16 = mig16.create_pi();
       auto b16 = mig16.create_pi();
       auto c16 = mig16.create_pi();
       auto d16 = mig16.create_pi();
       auto e16 = mig16.create_pi();
       auto f16 = mig16.create_pi();

       auto g16 = mig16.create_maj(b16, !c16, f16);
       auto h16 = mig16.create_maj(c16, !e16, g16);
       auto i16 = mig16.create_maj(a16, !f16, h16);
       auto j16 = mig16.create_maj(mig16.get_constant(true), a16, i16);
       auto k16 = mig16.create_maj(d16, g16, !j16);
       //mig16.create_po(g16);
       //mig16.create_po(h16);
       //mig16.create_po(i16);
       //mig16.create_po(j16);
       mig16.create_po(k16);

       std::string s16 = "tm2n4.v";
       write_verilog(mig16, s16.c_str());

       //tanh m3n2
       auto a17 = mig17.create_pi();
       auto b17 = mig17.create_pi();
       auto c17 = mig17.create_pi();
       auto d17 = mig17.create_pi();
       auto e17 = mig17.create_pi();

       auto f17 = mig17.create_maj(!a17,d17, e17);
       auto g17 = mig17.create_maj(mig17.get_constant(false), d17, !f17);
       auto h17 = mig17.create_maj(mig17.get_constant(true), b17, c17);
       auto i17 = mig17.create_maj(e17, g17, h17);
       //mig17.create_po(f17);
       //mig17.create_po(g17);
       //mig17.create_po(h17);
       mig17.create_po(i17);

       std::string s17 = "tm3n2.v";
       write_verilog(mig17, s17.c_str());

       //tanh m3n3
       auto a18 = mig18.create_pi();
       auto b18 = mig18.create_pi();
       auto c18 = mig18.create_pi();
       auto d18 = mig18.create_pi();
       auto e18 = mig18.create_pi();
       auto f18 = mig18.create_pi();

       auto g18 = mig18.create_maj(a18, b18, !c18);
       auto h18 = mig18.create_maj(b18, e18, g18);
       auto i18 = mig18.create_maj(a18, f18, !h18);
       auto j18 = mig18.create_maj(mig18.get_constant(false), f18, i18);
       auto k18 = mig18.create_maj(!d18, e18, j18);
       //mig18.create_po(g18);
       //mig18.create_po(h18);
       //mig18.create_po(i18);
       //mig18.create_po(j18);
       mig18.create_po(k18);

       std::string s18 = "tm3n3.v";
       write_verilog(mig18, s18.c_str());
      }
  };

  ALICE_ADD_COMMAND( a, "Rewriting" )
}

#endif
