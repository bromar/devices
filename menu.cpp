#include <iostream>

using namespace std;

enum menu_states{ INIT, CREATE, OPERATIONS } state;

int main()
{
	int input = 0;
	state = INIT;
	ioDevice cur_dev;

	switch(state)
	{
		case INIT:
			cur_dev = 0;
			state = CREATE;
			break;

		case CREATE:
			cout << "Which device would you like to create?" << endl;
			cout << "1. iostream" << endl;
			cout << "2. stringstream" << endl;
			cout << "3. fstream" << endl;

			cin >> input;
			if(input == 1)
			{
				//create iostream device

			}
			else if(input == 2)
			{
				//create stringstream device

			}
			else if(input == 3)
			{
				//create fstream device

			}

			state = OPERATIONS;
			break;

		case OPERATIONS:
			cout << "Which operation would you like to perform?" << endl;
			cout << "1. read" << endl;
			cout << "2. write" << endl;
			cout << "3. seek" << endl;
			cout << "4. rewind" << endl;
			cout << "5. show contents" << endl;
			cout << "6. create new device" << endl;

			cin >> input;
			//read
			if(input == 1)
			{
				cur_dev
			}
			//write
			else if(input == 2)
			{

			}
			//seek
			else if(input == 3)
			{
			
			}
			//rewind
			else if(input == 4)
			{

			}
			//show contents
			else if(input == 5)
			{

			}
			//create new device
			else if(input == 6)
			{

			}

			state = OPERATIONS;
			break;
	}

	return 0;
}
