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

#define EXCLUSION Sentry exclusion(this); exclusion.touch();

class DeviceDriver;

class Monitor {
  // ...
};

class Condition {
public:
  Condition( Monitor* m ) {}
  // ...
};

//========================== Inode =================================

class Inode: Monitor {
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
		assert( linkCount > 0);
		--linkCount;
		cleanup();	
	}
	
	void cleanup() {
		if (! openCount && !linkCount ) {
			//throwaway stuff
		}
	}
	DeviceDriver* driver;
	string* bytes;
};

vector<Inode> ilist;
vector<DeviceDriver*> drivers;

//========================== DeviceDriver =================================

class DeviceDriver: public Monitor {
	public:
		Condition ok2read;
		Condition ok2write;
		Inode* inode;
		bool readable;
		bool writeable;
		int deviceNumber;
		string driverName;

	DeviceDriver(string driverName)
		: Monitor(),
			ok2read(this), 
			ok2write(this), 
			deviceNumber(drivers.size()), 
			driverName( driverName )
		{
			drivers.push_back(this);
			//++inode->openCount;	//segfault
		}

	~DeviceDriver() {
		//--inode->openCount;
	}

	virtual int read() {}
	virtual int write() {}
	virtual int seek() {}
	virtual int rewind() {}
	virtual int ioctl() {}
	virtual void online() {}
	virtual void offline() {}
	virtual void fireup() {}
	virtual void suspend() {}

};

//========================== iostreamDevice =================================

class iostreamDevice : public DeviceDriver {

	public: 
		int inodeCount = 0;
		int openCount = 0; 	//access_counter

		iostream* bytes;		//data

		//my additions
		mode_t mode;
		size_t offset = 0;

		unsigned int flags;
		
		iostreamDevice( iostream* io )
			: bytes(io), DeviceDriver("iostreamDevice")
		{
			cout << "*Inside iostreamDevice()\n";
			readable = false;
			writeable = false;
			cout << "readable: " << readable << endl;
			cout << "writeable: " << writeable << endl;
		}

		~iostreamDevice() {
		}

		int open( const char* pathname, int flags) {
			readable = !(flags & 0x01);
			writeable = (flags & 0x01) | (flags & 0x02);
			cout << "readable: " << readable << endl;
			cout << "writeable: " << writeable << endl;
			driverName = pathname;
			cout << "driverName: " << driverName << endl;
			openCount++;
			return deviceNumber; // return fd to device driver
		}

		int close( int fd) {
			openCount--;
			//~iostreamDevice();
			return 0;
		}

		int read( int fd, void* buf, size_t count) {
			iostreamDevice* ioD = (iostreamDevice*)drivers[fd];
			char* buffer = (char*)buf;

			int i = 0;
			for(i = 0; i < count; i++)
			{
				//return if eof encountered with amount written
				if(bytes->peek() == EOF)
					return i;

				//read character from bytes then store into buffer
				cout << "read position " << bytes->tellg() << endl;
				buffer[i] = bytes->get();
				cout <<"reading from device " << (char)buffer[i] << endl;
				offset++; 						
			}
			//return with amount read
			return i;		
		}

		int write( int fd, void const* buf, size_t count) {
			iostreamDevice* ioD = (iostreamDevice*)drivers[fd];
			char* buffer = (char*)buf;

			int i = 0;
			for(i = 0; i < count; i++)
			{
				//write character from buffer into bytes
				cout << "write position " << bytes->tellp() << endl;
				bytes->put(buffer[i]);
				cout <<"writing into device " << (char)buffer[i] << endl;
				offset++;				
			}
			//return with amount written
			return i;	
			
		}

		int seek( int fd, off_t offsetIn, int whence) {
			iostreamDevice* ioD = (iostreamDevice*)drivers[fd];

			//set position to passed in position
			if(whence == SEEK_SET)
			{
				//save position in our offset variable
				offset = offsetIn;

				//set positions
				bytes->seekp(offsetIn,ios_base::beg);
				bytes->seekg(offsetIn,ios_base::beg);
			}

			//set position to current position + passed in position
			else if(whence == SEEK_CUR)
			{
				//get end position 
				off_t save = bytes->tellp();
				bytes->seekp(0, ios_base::end);
				off_t end = bytes->tellp();
				bytes->seekp(save);

				//set offset to current offset + offset passed in
				offset += offsetIn;

				//if offset is past the end position then wrap around
				if (offset > end)
				{
					//set positions if wrapped around
					offset = offset % end;
					bytes->seekp(offset, ios_base::beg);
					bytes->seekg(offset, ios_base::beg);
					return offset;
				}

				//set positions if not wrapped around
				bytes->seekp(offsetIn, ios_base::cur);
				bytes->seekg(offsetIn, ios_base::cur);
				cout << "SEEK read position " << bytes->tellg() << endl;
				cout << "SEEK write position " << bytes->tellp() << endl;
			}
	
			//NOT SUPPORTED
			else if(whence == SEEK_END)
			{
				//NOT SUPPORTED
				/*
				//get end position to save in our offset variable
				off_t save = bytes->tellp();
				bytes->seekp(0, ios_base::end);
				offset = bytes->tellp();
				offset += offsetIn;
				bytes->seekp(save);

				//set positions
				bytes->seekp(offsetIn,ios_base::end);
				bytes->seekg(offsetIn,ios_base::end);
				//*/
			} 
			return offset;
			
		}

		//rewind is a modification of seek
		int rewind( int pos ) {

		}

		/*
		int ioctl(  ) {

		}
		//*/
};

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
		inode->driver = this;
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
		if( whence == ios_base::SEEK_SET )
		{
			bytes->seekg(offset,beg);
			bytes->seekp(offset,beg);
		}
		else if( whence == ios_base::SEEK_CUR )
		{
			bytes->seekg(offset,cur);
			bytes->seekp(offset,cur);
		}
		else if( whence == ios_base::SEEK_END )
		{
			bytes->seekg(offset,end)
			bytes->seekp(offset,end)
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

//cout messes up output, so wrote my own function
void myPrint(char* buf, int size)
{
	cout << "============= ";
	for(int i = 0; i < size; i++)
	{
		cout << buf[i];
	}
	cout << endl;
}

int main() {

	char buf[50];
	char buf2[] = "hello there my friend";	
	char buf3[1024];

	string str; 
	string test = "hello there my friend";

	memset(buf,0,50);
	memset(buf3,0,1024);

	//create i device
	strstreambuf sb(buf,50,buf);
	iostream s(&sb);
	iostreamDevice i = iostreamDevice(&s);
	                
	//create ios device     
 	stringstream SSstream;   
	iostreamDevice ios = iostreamDevice(&SSstream);
	int iosFD = ios.open(ios.driverName.c_str(), O_RDWR);  
	//int iosFD = ios.open("iostreamDevice", O_RDWR);  //does not work !!! does open need to be rewritten?

	/*//test read and write
	cout << "amount write: " << i.write(0,buf2,50) << endl;
	cout << "wrote this: " << buf2 << endl;
	cout << "====================" << endl;

	cout << "amount read: " << i.read(0,buf,40) << endl;
	cout << "read this: "<< buf << endl;
	cout << "====================" << endl;
	//*/

	/*//test read and write
	cout << "amount write: " << i.write(0,buf2,25) << endl;
	cout << "wrote this: " << buf2 << endl;
	cout << "====================" << endl;

	cout << "amount write: " << i.write(0,buf2,25) << endl;
	cout << "wrote this: " << buf2 << endl;
	cout << "====================" << endl;

	cout << "amount read: " << i.read(0,buf,50) << endl;
	cout << "read this: "<< buf << endl;
	cout << "====================" << endl;
	//*/

	/*//test seeking 
	cout << "amount write: " << i.write(0,buf2,25) << endl;
	cout << "wrote this: " << buf2 << endl;
	cout << "====================" << endl;

	cout << "seeking to : " << i.seek(0,5,SEEK_SET) << endl;
	cout << "====================" << endl;

	cout << "amount write: " << i.write(0,buf2,25) << endl;
	cout << "wrote this: " << buf2 << endl;
	cout << "====================" << endl;

	cout << "amount read: " << i.read(0,buf,50) << endl;
	cout << "read this: "<< buf << endl;
	cout << "====================" << endl;
	//*/

	///* 
 	while (getline(cin, str)) // Reads line into str
	{ 
		//write str into stream
		ios.write(iosFD, str.c_str(), str.length());
 	}

	//read 100 characters from ios device into buf3, then print buf3
	ios.read(iosFD,buf3,100);
	myPrint(buf3,100);
   //*/

	//code below breaks when writing into ios device from buf2 !!! when using prevoius code lines 340 - 348
	//this works if previous code lines 340 - 348 is commented out

	//write buf2 to ios device twice
	ios.write(iosFD,buf2,50);
	ios.write(iosFD,buf2,50);
	
	//seek 5 from beginning then seek 5 from current position
	ios.seek(iosFD,5,SEEK_SET);
	ios.seek(iosFD,5,SEEK_CUR);

	//read 100 characters from ios device into buf3, then print buf3
	ios.read(iosFD,buf3,100);
	myPrint(buf3,100);
	
	// Stringstream test harness
	stringstream* ss = new stringstream;
	stringstreamDevice* ssDevice = new stringstreamDevice(ss);
	int ssDeviceFd = -1;
	char writeBuf[14] = "Hello, world!";
	char* readBuf = new char[14];
	
	// Open a file to test our input and output.
	ssDeviceFd = ssDevice->open("writeTest.txt", 2);

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
	
 	return 0;
}
