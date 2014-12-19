/*
	WARNING!!!!!!!
	Consider the case where a Device is created from a temporary stream (stringstream or istream).
	Our constructors require an address or pointer to this stream. As far as the implementation
	of C++ basic_istream, for example, is concerned this originating stream MUST NOT go out of scope.
	You will have to allocate memory for this stream or ensure that is always in scope.
	
	If any questions, ask us.
*/


#ifndef DEVICES_H
#define DEVICES_H 

#include <pthread.h>
#include <semaphore.h>
#include <climits>

#include <map>
#include <queue>
#include <iomanip>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <cassert>
#include <cstring>
#include <cstdio>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sstream>
#include <errno.h>
#include <assert.h>
#include <mutex>
#include <condition_variable>

#include "systems.h"


//Interrupts and IOCTL header
#ifdef __linux
#include <linux/ioctl.h>
#elif __APPLE__
#include <sys/ioctl.h>
#endif

/*
	stream flags in octal - base eight
*/
#define ODD_RDONLY 	00
#define ODD_WRONLY 	01
#define ODD_RDWR 	02

/*
	IOCTL Device Macros
*/
#define ODD_HARDRESET 001
#define ODD_FIONBIO 002
#define ODD_FIONREAD 003
#define	ODD_FIONWRITE 004

using namespace std;

class Device;


extern map<string,Device*> drivers;
extern vector<int> freedDeviceNumbers;

// Here is the section about interrupt handlers

void readCompletionHandler(int deviceIndex);
void writeCompletionHandler(int deviceIndex);

// Utility function to access Device ptr from a FD
Device * getDeviceFd(int fd);

class Device: public Monitor {
public:
	int deviceNumber;
	string driverName;
	
	Inode *inode;
	int inodeCount = 0;
	int openCount = 0;
	bool readable;
	bool writeable;
	int bytesRead;
	int bytesWritten;

	Device(string dn);
	~Device();
	
	virtual int open(const char* pathname, int flags);
	virtual int close();
	virtual int read();
	virtual int seek();
	virtual int rewind();
	virtual int ioctl(unsigned int cmd, unsigned long arg);
	virtual void online();
	virtual void offline();
	virtual void fireup();
	virtual void suspend();
	virtual void shutdown();
	virtual void initialize();
	virtual void finalize();
	virtual void completeRead();

	virtual void completeWrite();
	virtual int write();
};

//--------------------------------------------------

template< typename Item >
class iDevice : public Device {
	
	istream *stream;
	stringstream sstream;
	fstream* fstreamy;
	int offsetIn = 0;
  	int bufSize = 1024;//used for overloaded operators
  
	public:
	iDevice( istream *stream )
		: stream(stream), 
		Device("iDevice")
	{
		offsetIn = 0;
		stream->seekg(offsetIn,ios_base::beg);
	}

	iDevice( string s )
		: sstream(s), 
		Device("iDevice")
	{
		offsetIn = 0;
		stream = &sstream;
		stream->seekg(offsetIn,ios_base::beg);
	}

	iDevice( fstream* s )
		: fstreamy(s), 
		Device("iDevice")
	{
		offsetIn = 0;
		stream = fstreamy;
		//static_cast<istream*>(&sstream);
		stream->seekg(offsetIn,ios_base::beg);
	}

	//CONDITION ok2read;
	bool readCompleted;

	void completeRead() {
		//EXCLUSION
		cout << "readComplete" << endl;
		readCompleted = true;
		//ok2read.SIGNAL;
	}

	int read(Item buffer, size_t count) {
		//EXCLUSION
		int i;
		off_t save = stream->tellg();
		stream->seekg(0, ios_base::end);
		off_t end = stream->tellg();
		stream->seekg(save);
		for(i = 0; i < count; i++)
		{
			int tmp = stream->tellg();
			if(!stream||tmp>(end-1))
			{
				stream->clear(); //clear eofbit after reading eof
				return (bytesRead = i);
			}
			
			//read character from bytes then store into buffer
			buffer[i] = stream->get();

			if (tmp<0)
			{
				stream->clear(); //clear eofbit after reading eof
				return (bytesRead = i);
			}
			//cout << "Reading '" << (char)buffer[i] << "' from position tellg() " << tmp
			//	<< " and position offset " << offsetIn << " of device '" << this->driverName << "'" << endl;
			offsetIn++;

			readCompleted = false;
			readCompleted = true;
			//while(!readCompleted) ok2read.WAIT;
		}
		return (bytesRead = i);
	}

	int seek(off_t newOffset, int whence) {
		//EXCLUSION

		//get end position to save in our offset variable
		off_t save = stream->tellg();
		stream->seekg(0, ios_base::end);
		off_t end = stream->tellg();
		stream->seekg(save);

		//set position to passed in position
		if(whence == SEEK_SET)
		{
			//save position in our offset variable
			offsetIn = newOffset;
		}

		//set position to current position + passed in position
		else if(whence == SEEK_CUR)
		{
			//save position in our offset variable
			offsetIn = offsetIn + newOffset;
		}
		else if(whence == SEEK_END)
		{
			//save position in our offset variable
			offsetIn = (end-1) + newOffset;
		}	
		
		if(offsetIn > (end-1))
		{
			offsetIn = end-1;
		}

		if(offsetIn < 0)
		{
			offsetIn = 0;
		}
		//cout << "iDevice::seek()::offsetIn -: " << offsetIn << endl;

		//set positions 
		stream->seekg(offsetIn, ios_base::beg);
		return offsetIn;
	}
	int rewind() {
		//cout << "iRewind!\n";
		seek(0,SEEK_SET);
		return 0;
	}

	friend iDevice<Item> &operator>>( iDevice<Item> &lhs, Item rhs )
	{
		lhs.rewind();
		lhs.read(rhs,lhs.bufSize);
		return lhs;
	}

	friend iDevice<Item> &operator>>( iDevice<Item> &lhs, int rhs )
	{
		lhs.bufSize = rhs;
		return lhs;
	}

};

//--------------------------------------------------

template< typename Item >
class oDevice : public Device {

	ostream *stream;
	stringstream sstream;
	fstream* fstreamy;
	int offsetOut = 0;

	//helper function doPadding only works for write, not read
	void doPadding(off_t start, off_t end)
	{
		stream->seekp(start,ios_base::beg);
		for(int i = start; i <= end; i++ )	
		{
			//cout << "Padding: " << stream->tellp() << endl;
			stream->put('\0');
		}
	}
public:
	oDevice( ostream *stream )
		: stream(stream), 
		Device("oDevice")
	{
		offsetOut = 0;
		stream->seekp(offsetOut,ios_base::beg);
	}

	oDevice( string s )
		: sstream(s), 
		Device("oDevice")
	{
		offsetOut = 0;
		stream = &sstream;
		stream->seekp(offsetOut,ios_base::beg);
	}

	oDevice( fstream* s )
		: fstreamy(s), 
		Device("oDevice")
	{
		offsetOut = 0;
		stream = fstreamy;
		stream->seekp(offsetOut,ios_base::beg);
	}

	//CONDITION ok2write;
	bool writeCompleted;

	int output( Item buffer, int n ) {
  		return -1;
	}

	void completeWrite() {
		//EXCLUSION
		cout <<"writeComplete" << endl;
		writeCompleted = true;
		//ok2write.SIGNAL;
	}

	int write(Item buffer, size_t count) {
		//EXCLUSION
		off_t save = stream->tellp();
		stream->seekp(0, ios_base::end);
		off_t end = stream->tellp();
		stream->seekp(save);

		int i;
		for(i = 0; i < count; i++)
		{
			int tmp = stream->tellp();
			if(!stream)
			{
				stream->clear(); //clear eofbit after reading eof
				return (bytesWritten = i);
			}
			
			//put character from buffer into stream
			stream->put(buffer[i]);

			if (tmp<0)
			{
				stream->clear(); //clear eofbit after reading eof
				return (bytesWritten = i);
			}
			//cout << "Writing '" << (char)buffer[i] << "' from position tellp() " << tmp
			//	<< " and position offset " << offsetOut << " of device '" << this->driverName << "'" << endl;
			offsetOut++;

			writeCompleted = false;
			writeCompleted = true;
			//while(!writeCompleted) ok2write.WAIT;
		}
		return (bytesWritten = i);
	}

	int seek(off_t newOffset, int whence) {
		//EXCLUSION

		//get end position to save in our offset variable
		off_t save = stream->tellp();
		stream->seekp(0, ios_base::end);
		off_t end = stream->tellp();
		stream->seekp(save);

		//set position to passed in position
		if(whence == SEEK_SET)
		{
			//save position in our offset variable
			offsetOut = newOffset;
		}

		//set position to current position + passed in position
		else if(whence == SEEK_CUR)
		{

			//save position in our offset variable
			offsetOut = offsetOut + newOffset;
		}

		else if(whence == SEEK_END)
		{
			//save position in our offset variable
			offsetOut = (end-1) + newOffset;
		}
			
		
		if(offsetOut > (end-1))
		{
			doPadding(end,offsetOut);
		}

		if(offsetOut < 0)
		{
			offsetOut = 0;
		}

		//cout << "oDevice::seek()::offsetOut -: " << offsetOut << endl;
		//set positions 
		stream->seekp(offsetOut, ios_base::beg);
		return offsetOut;
	}

	int rewind() {
		//cout << "oRewind!\n";
		seek(0,SEEK_SET);
		return 0;
	}
};


template< typename Item >
class ioDevice : public iDevice<Item>, public oDevice<Item> {

  iostream *stream;
  stringstream sstream;
  fstream* fstreamy;
  int offset = 0;
  int bufSize = 1024;

public:

	ioDevice( iostream *s )  
		:stream(s),
		 iDevice<Item>(s), oDevice<Item>(s)
	{
		offset = 0;
	}

	ioDevice( string s )  
		:sstream(s), 
		 iDevice<Item>(s), oDevice<Item>(s)
	{
		offset = 0;
		stream = &sstream;
	}

	ioDevice( fstream* s )  
		:fstreamy(s), 
		 iDevice<Item>(s), oDevice<Item>(s)
	{
		offset = 0;
		stream = fstreamy;
	}


	int seek(off_t newOffset, int whence) {
		if (iDevice<Item>::readable==true&&iDevice<Item>::writeable==false)
		{
			return iDevice<Item>::seek(newOffset,whence);
		}
		else if (iDevice<Item>::readable==false&&iDevice<Item>::writeable==true)
		{
			return oDevice<Item>::seek(newOffset,whence);
		}
		else if (iDevice<Item>::readable==true&&iDevice<Item>::writeable==true)
		{
			iDevice<Item>::seek(newOffset,whence);
			return oDevice<Item>::seek(newOffset,whence);
		}
		return -1;
	}

	int rewind() {
		//cout << "Inside ioDevice rewind()\n";
		if (iDevice<Item>::readable==true&&iDevice<Item>::writeable==false)
		{
			return iDevice<Item>::rewind();
		}
		else if (iDevice<Item>::readable==false&&iDevice<Item>::writeable==true)
		{
			return oDevice<Item>::rewind();
		}
		else if (iDevice<Item>::readable==true&&iDevice<Item>::writeable==true)
		{
			iDevice<Item>::rewind();
			return oDevice<Item>::rewind();
		}
		return -1;
	}

	int open(const char* pathname, int flags) {
		int dn = -1;
		if( !(flags & 0x01) ) {
			dn = iDevice<Item>::open(pathname,flags);
		}
		if ((flags & 0x01) | (flags & 0x02)) {
			dn = oDevice<Item>::open(pathname,flags);
		}
		return dn;
	}

	int close() {
		iDevice<Item>::close();
		return oDevice<Item>::close();
	}

	int read(Item buffer, size_t count) {
		return iDevice<Item>::read(buffer,count);
	}

	int write(Item buffer, size_t count) {
		return oDevice<Item>::write(buffer,count);
	}

	void online() {
		iDevice<Item>::online();
		oDevice<Item>::online();
	}

	void offline() {
		iDevice<Item>::offline();
		oDevice<Item>::offline();
	}

	void fireup() {
		iDevice<Item>::fireup();
		oDevice<Item>::fireup();
	}

	void suspend() {
		iDevice<Item>::suspend();
		oDevice<Item>::suspend();
	}

	void shutdown() {
		iDevice<Item>::shutdown();
		oDevice<Item>::shutdown();
	}
	void initialize() {
		iDevice<Item>::initialize();
		oDevice<Item>::initialize();
	}
	void finalize() {
		iDevice<Item>::finalize();
		oDevice<Item>::finalize();
	}

	friend ioDevice<Item> &operator<<( ioDevice<Item> &lhs, Item rhs )
	{
		//lhs.rewind();
		lhs.write(rhs,lhs.bufSize);
		return lhs;
	}

	friend ioDevice<Item> &operator>>( ioDevice<Item> &lhs, Item rhs )
	{
		lhs.rewind();
		lhs.read(rhs,lhs.bufSize);
		return lhs;
	}

	friend ioDevice<Item> &operator>>( ioDevice<Item> &lhs, int rhs )
	{
		lhs.bufSize = rhs;
		return lhs;
	}

	friend ioDevice<Item> &operator<<( ioDevice<Item> &lhs, int rhs )
	{
		lhs.bufSize = rhs;
		return lhs;
	}

	char operator[](int rhs)
	{
		Item tmpBuff;
		iDevice<Item>::rewind();
		iDevice<Item>::seek(rhs,SEEK_CUR);
		iDevice<Item>::read(tmpBuff,1);
		return tmpBuff[0];
	}
};


#endif
