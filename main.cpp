#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "devices.h"

using namespace std;

#define IDEVICE 1
#define ODEVICE 2
#define IODEVICE 3
enum menu_states{ INIT, CREATE, OPERATIONS };

void myPrint(char* buf, int size)
{
	cout << "myPrint: ";
	for(int i = 0; i < size; i++)
	{
		cout << buf[i];
	}
	cout << "." << endl;
}

int main(int argc, char *argv[])
{
	int dev_type = 0;
	int op_type = 0;
	menu_states state = INIT;
	stringstream ss_idev;
	stringstream ss_odev;
	stringstream ss_iodev;
	
	iDevice<char*> cur_idev(&ss_idev);
	oDevice<char*> cur_odev(&ss_odev);
	ioDevice<char*> cur_iodev(&ss_iodev);
	
	char buf1[50];
	memset(buf1,0,50);
	string str("Hello World!");
int cur_idev_fd = -1;

	while(1)
	{	
		switch(state)
		{
			case INIT:
	//			cur_dev = 0;
				state = CREATE;
				break;
	
			case CREATE:
				cout << "Which device would you like to create?" << endl;
				cout << "1. iDevice" << endl;
				cout << "2. oDevice" << endl;
				cout << "3. ioDevice" << endl;
	
				cin >> dev_type;
	
				//create iDevice	
				if(dev_type == IDEVICE)
				{
					ss_idev << str;
					cout << "ss_idev: " << ss_idev.str() << endl;
					cout << "ss_idev length: " << ss_idev.str().length() << endl;
					cout << "StringStream value passed to iDevice constructor: ";
					cout << ss_idev.str() << endl;
				}
				//create oDevice
				else if(dev_type == ODEVICE)
				{
					ss_odev << str;
					cout << "ss_odev: " << ss_odev.str() << endl;
					cout << "ss_odev length: " << ss_odev.str().length() << endl;
					cout << "StringStream value passed to oDevice constructor: ";
					cout << ss_odev.str() << endl;
				}
				//create ioDevice
				else if(dev_type == IODEVICE)
				{
					ss_iodev << str;
					cout << "ss_iodev: " << ss_iodev.str() << endl;
					cout << "ss_iodev length: " << ss_iodev.str().length() << endl;
					cout << "StringStream value passed to ioDevice constructor: ";
					cout << ss_iodev.str() << endl;
				}
	
				state = OPERATIONS;
				break;
	
			case OPERATIONS:
				cout << "Which operation would you like to perform?" << endl;
				cout << "1. open" << endl;	
				cout << "2. read" << endl;
				cout << "3. write" << endl;
				cout << "4. seek" << endl;
				cout << "5. rewind" << endl;
				cout << "6. show contents" << endl;
				cout << "7. create new device" << endl;
	
				cin >> op_type;
				
				//open
				if(op_type == 1)
				{
					if(dev_type == IDEVICE)
					{
						cout << "+ Testing iDevice open()\n";
						cur_idev.open("iDevice",ODD_RDWR);
					}
					else if(dev_type == ODEVICE)
					{
						cout << "+ Testing oDevice open()\n";
						cur_odev.open("oDevice",ODD_RDWR);
					}
					else if(dev_type == IODEVICE)
					{
						cout << "+ Testing ioDevice open()\n";
						cur_iodev.open("ioDevice",ODD_RDWR);
					}
				}
				
				//read
				else if(op_type == 2)
				{
					//odevice cant read
					if(dev_type == ODEVICE)
					{
						cout << "oDevice cant read" << endl;
						break;
					}
	
					if(dev_type == IDEVICE)
					{	
						cout << "+ Testing iDevice read()\n";
						cout << "buf1: " << buf1 << endl;
						cur_idev.seek(5,SEEK_SET);
						cur_idev.read(buf1,12);	
					}	
					//ioDevice	
					else
					{	
						cout << "+ Testing ioDevice read()\n";	
						cout << "buf1: " << buf1 << endl;
						cur_iodev.seek(5,SEEK_SET);
						cur_iodev.read(buf1,12);
					}
					
					myPrint(buf1,12);	
				}
				
				//write
				else if(op_type == 3)
				{
					//iDevice cant write
					if(dev_type == ODEVICE)
					{
						cout << "iDevice cant write" << endl;
						break;
					}
				
					if(dev_type == ODEVICE)
					{	
						cout << "+ Testing oDevice write()\n";
						cout << "buf1: " << buf1 << endl;	
						//Programmer's responsibility to NOT write beyond buffer size!
						cur_odev.write(buf1,10);
					}
					//ioDevice
					else
					{
						cout << "+ Testing ioDevice write()\n";	
						cout << "buf1: " << buf1 << endl;
						//Programmer's responsibility to NOT write beyond buffer size!
						cur_iodev.write(buf1,10);
					}
					
				}
				
				//seek
				else if(op_type == 4)
				{
					//iDevice
					if(dev_type == IDEVICE)
					{
						cout << "+ Testing iDevice seek(SEEK_SET)\n";
						cur_idev.seek(-2,SEEK_SET);
						cur_idev.read(buf1,2);
						myPrint(buf1,12);
	
						cout << "+ Testing iDevice seek(SEEK_CUR)\n";
						cur_idev.seek(0,SEEK_SET);
						cur_idev.seek(-2,SEEK_CUR);
						cur_idev.read(buf1,2);
						myPrint(buf1,12);
	
						cout << "+ Testing iDevice seek(SEEK_END)\n";
						cur_idev.seek(-2,SEEK_END);
						cur_idev.read(buf1,2);
						myPrint(buf1,12);
					}
					//oDevice
					else if(dev_type == ODEVICE)
					{
						cout << "+ Testing oDevice seek(SEEK_SET)\n";
						cur_odev.seek(-1,SEEK_SET);
	
						cout << "+ Testing oDevice seek(SEEK_CUR)\n";
						cur_odev.seek(100,SEEK_CUR);
	
						cout << "+ Testing oDevice seek(SEEK_END)\n";
						cur_odev.seek(-10,SEEK_END);
					}
					//ioDevice
					else
					{
						cout << "+ Testing ioDevice seek()\n";
						cur_odev.seek(5,SEEK_SET);
					}
				}
				
				//rewind
				else if(op_type == 5)
				{
					if(dev_type == IDEVICE)
					{	
						cout << "+ Testing iDevice rewind()\n";
						cur_idev.rewind();	
					}	
					else if(dev_type == ODEVICE)
					{
						cout << "+ Testing oDevice rewind()\n";	
						cur_odev.rewind();
					}
					//ioDevice
					else
					{
						cout << "+ Testing ioDevice rewind()\n";
						cur_iodev.rewind();
					}
	
				}
				
				//show contents
				else if(op_type == 6)
				{
					cout << "+ showing contents" << endl;
					//myPrint(buf1,12);
				}
				
				//create new device
				else if(op_type == 7)
				{
					state = CREATE;
					break;
				}
	
				state = OPERATIONS;
				break;
		}
	}

	return 0;
}
