//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE file for license information
//
//**************************************************************************************************

#ifndef ossimTcpStreamClient_HEADER
#define ossimTcpStreamClient_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <string>
#include <memory>

/**
 * Utility class providing an TCP streaming of image data to server
 */
class OSSIM_DLL ossimTcpStreamClient
{
public:

   ossimTcpStreamClient();

   /** Closes any open connections. */
   ~ossimTcpStreamClient();

   /** Accepts hostname and portname as a string. Returns socket file descriptor or -1 if error. */
   int connectToServer(char* hostname, char* portname=NULL);

   /** Sets the desired image tile size */
   void setTileSize(unsigned int width, unsigned int height);

   /** Opens the specified image file for reading
    *  Returns true if successful. */
   bool open(const ossimFilename& imageFilePath);

   /** Performs asynchronous transfer
    *  Returns true if successful. */
   bool execute();

   /** Closes connection to server and ready to connect again. */
   bool disconnect();

protected:
   bool doMetadata();
   bool doRpcModelParams();
   bool doProjectionParams();
   bool doImageData();

   template<class T> bool streamBuffer(T* buffer, size_t num_words);
   uint8_t *m_buffer;
   void error(const char *msg);

   int m_svrsockfd;
   ossimRefPtr<ossimImageHandler> m_handler;
};

#endif
