/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Gonzalo Castillo
	UIN:733006800
	Date: 9/28/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <iostream>
using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	bool c = false;	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:c")) != -1) {
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
			case 'c':
				c= true;
				break;
		}

	}
	int status;
	pid_t pid= fork();
		if(pid<0){
			perror("fork error");
			return -1;
		}
		if(pid==0){
			char* args[]={(char*)"./server", nullptr};
			execvp(args[0],args);
			perror("execvp");
			exit(1);
		}else{
			
			FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
			FIFORequestChannel* newchan = &chan;
			if(c){
			//replace the current channel pointer with the new chan
			MESSAGE_TYPE new_message = NEWCHANNEL_MSG;
			chan.cwrite(&new_message,sizeof(MESSAGE_TYPE));
			char r[MAX_MESSAGE];
			
			chan.cread(&r,sizeof(r));
			
			newchan=new FIFORequestChannel("data1_",FIFORequestChannel::CLIENT_SIDE);
			}

			//if the file name is not provided, do that data request
			if(filename.empty()){
				// example data point request
				char buf[MAX_MESSAGE]; // 256
				datamsg x(p, t, e);
				
				memcpy(buf, &x, sizeof(datamsg));
				newchan->cwrite(buf, sizeof(datamsg)); // question
				double reply;
				newchan->cread(&reply, sizeof(double)); //answer
				cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;

				//4.2 collecting the first 1000 data points
				//make the output file in the received directory
				ofstream out_file;
				//open the output file in written binary
				out_file.open("received/x1.csv");
				for(int i=0;i<1000;i++){
					datamsg msg1(p,i*.004,1);//msg for the first point
					datamsg msg2(p,i*.004,2);//msg for the second point
					char* buf1=new char[MAX_MESSAGE];
					char* buf2=new char[MAX_MESSAGE];
					//get the information of the first ecg point by writing the message to the server and reading what it replys.
					memcpy(buf1,&msg1,sizeof(datamsg));
					newchan->cwrite(buf1,sizeof(datamsg));
					double replyb1;
					newchan->cread(&replyb1, sizeof(double));
					//get the information for the second ecg point 
					memcpy(buf2,&msg2,sizeof(datamsg));
					newchan->cwrite(buf2,sizeof(datamsg));
					double replyb2;
					newchan->cread(&replyb2,sizeof(double));
					//char message=i*.004+*buf1+*buf2;
					//fwrite(buf1, 1,bytes_received,file);
					out_file<<i*.004<<", "<<replyb1<<", "<<replyb2<<std::endl;
					delete[] buf1;
					delete[] buf2;
					
				}

				out_file.close();
			}else{
			// sending a non-sense message, you need to change this
			filemsg fm(0, 0);
			string fname = filename;//replace the random file with the filename
			
			int len = sizeof(filemsg) + (fname.size() + 1);
			char* buf2 = new char[len];
			memcpy(buf2, &fm, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), fname.c_str());
			newchan->cwrite(buf2, len);  // I want the file length;
			//if you dont read out anything, the client wont terminate because 
			//you are waiting for the client to read what the server is 
			//outputting.
			__int64_t file_size;
			newchan->cread(&file_size,sizeof(__int64_t));
			delete[] buf2;
			//make the output file in the received directory
			string out_file = "received/"+filename;
			//open the output file in written binary
			FILE* file = fopen(out_file.c_str(),"wb");
			if(!file){
				perror("cannot open file");
				exit(1);
			}
			__int64_t remaining_size=file_size;
			__int64_t offset=0;
			while(remaining_size>0){
				int bytes_to_read;
				if(remaining_size<MAX_MESSAGE){//the bytes you need to read are either the MAX_MESSAGE long or less than that
					bytes_to_read=remaining_size;
				}else{
					bytes_to_read=MAX_MESSAGE;

				}
				//get the file lendth and its offset
				filemsg fm(offset,bytes_to_read);
				//write the file name into the server so that we can read the contents of the file
				int len = sizeof(filemsg) + (fname.size() + 1);
				char* buf2 = new char[len];
				memcpy(buf2, &fm, sizeof(filemsg));
				strcpy(buf2 + sizeof(filemsg), fname.c_str());
				newchan->cwrite(buf2,len);
				char* reply=new char [bytes_to_read];
				//obtain the contents of the file in the server at a bytes_to_read length
				newchan->cread(reply, bytes_to_read);
				//write the contents into the output file
				fwrite(reply, 1, bytes_to_read,file);

				delete[] buf2;
				delete[] reply;
				//increase the offset and decrease the remaining size of the file
				remaining_size-=bytes_to_read;
				offset+=bytes_to_read;
			}
			//close the output file
			fclose(file);
		}
			// closing the channel    
			MESSAGE_TYPE m = QUIT_MSG;
			//chan2.cwrite(&m, sizeof(MESSAGE_TYPE));
			newchan->cwrite(&m, sizeof(MESSAGE_TYPE));
			//chan.~FIFORequestChannel(); 
			 //newchan->~FIFORequestChannel();
                        //I do not have to delete newchan if c=false because I did not allocate any new memory
                        if(c){//only delete newchan if c = true
                                newchan->cwrite(&m, sizeof(MESSAGE_TYPE));
                                delete newchan;
                                newchan=nullptr;
                        }
			//close the control channel aswell
			chan.cwrite(&m, sizeof(MESSAGE_TYPE));
			wait(&status);
			
			//chan.~FIFORequestChannel();
		
		}
	return 0;
}
