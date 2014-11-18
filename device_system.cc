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
				freedDeviceNumbers.erase(freedDeviceNumbers.cbegin());
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
	
	/*virtual int open() {}
	virtual int read() {}
	virtual int write() {}
	virtual int seek() {}
	virtual int rewind() {}
	virtual int ioctl() {}*/
	virtual void open() {}
	virtual void read() {}
	virtual void write() {}
	virtual void seek() {}
	virtual void rewind() {}
	virtual void ioctl() {}
	virtual void online() {}
	virtual void offline() {}
	virtual void fireup() {}
	virtual void suspend() {}

};


class streamDevice : Device {

	public: 
		int inodeCount = 0;
		int openCount = 0;
		
		ios *bytes;

		streamDevice( ios *io )
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
			cout << "readable: " << readable << endl;
			cout << "writeable: " << writeable << endl;
			driverName = pathname;
			cout << "driverName: " << driverName << endl;
			openCount++;
			return deviceNumber; // return fd to device driver
		}

		// return non-zero on error
		// should close() be a general function outside of a class?
		// seems strange to be able to close other FDs from within
		// a specific (this) device. Possibly change to int close()
		// where the device to be closed is this.
		int close(int fd) {
			//openCount = (openCount > 0) ? openCount - 1 : 0;
			// remove device from vector of drivers
			auto i = begin(drivers);
			while (i != end(drivers)) {
			    if ((*i)->deviceNumber == fd) {
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

		int read(int fd, void* buf, size_t count) {
			return -1;
		}

		int seek(int fd, off_t offset, int whence) {
			return -1;
		}

		int rewind(int pos) {
			return -1;
		}

		/*
		int ioctl(int d, unsigned long request, ... ) {

		}
		*/
};


int main(int argc, char **argv)
{
	cout << "Hello world!\n\n";

	streamDevice is1 = streamDevice(&cin);
	cout << "Read yes --\n";
	int dn1 = is1.open("~/Desktop/tes1.txt",ODD_RDONLY);
	cout << "deviceNumber: " << dn1 << endl << endl;

	streamDevice is2 = streamDevice(&cin);
	cout << "Write yes --\n";
	int dn2 = is2.open("~/Desktop/test2.txt",ODD_WRONLY);
	cout << "deviceNumber: " << dn2 << endl << endl;
	
	streamDevice is3 = streamDevice(&cin);
	cout << "Read+Write yes --\n";
	int dn3 = is3.open("~/Desktop/test3.txt",ODD_RDWR);
	cout << "deviceNumber: " << dn3 << endl << endl;

	cout << "Closing deviceNumber: " << dn2 << endl << endl;
	is2.close(dn2);

	streamDevice is4 = streamDevice(&cin);
	cout << "Write yes --\n";
	int dn4 = is4.open("~/Desktop/test4.txt",ODD_WRONLY);
	cout << "deviceNumber: " << dn4 << endl << endl;

	cout << "\nBye cruel world!\n";
}
