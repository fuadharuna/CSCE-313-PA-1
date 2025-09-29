/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Fuad Haruna	
	UIN: 134004237
	Date: 09/26/25
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;  
	double t = -1;  
	int e = -1;  

	int m = MAX_MESSAGE; //Buffer capacity at max capacity
	bool newChannel = false;
	vector<FIFORequestChannel*> availableChannels;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {  
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				m = atoi (optarg);  
				break;
			case 'c':
				newChannel = true;
				break;
		}
	}

	//TASK 1
	int child_pid = fork();

	if (child_pid == 0){
		//runs through child process
		string m_str = to_string(m); 
		char *args[] = {(char*)"./server", (char*)"-m", (char*)m_str.c_str(), NULL};
		int ret = execvp(args[0], args);
		if(ret == -1){
			perror("execvp");
			exit(EXIT_FAILURE);
		}
	}else{
		FIFORequestChannel controlChan("control", FIFORequestChannel::CLIENT_SIDE);
		availableChannels.push_back(&controlChan);

		char chanName[100]; //Increased buffer size to 100

		if (newChannel){
			//make new channel
			MESSAGE_TYPE newC = NEWCHANNEL_MSG;
			controlChan.cwrite(&newC, sizeof(MESSAGE_TYPE));
			//use chanName to read
			//cread response from server
			controlChan.cread(chanName, sizeof(chanName));  // Use sizeof(chanName)
			//create new channel using chanName read from server
			FIFORequestChannel* newChan = new FIFORequestChannel(chanName, FIFORequestChannel::CLIENT_SIDE);
			//use new to mark channel as new for if statement
			//add new channel to available channels list
			availableChannels.push_back(newChan);	
		}
		
		FIFORequestChannel chan = *(availableChannels.back());
		char buf[MAX_MESSAGE]; // 256

		// Fixed: Simplified condition
		if(p != -1 && t != -1 && e != -1){
			datamsg x(p, t, e);
			memcpy(buf, &x, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg)); // question
			double reply;
			chan.cread(&reply, sizeof(double)); //answer
			cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
		}
		// Fixed: Simplified condition - just check if p != -1
		else if(p != -1){
			//write to x1.csv
			ofstream oFile;
			oFile.open("received/x1.csv");
			double time = 0.0;
			double r1, r2;

			//loop first 1000 lines for specified person -> 2000 requests for ecg1 & ecg2
			for(int i = 0; i < 1000; i++){
				datamsg e1(p, time, 1);
				memcpy(buf, &e1, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg)); //questions
				chan.cread(&r1, sizeof(double));

				datamsg e2(p, time, 2);
				memcpy(buf, &e2, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg)); // question
				chan.cread(&r2, sizeof(double)); //answer

				oFile << time << ',' << r1 << ',' << r2 << '\n';

				time += 0.004;
			}
			oFile.close(); //To close file
		}
		// Fixed: Simplified condition
		else if(filename != ""){
			//requesting full file
			filemsg fm(0, 0);
			string fname = filename;
			
			int len = sizeof(filemsg) + (fname.size() + 1);
			char* buf2 = new char[len]; //new buf
			memcpy(buf2, &fm, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), fname.c_str());
			chan.cwrite(buf2, len); //want file length

			int64_t filesize = 0;
			chan.cread(&filesize, sizeof(int64_t));

			// Fixed: Open file with ios::binary flag
			ofstream oFile("received/" + fname, ios::binary);
			char* buf3 = new char[m];

			filemsg* fileRequest = (filemsg*)buf2; //casts char to filemsg
			
			// Fixed: Simplified loop using offset-based iteration
			for (int64_t offset = 0; offset < filesize; offset += m){
				int chunk_len = min((int64_t)m, filesize - offset);  // Fixed: Calculate chunk size properly
				
				fileRequest->offset = offset; // set offset in the file
				fileRequest->length = chunk_len;  // Use chunk_len

				chan.cwrite(buf2, len); //read response
				//read server response and place in buf3
				chan.cread(buf3, chunk_len);  
				//write buf3 into file
				oFile.write(buf3, chunk_len);  // Use write() method
			}
			oFile.close();
			delete[] buf2;
			delete[] buf3;
		}
	
		// closing the channel    
		MESSAGE_TYPE quit_msg = QUIT_MSG;  // Renamed variable from 'm' to 'quit_msg'

		// Fixed: Loop through all channels to send quit message
		for (FIFORequestChannel* ch : availableChannels) {
			ch->cwrite(&quit_msg, sizeof(MESSAGE_TYPE));
		}
		
		// Fixed: Delete dynamically allocated channels (skip index 0 - stack allocated)
		for (size_t i = 1; i < availableChannels.size(); ++i) {
			delete availableChannels[i];
		}
		
		wait(NULL);  // Fixed: Added wait to reap child process
	}
}
