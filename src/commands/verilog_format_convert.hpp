/* also: Advanced Logic Synthesis is and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file test.hpp
 *
 * @b testing -write first also command
 *
 * @athor kunmh
 * @since 0.1
 */

#ifndef TEST_HPP
#define TEST_HPP
#include "../core/format_convert.hpp"
#include <alice/alice.hpp>
namespace alice
{
class verilog_format_convert_command : public command
{
public:
  explicit verilog_format_convert_command( const environment::ptr& env )
      : command( env, "for format conversion for verilog" )
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

ALICE_ADD_COMMAND( verilog_format_convert, "I/O" )

} // namespace alice

#endif
