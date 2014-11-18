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

class iostreamDevice : DeviceDriver {

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
				if(bytes->eof())
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

			if(whence == SEEK_SET)
			{
				//save position in our offset variable
				this->offset = offsetIn;

				//set positions
				bytes->seekp(offsetIn,ios_base::beg);
				bytes->seekg(offsetIn,ios_base::beg);
			}
			else if(whence == SEEK_CUR)
			{
				//save position in our offset variable
				this->offset += offsetIn;

				//set positions
				bytes->seekp(offsetIn,ios_base::cur);
				bytes->seekg(offsetIn,ios_base::cur);
			}
			else if(whence == SEEK_END)
			{
				//get end position to save in our offset variable
				off_t save = bytes->tellp();
				bytes->seekp(0, ios_base::end);
				this->offset = bytes->tellp();
				this->offset += offsetIn;
				bytes->seekp(save);

				//set positions
				bytes->seekp(offsetIn,ios_base::end);
				bytes->seekg(offsetIn,ios_base::end);
			} 
			return this->offset	;
			
		}

		int rewind( int pos ) {
			
		}

		/*
		int ioctl(  ) {

		}
		//*/
};

int main() {

	char buf[50];
	string test = "hello there my friend";
	char* buf2 = "hello there my friend";	
	char buf3[1024];

	strstreambuf sb(buf,50,buf);
	iostream s(&sb);
	iostreamDevice i = iostreamDevice(&s);

	

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

 	string str;                      
 	stringstream SSstream;   
	iostreamDevice ios = iostreamDevice(&SSstream);
 
 	while (getline(cin, str)) // Reads line into str
	{ 
		//write str into stream
		ios.write(0,str.c_str(),str.length());
 	}

	//read all from stream and output
	ios.read(0,buf3,100);
	cout << buf3 << endl;
 	return 0;
}

































