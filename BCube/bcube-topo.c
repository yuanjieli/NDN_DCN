#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <string>

using namespace std;

#define MAX_NODE 20000
#define MAX_PORT 10
#define MAX_BCUBE_K 20

struct DNode
{
	public:
	DNode(int i): identity(i), portnumber(0){}
	DNode(){}
	long identity;
	long portlist[MAX_PORT];
	long portnumber;
	long nodeclass; //1: Switch; 2: Server
};

long DCNpow(long n, long exp)
{
	return (long)pow((double)n, (double)(exp));
}

string Bcubeid(long id, long tserverbegin, long tservernum, long tn, long k)   // 生成数组bid各位
{
	
	long i;
	ostringstream oss;

	id-=tserverbegin; //from 0
	tservernum/=tn;   //num of bcube(k-1)
	i=k;

	while (k>0)
	{
		oss << id/tservernum;
		id %= tservernum;
		tservernum/=tn;
		i--;
		k--;
	}
	
	return oss.str();
}



int main(int argc, char **argv)
{
	if(argc<4)
	{
		cout<<"bcube-topo n k output"<<endl;
		return -1;
	}
	
	DNode identityref[MAX_NODE];
	
	
	long n = atoi(argv[1]);	//#ports on each server
	long k = atoi(argv[2]);	//#BCube level, starting from 0
	
	k ++; 	//This is just used for compability with old code
	
	long i, nodenumber, serverbeginnumber, servernumber, switchnumber, j, id, tswitchlevelnumber, nextbegin, tlevelservernumber, tprenum, tportrange, p, switchlevel;
	
	nodenumber = DCNpow(n,k-1)*(n+k);
	serverbeginnumber = DCNpow(n,k-1)*k+1;
	servernumber = DCNpow(n,k);
	switchnumber = DCNpow(n,k-1);
	switchlevel = k;
	
	tswitchlevelnumber = switchnumber;
	tlevelservernumber = servernumber;
	tportrange = servernumber/n;
	
	for (i=1; i<=switchlevel; i++)  //各层交换机数相等
	{
		//cout<<"Level "<<switchlevel-1<<"========\n";
		nextbegin = serverbeginnumber;
		for (j=1; j<=switchnumber; j++)
		{
			id = (i-1)*switchnumber+j;
			identityref[id].identity = id;
			identityref[id].nodeclass = 1;
			identityref[id].portnumber = n;
			//cout<<"NDN Router R"<<id<<" with "<<n<<" ports"<<endl;

			if ((j-1)%tswitchlevelnumber == 0)
			{
				identityref[id].portlist[1] = nextbegin;
				//cout<<"NDN Router R"<<id<<"'s 1st port is connected to Server "<<nextbegin<<endl;
				tprenum = nextbegin;
				nextbegin += tlevelservernumber;
			}
			else
			{
				identityref[id].portlist[1] = ++tprenum;
				//++tprenum;
			}

			for (p=2; p<=identityref[id].portnumber; p++)
			{
				identityref[id].portlist[p] = identityref[id].portlist[p-1]+tportrange;
				//cout<<"NDN Router R"<<id<<"'s "<<p<<"th port is connected to Server"<<
			}
			
		}
		tswitchlevelnumber/=n;
		tlevelservernumber/=n;
		tportrange/=n;
		//cout<<"========\n\n";
	}

 
	for (i=serverbeginnumber; i<=serverbeginnumber+servernumber-1; i++)
	{
		identityref[i].identity = i;
		identityref[i].nodeclass = 2;
		identityref[i].portnumber = k;

		tswitchlevelnumber = switchnumber;
		tlevelservernumber = servernumber;

		for (j=1;j<=identityref[i].portnumber; j++)
		{
			identityref[i].portlist[j] = ((i-serverbeginnumber)/tlevelservernumber)*(tswitchlevelnumber)+(i-serverbeginnumber)%tswitchlevelnumber+(j-1)*switchnumber+1;
			tswitchlevelnumber/=n;
			tlevelservernumber/=n;
		}
	}
	
	//print all nodes in bcube
	/*cout<<"swich :"<<endl;
	for(int bc=1;bc<=switchnumber*k;bc++)
	{
		for(int port=1;port<=n;port++)
			cout<<"swich"<<bc<<" port "<<port<<" connects to "<<"server "<<identityref[bc].portlist[port]<<endl;
	}
	cout<<endl;
	for(int bc=switchnumber*k+1;bc<=nodenumber;bc++)
	{
		for(int port=1;port<=k;port++)
			cout<<"server"<<bc<<" port "<<port<<" connects to "<<"swith "<<identityref[bc].portlist[port]<<endl;
	}*/
	
	ofstream fout;
	fout.open(argv[3]);
	
	fout<<"#BCube("<<n<<","<<k-1<<")\n\n";
	
	fout<<"router\n\n"
			<<"# node  comment     yPos    xPos\n";	
	for(int bc=switchnumber*k+1;bc<=nodenumber;bc++)
	{
		//fout<<"S"<<bc-serverbeginnumber<<"     NA          1       1\n";
		fout<<"S"<<Bcubeid(bc, serverbeginnumber, servernumber, n, k)<<"     NA          1       1\n";
	}
	for(int bc=1;bc<=switchnumber*k;bc++)
	{
		fout<<"R"<<bc-1<<"     NA          1       1\n";
	}
	fout<<endl;
	
	fout<<"link\n\n"
			<<"# srcNode   dstNode     bandwidth   metric  delay   queue\n";
	for(int bc=1;bc<=switchnumber*k;bc++)
	{
		for(int port=1;port<=n;port++)
			//cout<<"swich"<<bc<<" port "<<port<<" connects to "<<"server "<<identityref[bc].portlist[port]<<endl;
			fout<<"R"<<bc-1<<"        S"<<Bcubeid(identityref[bc].portlist[port], serverbeginnumber, servernumber, n, k)<<"        100Mbps      1        1ms    20\n";
	}
	for(int bc=switchnumber*k+1;bc<=nodenumber;bc++)
	{
		for(int port=1;port<=k;port++)
			//cout<<"server"<<bc<<" port "<<port<<" connects to "<<"swith "<<identityref[bc].portlist[port]<<endl;
			fout<<"S"<<Bcubeid(bc, serverbeginnumber, servernumber, n, k)<<"        R"<<identityref[bc].portlist[port]-1<<"        100Mbps      1        1ms    20\n";
	}
	
	fout.close();
	
	
	return 0;
}