#!/usr/bin/python
import re
 
line_test = "cirkit> prog adder";

def is_word_in( word, string ):
    return not( string.find(word) == -1)

#print( is_word_in( "adder", line_test ) )

name = ''
last_line = ''

with open( "log.txt", 'r') as f, open( "result.csv", 'w' ) as fw: 
    fw.write("benchmarks,xmg size,xmg depth,lut size,lut depth,cpu_time,xmg size,xmg depth,lut size,lut depth" + '\n')

    for line in f:
        if( is_word_in( "> prog", line ) ):
            name = line.split()[-1]
            fw.write( name + ',' )
            print name
        
        if( is_word_in( name+":", line ) ):
            num_xor = line.split()[-4]
            num_maj = line.split()[-7]
            lev = line.split()[-1]
            size_before = int(num_xor) + int(num_maj)
            fw.write( str( size_before ) + ',' + lev + ',')
            print size_before, lev
        
        if( is_word_in( "edge", line ) and is_word_in( name, line ) ):
            obj = re.search( r'.* nd = (.*\d+).*edge =.*', line );
            if( obj ):
                lut_size =  int( obj.group(1) ) 
            #lut_size  = line.split()[-10]
            lut_depth = line.split()[-1]
            fw.write( str( lut_size ) + ',' + lut_depth + ',')
            print lut_size, lut_depth
        
        if( is_word_in( "run-time", line ) and is_word_in( "After optimization", last_line ) ):
            time = line.split()[-2]
            fw.write( time + ',' )
            print time

        if( is_word_in( "unnamed", line ) ):
            num_xor = line.split()[-4]
            num_maj = line.split()[-7]
            lev = line.split()[-1]
            size_after = int(num_xor) + int(num_maj)
            fw.write( str( size_after ) + ',' + lev + ',')
            print size_after, lev
            
        if( is_word_in( "edge", line ) and is_word_in( "top", line ) ):
            obj = re.search( r'.* nd = (.*\d+).*edge =.*', line );
            if( obj ):
                lut_size =   int( obj.group(1) ) 
            #lut_size  = line.split()[-10]
            lut_depth = line.split()[-1]
            fw.write( str( lut_size )+ ',' + lut_depth + '\n')
            print lut_size, lut_depth

        last_line = line
            
            
            
            
            
            
            
            
            
            #searchObj = re.search( r'(.*)> prog (.*)', line )
            #print searchObj
            #if( searchObj ):
            #    print "benchmarks : ", searchObj.group(2)

#searchObj = re.search( r'(.*)> prog (.*)', line )
 
#if searchObj:
 #  print "searchObj.group() : ", searchObj.group()
 #  print "searchObj.group(1) : ", searchObj.group(1)
 #  print "searchObj.group(2) : ", searchObj.group(2)
#else:
 #  print "Nothing found!!"
