
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

using namespace std;

/*
	stream flags in octal - base eight
*/
#define ODD_RDONLY 	00
#define ODD_WRONLY 	01
#define ODD_RDWR 	02

#define EXCLUSION Sentry exclusion(this); exclusion.touch();

class Device;

class Monitor {
  // ...
};

class Condition {
public:
  Condition( Monitor* m ) {}
  // ...
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

vector<Inode> ilist;
vector<Device*> drivers;
// used to maintain a record of reusable file descriptors
// deviceNumbers
vector<int> freedDeviceNumbers;

class Device: public Monitor {
	public:
		Condition ok2read;
		Condition ok2write;
		Inode *inode;
		bool readable;
		bool writeable;
		int deviceNumber;
		string driverName;

	// this constructor originally had an initialization of
	// deviceNumber(drivers.size()) which is not safe. the
	// case where drivers is resized (file descriptors removed)
	// will cause at least two device drivers to have identical
	// device numbers (indices of vector drivers).
	// solution is to scan vector drivers for next available
	// index (file descriptor).
	Device(string driverName)
		:Monitor(),ok2read(this),
		ok2write(this),
		driverName( driverName )
		{
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
	
	virtual int open() {}
	virtual int close() {}
	virtual int read() {}
	virtual int write() {}
	virtual int seek() {}
	virtual int rewind() {}
	virtual int ioctl() {}
	virtual void online() {}
	virtual void offline() {}
	virtual void fireup() {}
	virtual void suspend() {}
	virtual void shutdown() {}
	virtual void initialize() {}
	virtual void finalize() {}

};


class streamDevice : Device {

	public: 
		int inodeCount = 0;
		int openCount = 0;
		
		iostream * bytes;
		mode_t mode;
		size_t offset = 0;

		streamDevice( iostream *io )
			: bytes(io), Device("streamDevice")
		{
			cout << "*Inside streamDevice()\n";
			readable = false;
			writeable = false;
			cout << "readable: " << readable << endl;
			cout << "writeable: " << writeable << endl;
		}

		~streamDevice() {
			// dunno what to do here
		}

		int open(const char* pathname, int flags) {
			readable = !(flags & 0x01);
			writeable = (flags & 0x01) | (flags & 0x02);
			driverName = pathname;
			openCount++;
			return deviceNumber; // return fd to device driver
		}

		// return non-zero on error
		int close() {
			//openCount = (openCount > 0) ? openCount - 1 : 0;
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

		int read(void* buf, size_t count) {
			char* buffer = (char*)buf;

			int i = 0;
			for(i = 0; i < count; i++)
			{
				//return if eof encountered with amount written
				if(bytes->peek() == EOF)
				{
					bytes->seekg(offset);
					bytes->seekp(offset);
					bytes->clear(); //clear eofbit after reading eof
					return i;
				}
				int tmp = bytes->tellg();
				((char*)buf)[i] = bytes->get();

				//read character from bytes then store into buffer
				cout << "Reading '" << (char)buffer[i] << "' from position tellg() " << tmp
					<< " and position offset " << offset << " of device '" << this->driverName << "'" << endl;
				offset++; 						
			}
			//return with amount read
			bytes->seekg(offset);
			bytes->seekp(offset);
			return i;		
		}

		int write(void const* buf, size_t count) {
			char* buffer = (char*)buf;

			int i = 0;
			for(i = 0; i < count; i++)
			{
				//write character from buffer into bytes
				cout << "Writing '" << (char)buffer[i] << "' at position " << bytes->tellp()
					<< " and position offset " << offset << " of device '" << this->driverName << "'" << endl;
				bytes->put(buffer[i]);
				offset++;
			}
			//return with amount written
			bytes->seekg(offset);
			bytes->seekp(offset);
			return i;
		}

		int seek(off_t offsetIn, int whence) {
			//set position to passed in position
			if(whence == SEEK_SET)
			{
				//get end position to save in our offset variable
				off_t save = bytes->tellp();
				bytes->seekp(0, ios_base::end);
				off_t end = bytes->tellp();
				bytes->seekp(save);

				//save position in our offset variable
				offset = offsetIn;			
				
				if(offset >  end)
				{
					doPadding(save,offset);
				}


				//set positions
				bytes->seekp(offsetIn,ios_base::beg);//move put pointer - write
				bytes->seekg(offsetIn,ios_base::beg);//move get pointer - read
			}

			//set position to current position + passed in position
			else if(whence == SEEK_CUR)
			{
				//get end position to save in our offset variable
				off_t save = bytes->tellp();
				bytes->seekp(0, ios_base::end);
				off_t end = bytes->tellp();
				bytes->seekp(save);

				//save position in our offset variable
				offset += offsetIn;			
				
				if(offset >  end)
				{
					doPadding(save,offset);
				}
				
				//set positions 
				bytes->seekp(offset, ios_base::beg);
				bytes->seekg(offset, ios_base::beg);

				cout << "SEEK read position " << bytes->tellg() << endl;
				cout << "SEEK write position " << bytes->tellp() << endl;
			}
	
			else if(whence == SEEK_END)
			{
				///*			

				//get end position to save in our offset variable
				off_t save = bytes->tellp();
				bytes->seekp(0, ios_base::end);
				off_t end = bytes->tellp();
				bytes->seekp(save);

				//save position in our offset variable
				offset = end + offsetIn;

				if(offset > end)
				{
					doPadding(save,offset);
				}

				//set positions
				bytes->seekp(offset,ios_base::beg);
				bytes->seekg(offset,ios_base::beg);
				//*/
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
				cout << "Padding: " << bytes->tellp() << endl;
				bytes->put('\0');
			}
		}

	 	void online() {return;}

	 	void offline() {return;}

	 	void fireup() {return;}

		void suspend() {return;}

		void shutdown() {return;}

		void initialize() {return;}

		void finalize() {return;}

		static void read_occured() {return;}

		static void write_occured() {return;}

		int ioctl() {return 0;}


};

//Brian's code
class stringstreamDevice : Device {
	public:
	int inodeCount = 0;
	int openCount = 0;
	stringstream* bytes;
	stringstreamDevice( stringstream* ss )
	: bytes(ss), Device("stringstreamDevice")
	{
		readable = true;
		writeable = true;
	}
	~stringstreamDevice() {
	}
	int open( const char* pathname, int flags) {
		//Unable to get rid of the error for specifying the inode kind.
		//inode->kind = device;
		//inode->driver = this;
		if( flags == O_RDONLY )
		{
			readable = true;
			writeable = false;
		}
		else if( flags == O_WRONLY )
		{
			readable = false;
			writeable = true;
		}
		else if( flags == O_RDWR )
		{
			readable = true;
			writeable = true;
		}
		++inodeCount;
		++openCount;
		return deviceNumber;
	}
	int close( int fd) {
		cout << "Closing device " << driverName << endl;
		--openCount;
		cout << "Device " << driverName << " closed\n";
		return 0;
	}
	int read( int fd, void* buf, size_t count) {
		Device* ssd = drivers[fd];
		cerr << "Beginning read from device " << ssd->driverName << endl;
		char* s = (char*) buf;
		int i = 0;
		for( ; i < count; i++ )	
		{
			*(s+i) = bytes->get();
		}
		return i;
	}
	int write( int fd, void* buf, size_t count) {
		Device* ssd = drivers[fd];
		cerr << "Beginning write to device " << ssd->driverName << endl;
		char* s = (char*) buf;
		int i = 0;
		for( ; i < count; i++ )	
		{
			bytes->put(*s);
			s++;
		}
		return i;
	}
	int seek( int fd, off_t offset, int whence) {
		if( whence == SEEK_SET )
		{
			bytes->seekg(offset,ios_base::beg);
			bytes->seekp(offset,ios_base::beg);
		}
		else if( whence == SEEK_CUR )
		{
			bytes->seekg(offset,ios_base::cur);
			bytes->seekp(offset,ios_base::cur);
		}
		else if( whence == SEEK_END )
		{
			bytes->seekg(offset,ios_base::end);
			bytes->seekp(offset,ios_base::end);
		}
		return offset;
	}
	int rewind( int pos ) {
		seek( deviceNumber, 0, SEEK_CUR);
	}
	/*
	int ioctl( ) {
	}
	//*/
};


void myPrint(char* buf, int size)
{
	cout << "myPrint: ";
	for(int i = 0; i < size; i++)
	{
		cout << buf[i];
	}
	cout << endl;
}


//tests
int main() {

	cout << "Hello world!\n\n";

	char buf[50];
	char buf2[] = "hello there my friend";//len 21
	char buf3[1024];
	string str;

	memset(buf,0,50);
	memset(buf3,0,1024);

	//create i device
	strstreambuf sb(buf,50,buf);
	iostream stream(&sb);

	//create ios device     
 	stringstream SSstream;   
	streamDevice ios = streamDevice(&SSstream);
	ios.open("iostreamDevice1", O_RDWR);

	/*
	streamDevice is1 = streamDevice(&stream);
	cout << "Read yes --\n";
	int dn1 = is1.open("~/Desktop/tes1.txt",ODD_RDONLY);
	cout << "deviceNumber: " << dn1 << endl << endl;

	streamDevice is2 = streamDevice(&stream);
	cout << "Write yes --\n";
	int dn2 = is2.open("~/Desktop/test2.txt",ODD_WRONLY);
	cout << "deviceNumber: " << dn2 << endl << endl;
	
	streamDevice is3 = streamDevice(&stream);
	cout << "Read+Write yes --\n";
	int dn3 = is3.open("~/Desktop/test3.txt",ODD_RDWR);
	cout << "deviceNumber: " << dn3 << endl << endl;

	cout << "Closing deviceNumber: " << dn2 << endl << endl;
	is2.close();

	streamDevice is4 = streamDevice(&stream);
	cout << "Write yes --\n";
	int dn4 = is4.open("~/Desktop/test4.txt",ODD_WRONLY);
	cout << "deviceNumber: " << dn4 << endl << endl;

	cout << "\nBye cruel world!\n";
  //*/            


	/*//module 1 test read and write
	cout << "amount write: " << ios.write(buf2,50) << endl;
	cout << "wrote this: ";
	myPrint(buf2,50);
	cout << "====================" << endl;

	ios.seek(0,SEEK_SET);
	cout << "amount read: " << ios.read(buf,50) << endl;
	cout << "read this: ";
	myPrint(buf,50);
	cout << "====================" << endl;
	//*/

	/*//module 2 test read and write
	cout << "amount write: " << ios.write(buf2,25) << endl;
	cout << "wrote this: ";
	myPrint(buf2,25);
	cout << "====================" << endl;

	cout << "amount write: " << ios.write(buf2,25) << endl;
	cout << "wrote this: ";
	myPrint(buf2,25);
	cout << "====================" << endl;

	ios.seek(0,SEEK_SET);
	cout << "amount read: " << ios.read(buf,50) << endl;
	cout << "read this: ";
	myPrint(buf,100);
	cout << "====================" << endl;
	//*/

	/*//module 3 test seeking 
	cout << "amount write: " << ios.write(buf2,25) << endl;
	cout << "wrote this: ";
	myPrint(buf2,25);
	cout << "====================" << endl;

	cout << "seeking to : " << ios.seek(5,SEEK_SET) << endl;
	cout << "====================" << endl;

	cout << "amount write: " << ios.write(buf2,25) << endl;
	cout << "wrote this: ";
	myPrint(buf2,25);
	cout << "====================" << endl;

	ios.seek(0,SEEK_SET);
	cout << "amount read: " << ios.read(buf,50) << endl;
	cout << "read this: ";
	myPrint(buf,50);
	cout << "====================" << endl;
	//*/




	/* //module 4 std input
	cout << "Input some string to write to device (CTR-D to stop): ";
 	while (getline(cin, str)) // Reads line into str
	{ 
		//write str into stream
		ios.write(str.c_str(), str.length());
 	}

	//read 100 characters from ios device into buf3, then print buf3
	ios.seek(0,SEEK_SET);
	ios.read(buf3,100);
	myPrint(buf3,100);
   //*/

	///* //test SEEK_END kind of working
	//write buf2 to ios device twice

	//ios.seek(0,SEEK_SET);
	//ios.write(buf2,25);
	//ios.seek(5,SEEK_END);
	//ios.write(buf2,25);

	//seek 5 from beginning then seek 5 from current position
	ios.seek(5,SEEK_SET);
	ios.seek(5,SEEK_CUR);
	ios.write(buf2,25);
	
	//read 100 characters from ios device into buf3, then print buf3
	ios.seek(0,SEEK_SET);
	ios.read(buf3,100);
	myPrint(buf3,100);
	//*/	


	/*segfault 

	// Stringstream test harness
	stringstream* ss = new stringstream;
	stringstreamDevice* ssDevice = new stringstreamDevice(ss);
	int ssDeviceFd = -1;
	char writeBuf[14] = "Hello, world!";
	char* readBuf = new char[14];

	// Open a file to test our input and output.
	ssDeviceFd = ssDevice->open("writeTest.txt", 2);  //segfault here because of (inode->driver = this) in open function

	assert( ssDeviceFd != -1 );
	cerr << "Write test stream opened, fd = " << ssDeviceFd << "\n";
	
	// Read the following data into a Device.
	ssDevice->write(ssDeviceFd, writeBuf, 14);
	cerr << "Buffer contents written to write-test stream\n"
		 << "Bytes: " << (ssDevice->bytes)->str() << endl;

	// Close the file when finished.
	ssDeviceFd =  (!ssDevice->close(ssDeviceFd))? -1 : ssDeviceFd;
	assert( ssDeviceFd == -1 );
	cout << "Write test stream closed\n";
	
	// If the file was successfully written to, we can use it to test read.
	ssDeviceFd = ssDevice->open("writeTest.txt", 2);
	*ss << "Hello, world!";
	ssDevice->write(ssDeviceFd, writeBuf, 14);
	cout << "Stream contents written to read-test stream\n";
	ssDevice->read(ssDeviceFd, readBuf, 14);
	cout << "Contents stored in buffer: " << readBuf << endl;
	
	//*/

	/*   experiment with select()
    fd_set rfds;
    struct timeval tv;
    int retval;

    // Watch stdin (fd 0) to see when it has input. 
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    // Wait up to five seconds. 
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    retval = select(1, &rfds, NULL, NULL, &tv);
    // Don’t rely on the value of tv now! 

    if (retval == -1)
        perror("select()");
    else if (retval)
        printf("Data is available now.\n");
        // FD_ISSET(0, &rfds) will be true. 
    else
        printf("No data within five seconds.\n");
    return 0;
  	//*/
	
 	return 0;
}
