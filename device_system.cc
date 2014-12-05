#include<string>
#include<iostream>
#include<sstream>
#include<stdio.h>
#include<mutex>
#include<condition_variable>
#include<limits>
#include<climits>
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
#include <strstream>
#include <cstring>
#include <cstdio>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sstream>
#include <errno.h>
#include <assert.h>


//Interrupts and IOCTL header
#ifdef __linux
#include <linux/ioctl.h>
#elif __APPLE__
#include <sys/ioctl.h>
#endif

using namespace std;

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


class Device;


//=========================== interrupts ======================


class InterruptSystem {
public:       // man sigsetops for details on signal operations.
  //static void handler(int sig);
  // exported pseudo constants
  static sigset_t on;                    
  static sigset_t alrmoff;    
  static sigset_t alloff;     
  InterruptSystem() {  
    //signal( SIGALRM, InterruptSystem::handler ); 
    sigemptyset( &on );                    // none gets blocked.
    sigfillset( &alloff );                  // all gets blocked.
    sigdelset( &alloff, SIGINT );
    sigemptyset( &alrmoff );      
    sigaddset( &alrmoff, SIGALRM ); //only SIGALRM gets blocked.
    set( alloff );        // the set() service is defined below.
    struct itimerval time;
    time.it_interval.tv_sec  = 0;
    time.it_interval.tv_usec = 400000;
    time.it_value.tv_sec     = 0;
    time.it_value.tv_usec    = 400000;
    cerr << "\nstarting timer\n";
    setitimer(ITIMER_REAL, &time, NULL);
  }
  sigset_t set( sigset_t mask ) {
    sigset_t oldstatus;
    pthread_sigmask( SIG_SETMASK, &mask, &oldstatus );
    return oldstatus;
  } // sets signal/interrupt blockage and returns former status.
  sigset_t block( sigset_t mask ) {
    sigset_t oldstatus;
    pthread_sigmask( SIG_BLOCK, &mask, &oldstatus );
    return oldstatus;
  } //like set but leaves blocked those signals already blocked.
};

// signal mask pseudo constants
sigset_t InterruptSystem::on;   
sigset_t InterruptSystem::alrmoff; 
sigset_t InterruptSystem::alloff;  

InterruptSystem interrupts;               // singleton instance.


//============================================================


class Monitor : public mutex {
public:
  sigset_t mask;
public:
  Monitor( sigset_t mask = InterruptSystem::alrmoff )   
    : mask(mask) 
  {}
};


class Sentry {  // To automatically block and restore interrupts.
  const sigset_t old;     // Place to store old interrupt status.
public:
  Sentry( Monitor* m ) 
    : old( interrupts.block(m->mask) )                 
  {}
  ~Sentry() {
    interrupts.set( old );                              
  }
};


// Translation of my coordination primitives to C++14's.  The only
// thing that C++14 lacks is prioritized waiting, which is very 
// important for scheduling.

// The following version of EXCLUSION works correctly because C++ 
// invokes destructors in the opposite order of the constructors.
#define EXCLUSION Sentry snt_(this); unique_lock<Monitor::mutex> lck(*this);
#define CONDITION condition_variable
#define WAIT      wait(lck)
#define SIGNAL    notify_one()


class Semaphore : public Monitor {  // signal-safe semaphore
// The common C++ version of semaphore is not async-signal safe 
// in the sense defined by POSIX.
private:
  CONDITION available;
  int count;
public:
  Semaphore( int count = 0 )
    : count(count)
  {}
  void release() {
    EXCLUSION
    ++count;
    available.SIGNAL;
  }
  void acquire() {
    EXCLUSION
    while(count == 0) available.WAIT;
    --count;
  }
};



class Inode : Monitor {
public: 
	int linkCount = 0;
	int openCount = 0;
	bool readable = false;
	bool writable = false;
	enum Kind { regular, directory, symlink, pipe, socket, device } kind;

	time_t ctime, mtime, atime;

	~Inode() {
	}

	int unlink() {
		assert(linkCount > 0);
		--linkCount;
		cleanup();
		return 0;
	}
	
	void cleanup() {
		if (! openCount && !linkCount ) {
			//throwaway stuff
		}
	}
	Device* driver;
	string* bytes;
};

//==================================================================

vector<Inode> ilist;
vector<Device*> drivers;
// used to maintain a record of reusable file descriptors
// deviceNumbers
vector<int> freedDeviceNumbers;



// Here is the section about interrupt handlers
//iDevice<char> stuff(cin);

void readCompletionHandler(int deviceIndex);
void writeCompletionHandler(int deviceIndex);


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

	// this constructor originally had an initialization of
	// deviceNumber(drivers.size()) which is not safe. the
	// case where drivers is resized (file descriptors removed)
	// will cause at least two device drivers to have identical
	// device numbers (indices of vector drivers).
	// solution is to scan vector drivers for next available
	// index (file descriptor).
	Device(string dn)
		:driverName( dn )
		{
			cout << "*Inside Device()\n";
			readable = false;
			writeable = false;
			cout << "readable: " << readable << endl;
			cout << "writeable: " << writeable << endl;
			// check to see if we may re-use a file descriptor
			if (freedDeviceNumbers.size() > 0)
			{
				deviceNumber = freedDeviceNumbers.front();
				freedDeviceNumbers.erase(freedDeviceNumbers.begin());
			}
			else
			{
				deviceNumber = drivers.size();
			}
			drivers.push_back(this);
			//inode->openCount++;
		}

	~Device() {
		//inode->openCount--;

		// we must remove device driver from vector
		// look into remove_if
		auto i = begin(drivers);
		while (i != end(drivers)) {
		    if ((*i)->deviceNumber == deviceNumber) {
		        i = drivers.erase(i);
		    }
		    else {
		        i++;
		    }
		}
	}
	
	virtual int open(const char* pathname, int flags) {
		auto i = begin(drivers);
		while (i != end(drivers)) {
		    if ((*i)->driverName == pathname) {
		        return -1;
		    }
		    i++;
		}
		readable = !(flags & 0x01);
		writeable = (flags & 0x01) | (flags & 0x02);
		driverName = pathname;
		openCount++;
		return deviceNumber; // return fd to device driver
	}

	// return non-zero on error
	virtual int close() {
		openCount = (openCount > 0) ? openCount - 1 : 0;
		if (openCount==0)
		{
			// remove device from vector of drivers
			auto i = begin(drivers);
			while (i != end(drivers)) {
			    if ((*i)->deviceNumber == this->deviceNumber) {
			        // push current deviceNumber onto available
					freedDeviceNumbers.push_back((*i)->deviceNumber);
			        i = drivers.erase(i);
			        return 0;
			    }
			    else {
			        i++;
			    }
			}
			return 1;
		}
		return 0;
	}
	
	//virtual int open() {}
	//virtual int close() {}
	virtual int read() {}
	virtual int write() {}
	virtual int seek() {}
	virtual int rewind() {}
	//virtual int ioctl() {}
	/*
		Commands:
			1. HARDRESET - reset open count to one for device. used to close device if error.
			2. FIONREAD - get # bytes to read
 			3. FIONBIO - set/clear non-blocking i/o
			4. FIOASYNC - set/clear async i/o
			5. FIOSETOWN - set owner. ?
			6. FIOGETOWN - get owner. ?
			7. TIOCSTOP - stop output
			8. TIOCSTART - start output
			9. FIONWRITE - bytes written
			...
			n. ???
	*/
	virtual int ioctl(unsigned int cmd, unsigned long arg)
	{
		//defined in sys/ioctl.h
		//_IO(1,2);//io macro
		switch(cmd)
		{
			case ODD_HARDRESET:
			openCount = 1;
			break;
			case ODD_FIONBIO:
			if (readable)
			{
				readCompletionHandler(deviceNumber);
			}
			if (writeable)
			{
				writeCompletionHandler(deviceNumber);
			}
			break;
			case ODD_FIONREAD:
			return bytesRead;
			break;
			case ODD_FIONWRITE:
			return bytesWritten;
			break;
			default: break;
		}
		return -1;
	}
	virtual void online() {}
	virtual void offline() {}
	virtual void fireup() {
		cout << "firing up  " << driverName << endl;
	}
	virtual void suspend() {
		cout << "suspending  " << driverName << endl;
	}
	virtual void shutdown() {
		cout << "shuting down " << driverName << endl;
	}
	virtual void initialize() {
		cout << "initializing down " << driverName << endl;
	}
	virtual void finalize() {
		cout << "finalizing " << driverName << endl;
	}
	
	virtual void completeWrite() {}
	virtual void completeRead() {}


};






//==================================================================







template< typename Item >
class iDevice : public Device {

  istream *stream;
  size_t offsetIn = 0;
  
public:
	iDevice( istream *stream )
		: stream(stream), 
		Device("iDevice")
	{}

	CONDITION ok2read;
	bool readCompleted;

	int input( Item* buffer, int n ) {
		/*EXCLUSION
		int i = 0;
		for ( ; i != n; ++i ) {
			if ( !stream ) break;
			stream >> buffer[i];                        // read a byte
			readCompleted = false;
			readCompleted = true;  // This line is for testing purposes
			// only.  Delete it when interrupts are working.
			while( ! readCompleted ) ok2read.WAIT;
		}
		return i;*/
		read(buffer,n);
	}
  
  		int read(Item* buffer, size_t count) {
			EXCLUSION
			int i = 0;
			for(i = 0; i < count; i++)
			{
				//return if eof encountered with amount read
				if(!stream)
				{
					stream->seekg(offsetIn);
					stream->clear(); //clear eofbit after reading eof
					return (bytesRead = i);
				}
				int tmp = stream->tellg();
				buffer[i] = stream->get();

				//read character from bytes then store into buffer
				//cout << "Reading '" << (char)buffer[i] << "' from position tellg() " << tmp
				//	<< " and position offset " << offsetIn << " of device '" << this->driverName << "'" << endl;
				offsetIn++; 
				readCompleted = false;
      			readCompleted = true;
      			while( ! readCompleted ) ok2read.WAIT;						
			}
			//return with amount read
			stream->seekg(offsetIn);
			//bytesRead = i
			return (bytesRead = i);		
		}
		
		int seek(off_t newOffset, int whence) {
			EXCLUSION
			//set position to passed in position
			if(whence == SEEK_SET)
			{
				//get end position to save in our offset variable
				off_t save = stream->tellg();
				stream->seekg(0, ios_base::end);
				off_t end = stream->tellg();
				stream->seekg(save);

				//save position in our offset variable
				offsetIn = newOffset;			
				
				if(offsetIn >  end)
				{
					offsetIn = end;
				}


				//set positions
				stream->seekg(offsetIn,ios_base::beg);//move get pointer - read
			}

			//set position to current position + passed in position
			else if(whence == SEEK_CUR)
			{
				//get end position to save in our offset variable
				off_t save = stream->tellg();
				stream->seekg(0, ios_base::end);
				off_t end = stream->tellg();
				stream->seekg(save);

				//save position in our offset variable
				offsetIn += newOffset;			
				
				if(offsetIn >  end)
				{
					offsetIn = end;
				}
				
				//set positions 
				stream->seekg(offsetIn, ios_base::beg);
			}
	
			else if(whence == SEEK_END)
			{
				///*			

				//get end position to save in our offset variable
				off_t save = stream->tellg();
				stream->seekg(0, ios_base::end);
				off_t end = stream->tellg();
				stream->seekg(save);

				//save position in our offset variable
				offsetIn = end + newOffset;

				if(offsetIn > end)
				{
					offsetIn = end;
				}

				//set positions
				stream->seekg(offsetIn,ios_base::beg);
				//
			} 
			return offsetIn;
			
		}
		

		//rewind is a modification of seek
		//reset file ptr position to beginning (position 0). SEEK_SET
		int rewind(int pos) {
			seek(0,SEEK_SET);
			return 0;
		}	

	void completeRead() {
		EXCLUSION
		cout <<"readComplete" << endl;
		readCompleted = true;
		ok2read.SIGNAL;
	}

};


template< typename Item >
class oDevice : public Device {

  	ostream *stream;
    size_t offsetOut = 0;

public:		
	oDevice( ostream *stream )
		: stream(stream), Device("oDevice")
	{}

	CONDITION ok2write;
	bool writeCompleted;

	int output( Item* buffer, int n ) {
		/*EXCLUSION
		int i = 0;
		for ( ; i != n; ++i ) {
			if ( !stream ) break;
			stream << buffer[i];                       // write a byte
			writeCompleted = false;
			writeCompleted = true;  // This line is for testing purposes
			// only.  Delete it when interrupts are working.
			while( ! writeCompleted ) ok2write.WAIT;
		}
		return (bytesWritten=i);*/
		write(buffer,n);
	}
  
  		int write(Item* buffer, size_t count) {
  		    EXCLUSION
			int i = 0;
			for(i = 0; i < count; i++)
			{
				//write character from buffer into bytes
				//cout << "Writing '" << (char)buffer[i] << "' at position " << stream->tellp()
				//	<< " and position offset " << offsetOut << " of device '" << this->driverName << "'" << endl;
				if ( !stream ) break;
				writeCompleted = false;
      			writeCompleted = true;
				stream->put(buffer[i]);
				offsetOut++;
				while( ! writeCompleted ) ok2write.WAIT;
			}
			//return with amount written
			stream->seekp(offsetOut);
			return (bytesWritten=i);
		}
		

		int seek(off_t newOffset, int whence) {
			EXCLUSION
			//set position to passed in position
			if(whence == SEEK_SET)
			{
				//get end position to save in our offset variable
				off_t save = stream->tellp();
				stream->seekp(0, ios_base::end);
				off_t end = stream->tellp();
				stream->seekp(save);

				//save position in our offset variable
				offsetOut = newOffset;			
				
				if(offsetOut >  end)
				{
					doPadding(save,offsetOut);
				}


				//set positions
				stream->seekp(offsetOut,ios_base::beg);//move put pointer - write
			}

			//set position to current position + passed in position
			else if(whence == SEEK_CUR)
			{
				//get end position to save in our offset variable
				off_t save = stream->tellp();
				stream->seekp(0, ios_base::end);
				off_t end = stream->tellp();
				stream->seekp(save);

				//save position in our offset variable
				offsetOut += newOffset;			
				
				if(offsetOut >  end)
				{
					doPadding(save,offsetOut);
				}
				
				//set positions 
				stream->seekp(offsetOut, ios_base::beg);
				
				cout << "SEEK write position " << stream->tellp() << endl;
			}
	
			else if(whence == SEEK_END)
			{
				///*			

				//get end position to save in our offset variable
				off_t save = stream->tellp();
				stream->seekp(0, ios_base::end);
				off_t end = stream->tellp();
				stream->seekp(save);

				//save position in our offset variable
				offsetOut = end + newOffset;

				if(offsetOut > end)
				{
					doPadding(save,offsetOut);
				}

				//set positions
				stream->seekp(offsetOut,ios_base::beg);
				//
			} 
			return offsetOut;
			
		}
		

		//rewind is a modification of seek
		//reset file ptr position to beginning (position 0). SEEK_SET
		int rewind(int pos) {
			seek(0,SEEK_SET);
			return 0;
		}
		
		//helper function doPadding only works for write, not read
		void doPadding(off_t start, off_t end)
		{
			for(int i = start; i < end; i++ )	
			{
				cout << "Padding: " << stream->tellp() << endl;
				stream->put('\0');
			}
		}


	void completeWrite() {
		EXCLUSION
		cout <<"writeComplete" << endl;
		writeCompleted = true;
		ok2write.SIGNAL;
	}

};



template< typename Item >
class ioDevice : public Device{

  iostream *stream;
  size_t offset = 0;

public:

	ioDevice( iostream *s )  
		:stream(s),
		 Device("ioDevice")
		 
	{}
	
	CONDITION ok2read;
	bool readCompleted;

	int input( Item* buffer, int n ) {
		read(buffer,n);
		/*EXCLUSION
		int i = 0;
		for ( ; i != n; ++i ) {
			if ( !stream ) break;
			stream >> buffer[i]; // read a byte
			readCompleted = false;
			readCompleted = true;  // This line is for testing purposes
			// only.  Delete it when interrupts are working.
			while( ! readCompleted ) ok2read.WAIT;
		}
		return i;*/
	}
  
	int read(Item* buffer, size_t count) {
		EXCLUSION
		int i = 0;
		for(i = 0; i < count; i++)
		{
			//return if eof encountered with amount read
			cout << "i " << i << endl;
			if(!stream)
			{
				stream->seekg(offset);
				stream->clear(); //clear eofbit after reading eof
				return (bytesRead=i);
			}
			int tmp = stream->tellg();
			buffer[i] = stream->get();

			//read character from bytes then store into buffer
			cout << "Reading '" << buffer[i] << "' from position tellg() " << tmp
				<< " and position offset " << offset << " of device '" << this->driverName << "'" << endl;
			offset++; 
			readCompleted = false;
  			readCompleted = true;
  			while( ! readCompleted ) ok2read.WAIT;
		}
		//return with amount read
		stream->seekg(offset);
		stream->seekp(offset);
		return (bytesRead=i);
	}

		

	void completeRead() {
		EXCLUSION
		cout <<"readComplete" << endl;
		readCompleted = true;
		ok2read.SIGNAL;
	}
	
	
	CONDITION ok2write;
	bool writeCompleted;

	int output( Item* buffer, int n ) {
		write(buffer,n);
		/*EXCLUSION
		int i = 0;
		for ( ; i != n; ++i ) {
			if ( !stream ) break;
			stream << buffer[i];                       // write a byte
			writeCompleted = false;
			writeCompleted = true;  // This line is for testing purposes
			// only.  Delete it when interrupts are working.
			while( ! writeCompleted ) ok2write.WAIT;
		}
		return i;*/
	}
  
	int write(Item* buffer, size_t count) {
	    EXCLUSION
		int i = 0;
		for(i = 0; i < count; i++)
		{
			//cout << "!sizeof " << sizeof(buffer) * sizeof(Item) << endl;
			if (!stream)
			{
				cout << "!sizeof " << sizeof(buffer) * sizeof(Item) << endl;
				stream->seekp(0,ios_base::end);
				stream->seekg(0,ios_base::end);
				stream->clear(); //clear eofbit after reading eof
				return (bytesWritten=i);
			}
			//write character from buffer into bytes
			cout << "Writing '" << buffer[i] << "' at position " << stream->tellp()
				<< " and position offset " << offset << " of device '" << this->driverName << "'" << endl;
			writeCompleted = false;
  			writeCompleted = true;
			stream->put(buffer[i]);
			offset++;
			while( ! writeCompleted ) ok2write.WAIT;
		}
		//return with amount written
		stream->seekp(offset);
		stream->seekg(offset);
		return (bytesWritten=i);
	}
	
	void completeWrite() {
		EXCLUSION
		cout <<"writeComplete" << endl;
		writeCompleted = true;
		ok2write.SIGNAL;
	}
	
	int seek(off_t newOffset, int whence) {
		EXCLUSION
		//set position to passed in position
		if(whence == SEEK_SET)
		{
			cout << "0SEEK read position " << stream->tellg() << endl;
			cout << "0SEEK write position " << stream->tellp() << endl;

			//get end position to save in our offset variable
			off_t save = stream->tellp();
			stream->seekp(0, ios_base::end);
			off_t end = stream->tellp();
			stream->seekp(save);

			//save position in our offset variable
			offset = newOffset;
			
			if(offset > (end-1))
			{
				cout << "doPadding\n";
				doPadding(save,offset);
			}

			//set positions
			stream->seekp(offset,ios_base::beg);//move put pointer - write
			stream->seekg(offset,ios_base::beg);//move get pointer - read
			
			cout << "1SEEK read position " << stream->tellg() << endl;
			cout << "1SEEK write position " << stream->tellp() << endl;
		}

		//set position to current position + passed in position
		else if(whence == SEEK_CUR)
		{
			//get end position to save in our offset variable
			off_t save = stream->tellp();
			stream->seekp(0, ios_base::end);
			off_t end = stream->tellp();
			stream->seekp(save);

			//save position in our offset variable
			offset += newOffset;			
			
			if(offset >  end)
			{
				doPadding(save,offset);
			}
			
			//set positions 
			stream->seekp(offset, ios_base::beg);
			stream->seekg(offset, ios_base::beg);

			cout << "SEEK read position " << stream->tellg() << endl;
			cout << "SEEK write position " << stream->tellp() << endl;
		}

		else if(whence == SEEK_END)
		{
			///*			

			//get end position to save in our offset variable
			off_t save = stream->tellp();
			stream->seekp(0, ios_base::end);
			off_t end = stream->tellp();
			stream->seekp(save);

			//save position in our offset variable
			offset = end + newOffset;

			if(offset > end)
			{
				doPadding(save,offset);
			}

			//set positions
			stream->seekp(offset,ios_base::beg);
			stream->seekg(offset,ios_base::beg);
			//*/
			
			cout << "SEEK read position " << stream->tellg() << endl;
			cout << "SEEK write position " << stream->tellp() << endl;
		} 
		return offset;
		
	}	

	//rewind is a modification of seek
	//reset file ptr position to beginning (position 0). SEEK_SET
	int rewind(int pos) {
		seek(0,SEEK_SET);
		return 0;
	}

	//helper function doPadding only works for write, not read
	void doPadding(off_t start, off_t end)
	{
		for(int i = start; i < end; i++ )	
		{
			cout << "Padding: " << stream->tellp() << endl;
			stream->put('\0');
		}
	}

};




void readCompletionHandler(int deviceIndex) {  // When IO-completion interrupts
  // are available, this handler should be installed to be directly 
  // invoked by the hardawre's interrupt mechanism.
  drivers[deviceIndex]->completeRead();
}

void writeCompletionHandler(int deviceIndex) {  // When IO-completion interrupts
  // are available, this handler should be installed to be directly 
  // invoked by the hardawre's interrupt mechanism.
  drivers[deviceIndex]->completeWrite();
}


//==================================================================


///*

		/*
		Commands:
			1. HARDRESET - reset open count to one for device. used to close device if error.
			2. FIONREAD - get # bytes to read
 			3. FIONBIO - set/clear non-blocking i/o
			4. FIOASYNC - set/clear async i/o
			5. FIOSETOWN - set owner. ?
			6. FIOGETOWN - get owner. ?
			7. TIOCSTOP - stop output
			8. TIOCSTART - start output
			...
			n. ???
		//
		int ioctl(unsigned int cmd, unsigned long arg)
		{
			//defined in sys/ioctl.h
			//_IO(1,2);//io macro
			switch(cmd)
			{
				default: break;
			}
			return -1;
		}
};

*/

void myPrint(char* buf, int size)
{
	cout << "myPrint: ";
	for(int i = 0; i < size; i++)
	{
		cout << buf[i];
	}
	cout << "." << endl;
}


//tests
int main() {

	char buffer[50];
	char bufferIn[50];
	char bufferOut[50];

	char buf1[] = "hello test this";//15
	char buf2[20];
	
	cout << "tellg of cin: " << cin.tellg() << endl;
	cout << "tellp of cout: " << cout.tellp() << endl;
	
	/*
	iDevice<char> in(cin);
	oDevice<char> out(cout);

	cout << "reading" << endl;
	cout << "read: " << in.read(buffer,50) << endl;
	//in.input(buffer,50);
	
	cout << "writing" << endl;
	cout << "write: " << out.write(buffer,50) << endl;
	//out.output(buffer,50);
	//*/
	cout << "\n=========================================" << endl;
	///*
	char bufstr[50];
	strstreambuf sb2(bufstr,50,bufstr);
	iostream s2(&sb2);
	stringstream ss;
	ss << "hello this is test";
	ioDevice<char> inOut(&ss);
	//*/
	
	cout << "whatwhatwhatwhatwhatwhatwhatwhatwhat" << endl;
	
	//string temp = "";
	//stringstream ss(temp);
	//ioDevice<char> inOut(ss);
	
	inOut.seek(0,SEEK_SET);
	
	// Works with -> input: << , output: >>
	cout <<"input: "<< inOut.read(buf1,15) << endl;//Must do Bounds checking!
	//out <<"output: "<< inOut.write(buf2,15) << endl;
	
	myPrint(buf2,20);
	
	inOut.seek(0,SEEK_SET);
	cout << "pleasepleasepleasepleasepleasepleasepleasepleaseplease" << endl;

	// Works with -> input: >> , output: >>
	myPrint(buf1,15);
	cout <<"write: "<< inOut.write(buf2,15) << endl;
	cout <<"read: "<< inOut.read(buf1,15) << endl;//??
	
	myPrint(buf2,20);

	
 	return 0;
}
