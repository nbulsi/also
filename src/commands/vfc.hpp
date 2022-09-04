/* also: Advanced Logic Synthesis is and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file vfc.hpp
 *
 * @b verilog_format_convert command
 *
 * @author Ruibing Zhang
 * @author Zhufei Chu
 * @since 0.1
 */

#ifndef VFC_HPP
#define VFC_HPP
#include "../core/format_convert.hpp"

namespace alice
{
class vfc_command : public command
{
public:
  explicit vfc_command( const environment::ptr& env )
      : command( env, "verilog format conversion for EDA competition" )
  {
    add_option( "filename, -f", filename, "the input filename.v file name,the output filename is _filename.v" );
  }

protected:
  void execute()
  {
    string inputfile_name = filename;
    string outputfile_name = "_" + filename;
    mag_convert( inputfile_name, outputfile_name );
  }

private:
  string filename;
};

ALICE_ADD_COMMAND( vfc, "I/O" )

} // namespace alice

#endif
