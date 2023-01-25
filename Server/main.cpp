//Example code: A simple server side code, which echos back the received message.
//Handle multiple socket connections with select and fd_set on Linux
//Based on: https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/
/************************************************/
/*
	Simple SPIFFS as DFS implementation
	Fun project, do not go prod ;)

	Write, Append, Delete, Read, Format - implemented
	Files will sync across nodes on 'LS'

	Mostly for plain text, but can be easily adopted
	to handle bytes instead.

	Zero security, zero intelligence - raw and pure.
*/
/************************************************/
#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h> //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>		// ++
#include <vector>		// ++
#include <map> 			// __
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include "md5.h"

#define TRUE 1
#define FALSE 0
#define PORT 8888

#define FILE_MIN_LEN 3
#define FILE_MAX_LEN 16
#define MAX_NODES 10
#define CLIENT_MAX_BUFF 1024
 
// We keep a copy of it on server for validation with nodes
struct tFile
{
	std::string m_strFile;
	std::string m_strData;
	std::string m_strMD5;
	int m_iSize;
};

struct tNode
{
	int m_Socket;
	int m_iLastPing;

	std::string GetIP()
	{
		struct sockaddr_in address; 
		int addrlen;
		getpeername(m_Socket , (struct sockaddr*)&address ,(socklen_t*)&addrlen); 
		return std::string(inet_ntoa(address.sin_addr) ) ;
	}

	bool WriteFile(const std::string& rstrName, const std::string& rstrData)
	{
		std::string strCommand = "WRITE,"+rstrName+","+rstrData+"\n";
		if( send(m_Socket , strCommand.c_str(), strCommand.size(), 0) != strCommand.size() )
		{
			perror("send");
			return false;
		} 
		return true;
	}
	bool AppendFile(const std::string& rstrName, const std::string& rstrData)
	{ 
		std::string strCommand = "APPEND,"+rstrName+","+rstrData+"\n";
		if( send(m_Socket , strCommand.c_str(), strCommand.size(), 0) != strCommand.size() )
		{
			perror("send");
			return false;
		} 
		return true;	 
	}	
	bool DeleteFile(const std::string&rstrName)
	{
		std::string strCommand = "DELETE,"+rstrName+"\n";
		if(send(m_Socket , strCommand.c_str(), strCommand.size(), 0) != strCommand.size() )
		{
			perror("send");
			return false;
		}
		return true;
	}
	bool Format()
	{
		std::string strCommand = "FORMAT\n";
		if( send(m_Socket , strCommand.c_str(), strCommand.size(), 0) != strCommand.size() )
		{
			perror("send");
			return false;
		} 
		return true;		
	} 
	std::string ReadFile(const std::string& rstrName)
	{

			std::string strCommand = "READ,"+rstrName+"\n";
			if(send(m_Socket , strCommand.c_str(), strCommand.size(), 0) != strCommand.size() )
			{
				perror("send");
				return "";
			}

			// Read the incoming data
            int32_t     iBytesRead;
            char        cBuffer[CLIENT_MAX_BUFF];
            std::string strReturn="";
            //
            iBytesRead = (int)recv(m_Socket, (char *)cBuffer, CLIENT_MAX_BUFF, 0); 
            if(cBuffer[0]=='R' && cBuffer[1]=='E'&&cBuffer[2]=='A'&&cBuffer[3]=='D')
            {
            	strReturn = "";
            	for(int c =0; c< iBytesRead; c++)
            		strReturn+=cBuffer[c]; 
            }
 
			return strReturn;
	}
	bool ReadFileB(const std::string& rstrName,const int&iPosition, const int&iCount)
	{

			std::string strCommand = "READB,"+std::to_string(iPosition)+","+std::to_string(iCount)+","+rstrName+"\n";
			if(send(m_Socket , strCommand.c_str(), strCommand.size(), 0) != strCommand.size() )
			{
				perror("send");
				return false;
			}

			// Read the incoming data
            int32_t     iBytesRead;
            char        cBuffer[iCount]; 
            //
            iBytesRead = (int)recv(m_Socket, (char *)cBuffer, CLIENT_MAX_BUFF, 0); 
            if(cBuffer[0]=='R' && cBuffer[1]=='E'&&cBuffer[2]=='A'&&cBuffer[3]=='D')
  			for(int x = 0; x < iBytesRead; x++)
  			{
  				printf("%c ", cBuffer[x]);
  			}
  			printf("\n");
			return true;
	}	
	std::string MD5File(const std::string& rstrName)
	{
			std::string strReturn = "";
			std::string strCommand = "MD5,"+rstrName+"\n";
			if(send(m_Socket, strCommand.c_str(), strCommand.size(), 0) != strCommand.size() )
			{
				perror("send"); 
			}	
			else
			{
				// Read the incoming data
	            int32_t     iBytesRead;
	            char        cBuffer[CLIENT_MAX_BUFF];
	            
	            //
	            iBytesRead = (int)recv(m_Socket, (char *)cBuffer, CLIENT_MAX_BUFF, 0); 
	            if(iBytesRead==37&&cBuffer[0]=='M' && cBuffer[1]=='D'&&cBuffer[2]=='5'&&cBuffer)
	            	for(int c =4; c< 36; c++)
	            		strReturn+=cBuffer[c]; 
			}
			return strReturn;
	}

}Node[MAX_NODES];

std::vector<tFile> m_vFiles;

bool HasFile(const std::string& rstrName)
{
	for(int x = 0; x < m_vFiles.size(); x++)
	{
		if(m_vFiles[x].m_strFile==rstrName)
			return true;
	}
	return false;
} 
tFile *GetFile(const std::string& rstrName)
{
	for(int x = 0;x < m_vFiles.size(); x++)
	{
		if(m_vFiles[x].m_strFile == rstrName)
			return &m_vFiles[x];
	}
	return NULL;
}
bool WriteFile(const std::string& rstrName, const std::string& rstrData)
{
	tFile _New;
	_New.m_strFile 	= rstrName;
	_New.m_strData 	= rstrData;
	_New.m_iSize 	= rstrData.size();

	MD5 m_MD5;
	m_MD5.update(rstrData.c_str(), rstrData.size());
	m_MD5.finalize();
	_New.m_strMD5 = m_MD5.toString(); 

	tFile *pFile = GetFile(rstrName);
	if(pFile)
	{
		pFile->m_strData = rstrData;
		pFile->m_iSize = rstrData.size();
		pFile->m_strMD5 = m_MD5.toString();
	}
	else
		m_vFiles.push_back(_New);

	// Replicate to all nodes
	for(int x = 0; x < MAX_NODES; x++)
	{
		if(Node[x].m_Socket != 0)
		{
			Node[x].WriteFile(rstrName, rstrData);
		}
	}
	return true;
}
bool AppendFile(const std::string& rstrName, const std::string& rstrData)
{
	tFile *pFile = GetFile(rstrName);
	if(pFile)
	{
		pFile->m_strData 	+= rstrData;
		pFile->m_iSize 		+= rstrData.size();
		MD5 m_MD5;
		m_MD5.update(pFile->m_strData.c_str(), pFile->m_strData.size());
		m_MD5.finalize();
		pFile->m_strMD5 = m_MD5.toString();

		for(int x = 0; x < MAX_NODES; x++)
		{
			if(Node[x].m_Socket  != 0)
			{  
				Node[x].AppendFile(rstrName,rstrData);
			}
		} 

		return true;
	}
	return false;
}	
void Format()
{

	for(int x = 0; x < MAX_NODES; x++)
	{
		if(Node[x].m_Socket  != 0)
		{  
			Node[x].Format();
		}
	}  
}
void DeleteFile(const std::string& rstrName)
{
	for(int x = 0; x < m_vFiles.size(); x++)
	{
		if(m_vFiles[x].m_strFile == rstrName)
		{
			m_vFiles.erase(m_vFiles.begin() + x);

			// Replicate
			for(int x = 0; x < MAX_NODES; x++)
			{
				if(Node[x].m_Socket  != 0)
				{ 
					Node[x].DeleteFile(rstrName);
				}
			} 
		}
	}
} 
bool ReadFileB(const std::string&rstrName, const int& iPosition, const int& iCount)
{

	tFile *pFile = GetFile(rstrName);

	if(pFile)
	{
		// Check boundaries 
		if(iPosition+iCount>pFile->m_iSize)
		{
			printf("Boundaries fail\n");
			return false;
		}

		if(iCount>512)
		{
			printf("Count MAX\b");
			return false;
		}

		// Find first valid hash and read the file 
		for(int x = 0; x < MAX_NODES; x++)
		{
			if(Node[x].m_Socket  != 0)
			{ 
				if(Node[x].MD5File(rstrName) == pFile->m_strMD5)
				{ 

					return Node[x].ReadFileB(rstrName,iPosition,iCount);
				}	
			}
		}

	}
	printf("File does not exist\n");
	return false;
}
bool ReadFile(const std::string&rstrName)
{

	tFile *pFile = GetFile(rstrName);

	if(pFile)
	{
		// Find first valid hash and read the file 
		for(int x = 0; x < MAX_NODES; x++)
		{
			if(Node[x].m_Socket  != 0)
			{ 
				if(Node[x].MD5File(rstrName) == pFile->m_strMD5)
				{ 
					printf("Getting file from node: %s\n", Node[x].GetIP().data());

					std::string strReceivedData = Node[x].ReadFile(rstrName);
					printf("-----------------\n");
					printf("%s\n", strReceivedData.data());
					printf("-----------------\n");
					return true;
				}	
			}
		}

	}
	printf("File does not exist\n");
	return false;
} 

// Helper
std::vector<std::string> SplitString(const std::string &strData, char delim)
{
	std::vector<std::string> output;
	std::string::size_type prev_pos = 0, pos = 0;

	while((pos = strData.find(delim, pos)) != std::string::npos)
	{
	    std::string substring( strData.substr(prev_pos, pos-prev_pos) );
	    output.push_back(substring);
	    prev_pos = ++pos;
	}

	output.push_back(strData.substr(prev_pos, pos-prev_pos)); 
	return output;
}

// Primitive input from the console
void HandleConsole()
{
	static std::string m_strConsoleCommandTemp;

	struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    if( !FD_ISSET(STDIN_FILENO, &fds) )
    { 
    }
    else 
    {
        int iCharacter = fgetc(stdin);

        // Enter
        if(iCharacter == 0x0A)
        {    
			printf(">>(%i) %s\n",m_strConsoleCommandTemp.size(), m_strConsoleCommandTemp.data());  

			std::vector<std::string> vCommand = SplitString(m_strConsoleCommandTemp, ',');

			//
            if(vCommand[0]=="QUIT")
            {
                exit(0);
            }
            // List files we know (server) and their hashes vs nodes
            else if(vCommand[0]=="LS")
            { 
            	for(int x = 0; x < m_vFiles.size(); x++)
            	{
            		printf("- %s (%ib) %s\n", m_vFiles[x].m_strFile.data(), m_vFiles[x].m_iSize, m_vFiles[x].m_strMD5.data());

					for(int z = 0; z < MAX_NODES;z++)
					{
						if(Node[z].m_Socket!=0)
						{
							bool bSynced = false;

							printf("-- %s@%s ",Node[z].GetIP().data(),Node[z].MD5File(m_vFiles[x].m_strFile).data(),bSynced);

							//
							if(Node[z].MD5File(m_vFiles[x].m_strFile) == m_vFiles[x].m_strMD5)
							{ 
								printf("(Checksum OK)\n");
							}					
							else 
							{
								// Replicate valid version 
								int iRet = Node[z].WriteFile(m_vFiles[x].m_strFile, m_vFiles[x].m_strData);
							
								if(iRet==1)
									printf("(Synced)\n");
								else
									printf("(Syncing)\n");

							}
						}
					}            		
            	}
            }            
            // Write data to file: <WRITE,filename,bytes_of_data>
            else if(vCommand[0]=="WRITE")
            {
            	if(vCommand.size()==3 && vCommand[1].size()>=FILE_MIN_LEN && vCommand[1].size()<=FILE_MAX_LEN && vCommand[2].size()!=0)
            	{
            		std::string strFilename = vCommand[1];
            		std::string strFileData = vCommand[2];
            		printf("Writing to file '%s' %i bytes of data\n", strFilename.data(), strFileData.size());
            		WriteFile(strFilename,strFileData);
 
            	}
            }
            // Append data to file: <WRITE,filename,bytes_of_data>
            else if(vCommand[0]=="APPEND")
            {
            	if(vCommand.size()==3 && vCommand[1].size()>=FILE_MIN_LEN && vCommand[1].size()<=FILE_MAX_LEN && vCommand[2].size()!=0)
            	{
            		std::string strFilename = vCommand[1];
            		std::string strFileData = vCommand[2];
            		printf("Writing to file '%s' %i bytes of data\n", strFilename.data(), strFileData.size()); 
            		AppendFile(strFilename,strFileData);
            	}
            }            
            // Delete file: <DELETE,filename>
            else if(vCommand[0]=="DELETE")
            {
            	if(vCommand.size()==2 && vCommand[1].size()>=FILE_MIN_LEN && vCommand[1].size()<=FILE_MAX_LEN)
            	{
            		std::string strFilename = vCommand[1]; 
            		printf("Deleting file '%s'\n", strFilename.data());
            		DeleteFile(strFilename);

            	}
            }                   
            // Read file: <READ,file>
            else if(vCommand[0]=="READ")
            {
            	if(vCommand.size()==2 && vCommand[1].size()>=FILE_MIN_LEN && vCommand[1].size()<=FILE_MAX_LEN)
            	{
            		std::string strFilename = vCommand[1]; 
            		printf("Reading file '%s'\n", strFilename.data());
            		ReadFile(strFilename);
            	}
            }
            // Read file bytes: <READB,file,position,count>
            else if(vCommand[0]=="READB")
            {
            	if(vCommand.size()==4 && vCommand[1].size()>=FILE_MIN_LEN && vCommand[1].size()<=FILE_MAX_LEN)
            	{
            		std::string strFilename = vCommand[1]; 
            		printf("Reading file '%s'\n", strFilename.data());
            		ReadFileB(strFilename,std::stoi(vCommand[2]),std::stoi(vCommand[3]));
            	}
            }            
			// Format <FORMAT>
            else if(vCommand[0]=="FORMAT")
            {
            	if(vCommand.size()==1)
            	{ 
            		printf("Formatting");
            		Format();
            	}
            }       
        	m_strConsoleCommandTemp.clear();

        }else 
            m_strConsoleCommandTemp+=(char)iCharacter;
    }
}
int main(int argc , char *argv[])
{
	

	int opt = TRUE;
	int master_socket , addrlen , new_socket  , activity, i , valread ;
	int max_sd;
	struct sockaddr_in address; 
		
	//set of socket descriptors		
    fd_set errorfds;
	fd_set readfds;
	//a message
	char *message = "ECHO Daemon v1.0 \r\n";
	
	//initialise all Node[] to 0 so not checked
	for (i = 0; i < MAX_NODES; i++)
	{
		Node[i].m_Socket  = 0;
	}
		
	//create a master socket
	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	
	//set master socket to allow multiple connections ,
	//this is just a good habit, it will work without this
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
		sizeof(opt)) < 0 )
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	
	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );
		
	//bind the socket to localhost port 8888
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	printf("Listener on port %d \n", PORT);
		
	//try to specify maximum of 3 pending connections for the master socket
	if (listen(master_socket, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
		
	//accept the incoming connection
	addrlen = sizeof(address);
	puts("Waiting for connections ...");
		
	while(TRUE)
	{ 
		//clear the socket set
		FD_ZERO(&readfds);
		FD_ZERO(&errorfds);
	
		//add master socket to set
		FD_SET(master_socket, &readfds);
		FD_SET(master_socket, &errorfds);
		max_sd = master_socket;
			
		//add child sockets to set
		for ( i = 0 ; i < MAX_NODES ; i++)
		{ 
				
			//if valid socket descriptor then add to read list
			if(Node[i].m_Socket > 0)
			{ 
				FD_SET( Node[i].m_Socket , &readfds);
				FD_SET( Node[i].m_Socket , &errorfds);
			}
				
			//highest file descriptor number, need it for the select function
			if(Node[i].m_Socket > max_sd)
				max_sd = Node[i].m_Socket;
		}
	 
	    timeval timeout; 
	    timeout.tv_sec = 0;
	    timeout.tv_usec = 0;

		//wait for an activity on one of the sockets  
		activity = select( max_sd + 1 , &readfds , NULL , &errorfds , &timeout);
	
		if ((activity < 0) && (errno!=EINTR))
		{
			printf("select error");
		}
			
		//If something happened on the master socket ,
		//then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
		{
			if ((new_socket = accept(master_socket,(struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}
			
			//inform user of socket number - used in send and receive commands
			printf("%s:%d Connected\n" ,  inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
		 
			//add new socket to array of sockets
			for (i = 0; i < MAX_NODES; i++)
			{
				//if position is empty
				if( Node[i].m_Socket  == 0 )
				{
					Node[i].m_Socket  = new_socket;   
					struct timeval timeout;      
				    timeout.tv_sec = 1;
				    timeout.tv_usec = 0;
				    
				    if (setsockopt (Node[i].m_Socket, SOL_SOCKET, SO_RCVTIMEO, &timeout,sizeof timeout) < 0)
				        perror("setsockopt failed\n");

				    if (setsockopt (Node[i].m_Socket, SOL_SOCKET, SO_SNDTIMEO, &timeout,sizeof timeout) < 0)
				        perror("setsockopt failed\n");


					break;
				}
			}
		}
			
		//else its some IO operation on some other socket
		for (i = 0; i < MAX_NODES; i++)
		{  


	        // This socket is set with Error descriptor, drop session
	        if (FD_ISSET(Node[i].m_Socket , &errorfds))
	        {
	            printf("%s Terminated\n", Node[i].GetIP().data());
					
	        }

	        // Disconnect
			if (FD_ISSET( Node[i].m_Socket , &readfds))
			{
				char buffer[1025];
				ssize_t rc = recv( Node[i].m_Socket , buffer, 1024,0); 
				if(rc<=0) 
				{
					printf("%s Disconnected\n", Node[i].GetIP().data()); 
					close( Node[i].m_Socket );
					Node[i].m_Socket = 0;
				} 
				else Node[i].m_iLastPing = time(0);
			}

			// Ping
			if(Node[i].m_Socket>0)
			{
				if(time(0) - Node[i].m_iLastPing >= 1)
				{ 
					if(send(Node[i].m_Socket , "PING,", 5, 0) != 5 ) 
					{ 
						printf("%s Timedout\n", Node[i].GetIP().data()); 
						close( Node[i].m_Socket );
						Node[i].m_Socket = 0;
					}

					Node[i].m_iLastPing = time(0);
				}
			}
		}
	          
	    HandleConsole(); 
	}
		
	return 0;
}