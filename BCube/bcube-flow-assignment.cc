#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <set>
#include <string>
#include <cmath>
using namespace std;

//assign flows to bcube

int main(int argc, char ** argv)
{
	srand(time(0));
	if(argc<7)
	{
		cout<<"bcube-flow-assignment n k #flows traffic_volume(MB) immutable output"<<endl;
		return -1;
	}
	
	long n = atoi(argv[1]);	//#ports on each server
	long k = atoi(argv[2]);	//#BCube level, starting from 0
	long flow = atoi(argv[3]); //#consumer-producer flows
	//number of packets each flow needs to retrieve. We assume the Interest+DATA=1044byte
	long no_packet = atoi(argv[4])*1000000/1140;	
	
	//if true, each server can only be used ONCE
	//i.e. each consumer will just send ONE flow to each producer
	//AND each producer will only serve ONE consumer
	//AND producer will be the consumer, and vice versa
	long immutable = atoi(argv[5]);

	
	set<long> used_server;
	ofstream fout;
	fout.open(argv[6]);
	
	for(int count = 0; count != flow; count++)
	{
		//used for storing bcubeIDs
		int *consumer = new int[k+1];
		int *producer = new int[k+1];
		long producer_id = 0, consumer_id = 0;
		while(true)
		{
			for(int i=0;i!=k+1;i++)
			{
				consumer[i]=rand()%n;
				producer[i]=rand()%n;
				
				consumer_id += consumer[i]*(long)pow((double)n,i);
				producer_id += producer[i]*(long)pow((double)n,i);
			}
			if(consumer_id==producer_id)	//consumer and producer cannot be the same!!!
				continue;
			
			if(immutable)
			{
				if(used_server.find(consumer_id)==used_server.end()
				&& used_server.find(producer_id)==used_server.end())
				{
					used_server.insert(consumer_id);
					used_server.insert(producer_id);
					break;					
				}
				else
					continue;
			}
			else
			{
				break;
			}						
		}
		
		//now we have a pair
		stringstream ss1, ss2;
		ss1<<"S"; ss2<<"S";
		for(int i=0;i!=k+1;i++)
		{
			ss1<<consumer[i];
			ss2<<producer[i];
		}
		
		fout<<ss1.str()<<" "<<ss2.str()<<" "<<no_packet<<endl;
		//cout<<ss1.str()<<" "<<ss2.str()<<" "<<no_packet<<endl;
		
		delete[] consumer;
		delete[] producer;
	}
	
	fout.close();
	return 0;
	
}