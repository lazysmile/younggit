//&>/dev/null;x="${0%.*}";[ ! "$x" -ot "$0" ]||(rm -f "$x";g++ -o "$x" "$0")&&"$x" $*;exit
// =====================================================================================
// 
//       Filename:  hello.cpp
// 
//    Description:  hello world 2.0
// asdf
//        Version:  1.0
//        Created:  12/24/2012 02:11:59 PM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  Philip.Young (Cxy), yangcongxi@cdeledu.com
//        Company:  CDEL
// 
// =====================================================================================

#include<iostream>

int main(char *argv[],int argc)
{
	std::cout<<"hello world 2.0"<<std::endl;
	return 0;
}
