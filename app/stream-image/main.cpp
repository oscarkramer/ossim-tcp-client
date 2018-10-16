#include <iostream>
#include "ossimTcpStreamClient.h"

using namespace std;

int main(int argc, char** argv)
{
   // Usage: <argv[0]> <path/to/OSSIM_DEV_HOME> <path-to-output.txt>
   if (argc < 3)
   {
      cout << "\nUsage:  " << argv[0] << " <path/to/OSSIM_BUILD_DIR> <path-to-output.txt>\n" << endl;
      return 1;
   }
   return 0;
}

