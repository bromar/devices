#include "devices.h"

//=====
//Globals
//=====

// signal mask pseudo constants
sigset_t InterruptSystem::on;
sigset_t InterruptSystem::alrmoff;
sigset_t InterruptSystem::alloff;

// used to maintain a record of reusable file descriptors
// deviceNumbers
InterruptSystem interrupts;// singleton instance.

vector<Inode> ilist;
vector<Device*> drivers;

vector<int> freedDeviceNumbers;

//============================================================


Monitor::Monitor( sigset_t mask )   
    : mask(mask) {}
Sentry::Sentry( Monitor* m ) 
: old( interrupts.block(m->mask) )                 
{}
Sentry::~Sentry() {
	interrupts.set( old );                              
}



Semaphore::Semaphore(int count)
	: count(count) {
	//
}
void Semaphore::release() {
	EXCLUSION
	++count;
	available.SIGNAL;
}
void Semaphore::acquire() {
	EXCLUSION
	while(count == 0) available.WAIT;
	--count;
}



Inode::Inode() {}
Inode::~Inode() {}
int Inode::unlink() {
	assert(linkCount > 0);
	--linkCount;
	cleanup();
	return 0;
}
void Inode::cleanup() {
	if (! openCount && !linkCount ) {
		//throwaway stuff
	}
}


// this constructor originally had an initialization of
// deviceNumber(drivers.size()) which is not safe. the
// case where drivers is resized (file descriptors removed)
// will cause at least two device drivers to have identical
// device numbers (indices of vector drivers).
// solution is to scan vector drivers for next available
// index (file descriptor).
Device::Device(string dn)
	:driverName( dn )
	{
		//cout << "*Inside Device()\n";
		readable = false;
		writeable = false;
		//cout << "readable: " << readable << endl;
		//cout << "writeable: " << writeable << endl;
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

Device::~Device() {
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

int Device::open(const char* pathname, int flags) {
	//Need to check for already existing pathname in Device
	//constructor! Not here.

	//auto i = begin(drivers);
	//while (i != end(drivers)) {
	//    if ((*i)->driverName == pathname) {
	//    	//cout << "this " << this << endl;
	//    	//cout << "i " << &(*i) << endl;
	//    	//*this = (&i);
	//        return (*i)->deviceNumber;
	//    }
	//    i++;
	//}

	readable = !(flags & 0x01);
	writeable = (flags & 0x01) | (flags & 0x02);
	driverName = pathname;
	openCount++;
	return deviceNumber; // return fd to device driver
}

// return non-zero on error
int Device::close() {
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

int Device::read() {return -1;}
int Device::write() {return -1;}
int Device::seek() {return -1;}
int Device::rewind() {return -1;}

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
int Device::ioctl(unsigned int cmd, unsigned long arg)
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

void Device::online() {}

void Device::offline() {}

void Device::fireup() {
	cout << "firing up  " << driverName << endl;
}
void Device::suspend() {
	cout << "suspending  " << driverName << endl;
}
void Device::shutdown() {
	cout << "shuting down " << driverName << endl;
}
void Device::initialize() {
	cout << "initializing down " << driverName << endl;
}
void Device::finalize() {
	cout << "finalizing " << driverName << endl;
}

void Device::completeWrite() {}
void Device::completeRead() {}


//==================================================================


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



/*
template< class Item >
iDevice::iDevice(istream *stream) {
	stream = stream;
	Device("iDevice");
}

template< typename Item >
iDevice::~iDevice() {}

template< typename Item >
int iDevice::input(Item* buffer, int n) {
	;//
}

template< typename Item >
int iDevice::read(Item* buffer, size_t count) {
	EXCLUSION
	int i = 0;
	for(i = 0; i < count; i++)
	{
		//return if eof encountered with amount read
		assert(buffer[i]);
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

template< typename Item >
int iDevice::seek(off_t newOffset, int whence) {
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

template< typename Item >
int iDevice::rewind(int pos) {
	seek(0,SEEK_SET);
	return 0;
}

template< typename Item >
void iDevice::completeRead() {
	EXCLUSION
	cout <<"readComplete" << endl;
	readCompleted = true;
	ok2read.SIGNAL;
}
*/
