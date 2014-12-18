#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "devices.h"

using namespace std;


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
	/*
		DO NOT PASS STDIN/STDOUT TO iDevice()
	*/

	char buf1[50];
	memset(buf1,0,50);

	/*
		iDevice Tests
	*/
	string str("Hello World!");
	stringstream ss;
	ss << str;
	cout << "StringStream value passed to iDevice constructor: " << ss.str() << endl;

	iDevice<char*> in(&ss);

	cout << "+ Testing iDevice read()\n";
	in.read(buf1,15);
	myPrint(buf1,12);
	
	cout << "+ Testing iDevice seek(SEEK_SET)\n";
	in.seek(-2,SEEK_SET);
	in.read(buf1,2);
	myPrint(buf1,12);


	cout << "+ Testing iDevice seek(SEEK_CUR)\n";
	in.seek(0,SEEK_SET);
	in.seek(-2,SEEK_CUR);
	in.read(buf1,2);
	myPrint(buf1,12);

	cout << "+ Testing iDevice seek(SEEK_END)\n";
	in.seek(-2,SEEK_END);
	in.read(buf1,2);
	myPrint(buf1,12);



	/*
		oDevice Tests
	*/
	stringstream ss2;
	ss2 << str;
	cout << "ss2: " << ss2.str() << endl;
	cout << "ss2 length: " << ss2.str().length() << endl;
	oDevice<char*> out(&ss2);

	cout << "+ Testing oDevice write()\n";
	cout << "buf1: " << buf1 << endl;
	//Programmer's responsibility to NOT write beyond buffer size!
	out.write(buf1,10);

	cout << "+ Testing oDevice seek(SEEK_SET)\n";
	out.seek(-1,SEEK_SET);

	cout << "+ Testing oDevice seek(SEEK_CUR)\n";
	out.seek(100,SEEK_CUR);

	cout << "+ Testing oDevice seek(SEEK_END)\n";
	out.seek(-10,SEEK_END);



	/*
		ioDevice Tests
	*/
	stringstream ss3;
	ss3 << str;
	cout << "ss3: " << ss3.str() << endl;
	cout << "ss3 length: " << ss3.str().length() << endl;
	ioDevice<char*> inout(&ss3);

	cout << "+ Testing ioDevice open()\n";
	inout.open("ioDevice",ODD_RDWR);

	cout << "+ Testing ioDevice seek()\n";
	inout.seek(5,SEEK_SET);

	cout << "+ Testing ioDevice rewind()\n";
	inout.rewind();

	cout << "+ Testing ioDevice read()\n";
	cout << "buf1: " << buf1 << endl;
	inout.seek(5,SEEK_SET);
	inout.read(buf1,12);
	myPrint(buf1,12);

	cout << "+ Testing ioDevice write()\n";
	cout << "buf1: " << buf1 << endl;
	inout.write(buf1,12);
	myPrint(buf1,12);

	cout << drivers["ioDevice"]->driverName << endl;// << drivers[0]->second << endl;




	/*
		ioDevice Tests - fstream
	*/
	cout << "+ Testing ioDevice(fstream*) constructor\n";
	char buftest[50];
	memset(buftest,0,50);
	string fname = "testfile.txt";
	fstream f(fname.c_str(),ios::in | ios::out);

	ioDevice<char*> fs(&f);
	fs.open(fname.c_str(), ODD_RDWR);


	/*
		ioDevice Tests - insertion and extraction operators
	*/
	cout << "+ Testing ioDevice extraction operator\n";
	memset(buftest,0,50);
	cout << "buftest before extraction of fstream object: " << buftest << endl;

	//example of use...
	//[ioDevice] >> [buffer size] >> [buffer];
	fs >> 26 >> buftest;
	//[ioDevice] >> [buffer];
	fs >> buftest;
	cout << "Buffer after extraction of fstream object: " << buftest << endl;


	cout << "+ Testing ioDevice insertion operator\n";
	//writes in hex to file instead of ascii characters. strange...
	char bufferTmp[26] = "One...two...three...four!";
	cout << "String to be inserted into fstream object: " << bufferTmp << endl;
	//cout << "String to be inserted into fstream object: " << tmp << endl;

	//[ioDevice] << [buffer size] << [buffer variable];
	fs << 26 << bufferTmp;
	//[ioDevice] << [buffer variable];
	fs << bufferTmp;

	char tmpp[100];
	memset(tmpp,0,100);
	fs >> tmpp;
	cout << "tmpp: " << tmpp << endl;
	
	/*
		ioDevice Tests - subscript operator
	*/
	//cout << "+ Testing ioDevice subscript operator\n";
	//cout << "fs[0]: " << fs[0] << endl;
	//char c = fs[1];
	//cout << "c = fs[1]: " << c << endl;

	/*
		ioDevice Tests - istream
	*/
	cout << "+ Testing ioDevice(iostream*) constructor\n";
	char buftest2[50];
	memset(buftest2,0,50);

	iDevice<char *> cindy(&cin);//not working as expected. expected to wait for input.
	cindy.open("cinDevice", ODD_RDWR);
	cindy >> buftest2;
	cout << "Buffer after extraction of cin object: " << buftest2 << endl;


 	return 0;
}
