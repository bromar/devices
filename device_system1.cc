#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <cassert>
#include <sstream>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <map>
#include <queue>
#include <iomanip>
#include <signal.h>
#include <sys/time.h>
#include <climits>
#include <mutex>
#include <condition_variable>
using namespace std;
#define EXCLUSION Sentry exclusion(this); exclusion.touch();
//Define flags according to the linux standard.
#define O_RDONLY	0
#define O_WRONLY	1
#define O_RDWR		2
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
class Device;
class Monitor {
// ...
};
class Condition {
public:
Condition( Monitor* m ) {}
// ...
};
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
	Device* driver;
	string* bytes;
};

vector<Inode> ilist;
vector<Device*> drivers;

class Device: public Monitor {
	public:
	Condition ok2read;
	Condition ok2write;
	Inode* inode;
	bool readable;
	bool writeable;
	int deviceNumber;
	string driverName;
	Device(string driverName)
	:Monitor(),ok2read(this),
	ok2write(this),deviceNumber(drivers.size()),
	driverName( driverName )
	{
	//Added this line, since inode was never initialized.
	inode = new Inode;
	inode->driver = this;
	drivers.push_back(this);
	++inode->openCount;
	}
	~Device() {
	--inode->openCount;
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

class iostreamDevice : Device {
public:
int inodeCount = 0;
int openCount = 0;
iostream* bytes;
iostreamDevice( iostream* io )
: bytes(io), Device("iostreamDevice")
{
readable = true;
writeable = true;
}
~iostreamDevice() {
}
int open( const char* pathname, int flags) {
}
int close( int fd) {
}
int read( int fd, void* buf, size_t count) {
}
int write( int fd, void* buf, size_t count) {
}
int seek( int fd, off_t offset, int whence) {
}
int rewind( int pos ) {
}
/*
int ioctl( ) {
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
		cout << "\nClosing device " << driverName;
		--openCount;
		cout << "\nDevice " << driverName << " closed";
		return 0;
	}
	int read( int fd, void* buf, size_t count) {
		stringstream* data;
		*data << *(ilist[fd].bytes);
		string* s;
		int i = 0;
		for( ; i < count && !bytes->eof(); i++)
		{
		*data >> *s;
		}
		buf = s;
		return i;
	}
	int write( int fd, void* buf, size_t count) {
		Device* ssd = drivers[fd];
		cerr << "Beginning write to device " << ssd->driverName;
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
	//This thing is lying to me. It says beg, cur, and end aren't
	//declared when they clearly are in ios_base.
	//Will finish when my will returns.
	/*	if( whence == SEEK_SET )
	{
	bytes->seekg(offset,beg);
	}
	else if( whence == SEEK_CUR )
	{
	bytes->seekg(offset,cur);
	}
	else if( whence == SEEK_END )
	{
	bytes->seekg(offset,end)
	}
	return bytes->tellg();*/
	}
	int rewind( int pos ) {
	//	seekg( deviceNumber, pos, SEEK_SET);
	}
	/*
	int ioctl( ) {
	}
	//*/
};

int main() {
/*	iostream* io;
	iostreamDevice ioDevice(io);
	int ioDeviceFd = ioDevice.deviceNumber;
	char* buf;
	
	// Open a file to test our input and output.
	ioDevice.open("writeTest.txt", 0);
	assert( ioDevice );
	cout << "\nWrite test file opened";
	
	// Read the following text into a file.
	*io << "Hello, world!";
	ioDevice.write(ioDeviceFd, buf, 13);
	cout << "\nStream contents written to write test file"

	// Close the file when finished.
	ioDevice.close(ioDeviceFd);
	assert( !ioDevice );
	cout << "\nWrite test file closed";
	
	// If the file was successfully written to, we can use it to test read.
	ioDevice.open("readTest.txt",0);
	*io << "Hello, world!";
	ioDevice.write(ioDeviceFd, buf, 13);
	cout << "\nStream contents written to read test file";
	ioDevice.read(ioDeviceFd, buf, 13);
	cout << "\nContents stored at test file: " << *buf;
*/
	stringstream* ss = new stringstream;
	stringstreamDevice ssDevice(ss);
	int ssDeviceFd = -1;
	string* buf;
	
	// Open a file to test our input and output.
	ssDeviceFd = ssDevice.open("writeTest.txt", 2);

	assert( ssDeviceFd != -1 );
	cerr << "Write test stream opened\n";
	
	// Read the following text into a file.
	*buf = "Hello, world!";
	ssDevice.write(ssDeviceFd, buf, 13);
	cerr << "Buffer contents written to write test stream\n";

	// Close the file when finished.
	ssDeviceFd = ssDevice.close(ssDeviceFd);
	assert( ssDeviceFd == 0 );
	cout << "Write test stream closed\n";
	
	// If the file was successfully written to, we can use it to test read.
	ssDeviceFd = ssDevice.open("writeTest.txt", 2);
	*ss << "Hello, world!";
	ssDevice.write(ssDeviceFd, buf, 13);
	cout << "Stream contents written to read test stream\n";
	ssDevice.read(ssDeviceFd, buf, 13);
	cout << "Contents stored in test stream: " << *buf << endl;
	
}
