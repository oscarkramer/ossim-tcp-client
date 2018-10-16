//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE file for license information
//
//**************************************************************************************************

#include "ossimTcpStreamClient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageSourceSequencer.h>
#include <unistd.h>

using namespace std;

#define MAX_BUF_LEN 4096
#define _DEBUG_ false

ossimTcpStreamClient::ossimTcpStreamClient()
:  m_svrsockfd(-1),
   m_buffer(new uint8_t[MAX_BUF_LEN])
{
   ossimFilename tmpdir = "/tmp";
}

ossimTcpStreamClient::~ossimTcpStreamClient()
{
   disconnect();
}

bool ossimTcpStreamClient::disconnect()
{
   bool success = true;

   if (m_svrsockfd < 0)
      return success;

   // Need to send close request to server:
   if (close(m_svrsockfd) < 0)
   {
      error("Error closing socket");
      success = false;
   }

   m_svrsockfd = -1;
   return success;
}


void ossimTcpStreamClient::error(const char *msg)
{
   perror(msg);
}


int ossimTcpStreamClient::connectToServer(char* hostname, char* portname)
{
   m_svrsockfd = -1;
   while (1)
   {
      // Consider port number in host URL:
      ossimString host (hostname);
      ossimString port;
      if (portname)
         port = portname;
      else if (host.contains(":"))
      {
         port = host.after(":");
         host = host.before(":");
      }

      if (port.empty())
      {
         // Maybe implied port then '/':
         port = host.after("/");
         port = "/" + port;
         host = host.before("/");
      }

      // Establish full server address including port:
      struct addrinfo hints;
      memset(&hints, 0, sizeof hints); // make sure the struct is empty
      hints.ai_family = AF_INET;     // don't care IPv4 or IPv6
      hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
      hints.ai_canonname = host.stringDup();     // fill in my IP for me
      struct addrinfo *res;
      int failed = getaddrinfo(host.stringDup(), port.stringDup(), &hints, &res);
      if (failed)
      {
         error(gai_strerror(failed));
         break;
      }

      // Create socket for this server by walking the linked list of available addresses:
      struct addrinfo *server_info = res;
      while (server_info)
      {
         m_svrsockfd =
               socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
         if (m_svrsockfd >= 0)
            break;
         server_info = server_info->ai_next;
      }
      if ((m_svrsockfd < 0) || (server_info == NULL))
      {
         error("Error opening socket");
         break;
      }

      // Connect to the server:portno:
      struct sockaddr_in* serv_addr = (sockaddr_in*) server_info->ai_addr;
      serv_addr->sin_family = AF_INET;
      if (connect(m_svrsockfd,(struct sockaddr*) serv_addr, server_info->ai_addrlen) < 0)
      {
         error("ERROR connecting");
         disconnect();
      }

      break;
   }
   return m_svrsockfd;
}

bool ossimTcpStreamClient::open(const ossimFilename& imageFilePath)
{
   // Let OSSIM open the image file:
   m_handler = ossimImageHandlerRegistry::instance()->open(imageFilePath);
   if (!m_handler)
   {
      error("ERROR opening input file.");
      return false;
   }
}

bool ossimTcpStreamClient::execute()
{
   bool success = false;
   do
   {
      if (!m_handler)
         break;

      // Fetch image parameters and steam:
      if (!doMetadata())
         break;

      // Must be RPC model. Fetch model parameters and stream:
      if (!doRpcModelParams())
         break;

      // Determine optimal UTM projection and fetch parameters:
      if (!doProjectionParams())
         break;

      // Finally stream pixels in tiled format:
      if (!doImageData())
         break;

      success = true;

   } while (false);

   return success;
}

bool ossimTcpStreamClient::doMetadata()
{
   size_t bufsize = 3;
   auto *buf = new uint32_t [bufsize];

   // Fetch image size and marshal:

   // Stream buffer:
   return streamBuffer<uint32_t>(buf, bufsize);
}

bool ossimTcpStreamClient::doRpcModelParams()
{
   size_t bufsize = 100;
   auto *buf = new double [bufsize];

   // Insure RPC exists:

   // Fetch RPC parameters and marshal with proper tags:

   // Stream buffer:
   return streamBuffer<double>(buf, bufsize);
}

bool ossimTcpStreamClient::doProjectionParams()
{
   size_t bufsize = 100;
   auto *buf = new double [bufsize];

   // Determine optimal UTM projection:

   // Fetch parameters and marshal:

   // Stream buffer with proper tags:
   return streamBuffer<double>(buf, bufsize);
}

bool ossimTcpStreamClient::doImageData()
{
   // Ready to stream pixels as tiles:
   ossimRefPtr<ossimImageSourceSequencer> sequencer = new ossimImageSourceSequencer(
         m_handler.get());
   ossimRefPtr<ossimImageData> tile = sequencer->getNextTile();

   while (tile)
   {

      // Stream buffer with proper tags:
      //return streamBuffer<double>(m_buffer, MAX_BUF_LEN);

   }
   return true;
}

template<class T> bool ossimTcpStreamClient::streamBuffer(T* buffer, size_t num_words)
{
   return false;
}


