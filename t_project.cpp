#include <stdio.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <time.h>
#include <cstring>
#include <vector>
#include <limits>
#include <ctime>

using namespace std;


 /* Global variables and pointers */
int my_id, dest, lifetime, neighbors, stp_len, RID;
int hop_count = 0; int seq =0;
int parent = -1;  //Meaning I have no parent
int next_hop;
int broadcast = 99;
int diameter = 6;
int send_config_interval = 5;
int receive_config_interval = 15;
int reconvergence = 5;
const int MAX_NODES = 20;   //the maximum number of nodes in a topology



double seconds_elapsed;

bool config_check = true;
bool event = true;
bool stp_mode = false;

string my_message, stp_config;

char data_type = 'D'; 
char config_type = 'C';
char n_ack = 'N';
char SOH = 'S';

ostringstream outstr;
istringstream instr;

/* to keep track of read lines in the channels */
int * read_count;
int * read_so_far;

string * out_channel;
string * in_channel;

string segment_buffer[MAX_NODES][100];
int segment_count[MAX_NODES];

bool * forward_data;
bool * forward_config;

ofstream * out_file;
ifstream * in_file;
ofstream output;
ofstream output2;

vector<int>neighbor_id;

time_t stp_start;
time_t current_time;
time_t send_config;
time_t time_to_send_config;
time_t  * time_to_receive_config;


/* functions sending */
void transport_send(string &my_message, char type, int dest, int seq);
string transport_encap(string &my_message, char type, int dest, int seq);
void network_receive_from_transport(string &t_data, int  dest, int t_len);
void datalink_receive_from_network(string &n_data, int n_len, int next_hop);

/* functions receiving */
void datalink_receive_from_channel();
string datalink_decap(string frame);
void network_receive_from_datalink(string net_packet, int node_index);
void decide_stp(string config_packet, int node_index);
string network_decap(string packet);
void transport_receive_from_network(string trans_segment);
void run_stp(int how_many_times);


main(int argc, char *argv[]) { 

	cout<<endl;
	/* check for accurate command line arguments */
	if(argc <= 5){
		cout<<"Usage: code node_id lifetime dest message neighbors\n";
		cout<<"eg: ./a.out 1 100 2 \"I am the root\" 2 3\n";
		exit(1);
	}
	
	time_t start_time = time(NULL);
	double lifetime_elapsed;

	/* Extracting command line arguments */
    my_id = atoi(argv[1]);
    lifetime = atoi(argv[2]);
    dest = atoi(argv[3]);
    string my_message(argv[4]);
    RID = my_id;      // I am the root initially
    hop_count = 0;
    
	
    /* temporal variables */
    int temp_int =0;
    string temp_str;
    
    /* Populate neighbors into a vector */
    for(int i=0; i < argc - 5; i++){
    	neighbor_id.push_back(atoi(argv[5+i]));
    }
    
    neighbors = neighbor_id.size();
    
   /* define and set channel names already declared */
    string o_channel[neighbors];
    string i_channel[neighbors];
    
    out_channel = o_channel;
    in_channel = i_channel;
    
    /* define and set stream objects initially declared*/
    ofstream out_fil[neighbors];
    ifstream in_fil[neighbors];
    
    out_file = out_fil;
    in_file = in_fil;
    
    /* define and set global read count parameters initially declared */
    int read_countt[neighbors];
    int read_so_farr[neighbors];
    
    read_count = read_countt;
    read_so_far = read_so_farr;
    
	/* just initialize my segment count to zero */
	for(int i =0; i<MAX_NODES; i++){
		segment_count[i] = 0;
	}

	
	
    
    /* setting up the holders of boolean variables for data and config forwarding */
    bool forward_dat[neighbors];
    bool forward_con[neighbors];
    
    forward_data = forward_dat;
    forward_config = forward_con;
    
    
    time_t time_to_receive_confi[neighbors];
    time_to_receive_config = time_to_receive_confi;
    
    /* creating neighbor out channels and opening for appending*/
    for (int i =0; i< neighbors; i++){
    
    	/* out channels */
    	outstr <<"node"<<my_id<<"to"<<neighbor_id[i];
    	out_channel[i] = outstr.str();        //storing out channels names into array
    	out_file[i].open(out_channel[i].c_str(), ios::app);  //opening them for appending
    	
    	outstr.clear(); outstr.str("");
    	
    	/* initializing boolean variables for out channels */
    	forward_data[i] = false;
    	forward_config[i] = true;
    	
    	/* Also set all read count of all files to 0 */
    	read_count[i] = 0;
    	read_so_far[i] = 0;
    }
    
    sleep(2);   //just to make sure my neighbors finish creating their out channels
    
    /* creating neighbor in  channels and open for reading*/
    for (int i =0; i< neighbors; i++){
    	
    	/* in channels */
    	outstr <<"node"<<neighbor_id[i]<<"to"<<my_id;
    	in_channel[i] = outstr.str();       //storing out channels names into array
    	in_file[i].open(in_channel[i].c_str());  //opening them for reading
    	
    	outstr.clear(); outstr.str("");
    }
 
 	/* lets also create the "node-X-received" file */
    outstr.clear(); outstr.str("");
    outstr<<"node"<<my_id<<"received";
    output.open(outstr.str().c_str(), ios::app);
    outstr.clear(); outstr.str("");
 
 
 	/* =========  RUN SPANNING TREE   ==========*/
	run_stp(diameter);  //the passed integer represent how many times to run it to make it converge	
	sleep(1); // brief wait;
	
   
    /* =========  START, SEND MESSAGE  =========*/
    cout<<"node "<<my_id<<" sending my initial message to node "<<dest<<endl;
    transport_send(my_message, data_type, dest, seq);
    time(&time_to_send_config);  //keep track of when next to send config messages
    
    for(int i =0; i<neighbors; i++){
   		 time(&time_to_receive_config[i]);
   	}
    sleep(2);
    
// lifetime = 60;
    time(&current_time);
    
    while((lifetime_elapsed = difftime(current_time, start_time) <= lifetime)){
    
    	/*  RECEIVE FROM CHANNEL AND PROCESS  */
  		 datalink_receive_from_channel();
  		 

  		 /* CHECK == IF TIME TO SEND CONFIG MESSAGES TO YOUR CHILDREN OR AS a DR */
  		 if(difftime(time(&current_time), time_to_send_config) >= send_config_interval){ 
  			 for(int i =0; i<neighbors; i++){
  			 	if(forward_config[i]){
  					 datalink_receive_from_network(stp_config, stp_len, neighbor_id[i]);
  			 	}
  			 }
  			time(&time_to_send_config); 
  		 }
  		 sleep(0.5);
  		 /* CHECK = IF TIME TO RECEIVE CONFIG MESSAGES FROM PARENT AND DR */
  		 if(my_id != RID){
  		 
  		 	for(int i=0; i<neighbors; i++){
  		 		if(!forward_config[i])
  				 	if(difftime(time(&current_time),time_to_receive_config[i]) >= receive_config_interval){
  				 	
  				 		if(neighbor_id[i] == parent){
  		 					cout<<"node "<<my_id<<" STP PARENT "<<parent<<" DEAD !! RIP , RE-CALCULATING STP !! "<<endl; 
  		 					/* RE-CALCULATE STP */
  		 					run_stp(reconvergence);
  		 					time(&time_to_receive_config[i]); // reset time
  		 				}else{
  		 					/* I am now the DR for that segment */
  		 					cout<<"node "<<my_id<<" I am now the Designated Brides for channel to "<<neighbor_id[i]<<endl;
  		 					forward_config[i] = true;
  		 					forward_data[i] = true;
  		 					time(&time_to_receive_config[i]); // reset time
  		 				
  		 				}
  		 				
  					 }
  			} 
  			 
  		}
  		
  		 sleep(2); 
   		 time(&current_time);
   }
   
/* ============ END OF LIFETIME LOOP ================== */

/* ======= DOCUMENTING ALL RECEIVED COMMUNICATION IN AN "Xreceived" file =====*/
  /* 
    
    
    outstr.clear(); outstr.str("");
    
    for(int i=0; i< neighbors; i++){
    	temp_int = 0;
    	
    	in_file[i].close();
    	in_file[i].open(in_channel[i].c_str());
    	
    	while(!in_file[i].eof()){
    		getline(in_file[i], temp_str);
    		
    		output2<<temp_str<<endl;	
    		temp_int++;
    		if(temp_int > 5000){ //just to identify infinite read error
    			temp_int =0 ;  cout<<"ERROR ERROR INFINITE FILE READ ERROR !!\n";
    			exit(0);
    		}
    	}
    } 
    */
  cout<<"node "<<my_id<<" PEACE OUT!!"<<endl;
  return 0;  
  
  /* ========  END OF PROGRAM ========= */
}

void transport_send(string &my_message, char type, int dest, int seq){
	seq = 0;
	string large_partition = my_message;
	int size = large_partition.size();
	int rem;
	
	if(size > 5){
		if((rem = size%5) != 0){
			seq = (size/5);
		}else{
			seq = (size/5) - 1;
		}
	}
	
	if(size > 5){
		
		string first_partition;
		string second_parition;
		
		while(size > 5){
			
			/* get the first 5 bytes, encapsulate, and send to network layer */
			string first_partition = large_partition.substr(0,5);
			string t_encap = transport_encap(first_partition, data_type, dest, seq);
    		int t_len = t_encap.size();
   			network_receive_from_transport(t_encap, dest, t_len);
			
			
			/* get second partition, and make it go through the while loop check */
			string second_partition = large_partition.substr(5, size - 5);
			large_partition = second_partition;
			size = large_partition.size();
			seq--;
		}
		
	}
	
	if((size > 0 ) && (size < 6)){
	
   		 string t_encap = transport_encap(large_partition, data_type, dest, seq);
   		 int t_len = t_encap.size();
    
    	/* pushes the transport segment to the network layer */
    	network_receive_from_transport(t_encap, dest, t_len);
    	
    	
    }
}



string transport_encap(string &my_message, char type, int dest, int seq){
	outstr.clear(); outstr.str(""); 
	
	if(seq > 9){
		outstr<<type<<my_id<<dest<<seq<<my_message;   // 2 bytes complete
	}else{
		outstr<<type<<my_id<<dest<<'0'<<seq<<my_message;   // pad with leading 0
	}
	
	string t_encap = outstr.str();
	return t_encap;
}

void network_receive_from_transport(string &t_data, int dest, int t_len){
	outstr.clear(); outstr.str("");
	
	char type = t_data.at(0);  // extract the message type
	bool dest_next_hop = false;
	
	/* see if destination is a neighbor, unicast the packet to him */
	for(int i=0; i<neighbors; i++){
		if(dest == neighbor_id[i]){
			next_hop = dest;
			dest_next_hop = true;
			break;
		}
	}
	
	/* if destination is not a neighbor */
	if(!dest_next_hop){
		next_hop = broadcast;   //the datalink will determine which ports are applicable to this forwarding
		
	}
	outstr<<type<<my_id<<dest<<t_data;   //encapsulation the transport data
	string n_encap = outstr.str();
	int n_len = n_encap.size();
	
	/* pushes the network packet to the datalink */
	datalink_receive_from_network(n_encap, n_len, next_hop );
}

void datalink_receive_from_network(string &n_data, int n_len, int next_hop){
	outstr.clear(); outstr.str("");
	
	char type = n_data.at(0);
	int source_addr = n_data.at(1);
	
	int f_len = n_len + 5;
	if(f_len > 9){
		outstr<<SOH<<f_len<<n_data;
	}else{                  
		outstr<<SOH<<'0'<<f_len<<n_data;
	}
	
	string frame = outstr.str(); 
	int checksum = 0;
	
	//calculating the checksum
	for(int i=0; i<frame.size(); i++){
		checksum += frame.at(i);
	}
	checksum = checksum%100;
	
	//making sure the checksum is 2 bytes
	if(checksum > 9){
		outstr<<checksum;
	}else{
		outstr<<'0'<<checksum;
	}
	frame = outstr.str();  //new frame
	int frame_len = frame.size();
	
	/* forward frames according to port states*/
	if(type == data_type){   // if data messages
		
		/* if next_hop is a single directly connected (neighbor), just unicast to him alone */
		for(int i=0; i< neighbors; i++){
			if(next_hop == neighbor_id[i]){
    			out_file[i]<<frame<<endl;
    			return;
    		}
    	}
		/* if next hop is not a neighbor, broadcast to forwarding data ports, parents or children (excluding origin) */
		for(int i=0; i< neighbors; i++){
			if(forward_data[i] &&  (source_addr != neighbor_id[i])){  //forward ports but excluding origin
    			out_file[i]<<frame<<endl;
    		}
    	}
	
	}else if(type == config_type){
		for(int i=0; i< neighbors; i++){
	
			if(forward_config[i]){
    			out_file[i]<<frame<<endl;
    		}
   	    }
	}else{
		cout<<"ERROR !! ERROR !! INVALID MESSAGE TYPE!! BIG ERROR !! CANNOT HAPPEN \n";
		exit(1);
	}
}

void datalink_receive_from_channel(){
	string temp_str;
	int loop = 0;
    for(int x=0; x< neighbors; x++){
    	bool dont_sleep = false;  //to give preference to those having to read from multiple channels
    	in_file[x].close();
    	in_file[x].open(in_channel[x].c_str());
    	loop = 0;
    	while(!in_file[x].eof()){
    		loop++;
    		temp_str = "";
    		getline(in_file[x], temp_str);
    		if(loop >= read_count[x]){
    			if(temp_str.size() > 1){ //making sure it is a valid getline read, not an empty line
    				dont_sleep = true;
    				
    				/* Decapsulate and send to network layer */
    				string frame_decap = datalink_decap(temp_str);
    				//cout<<"node "<<my_id<<" received frame_decaped "<<frame_decap<<" from "<<neighbor_id[x]<<endl;
    				network_receive_from_datalink(frame_decap, x );
    				
    			}else{
    				if(dont_sleep == false){ //if you have not been initially been reading, then sleep for not reading anything
    					 //cout<<"node "<<my_id<<" sleeping for not reading anything"<<endl;
    					 sleep(1);
    				}
    			}
    		}
    		if(loop > 5000){ //just to identify infinite read error loops
    			loop =0 ;  cout<<"node "<<my_id<<" ERROR ERROR INFINITE FILE READ ERROR !! \n";
    			exit(1);
    		}
    	}
    	read_count[x] = loop;
    	read_so_far[x] = loop - 1;   //to keep track of where to begin reading next time.
    }
}

string datalink_decap(string frame){
	int size = frame.size();
    int len = size - 5;  
    string frame_decap = frame.substr(3,len);

	return frame_decap;
}

void network_receive_from_datalink(string net_packet, int source_link){

	/* check if message is a STP config message */
	if(net_packet.at(0) == config_type){
		time(&time_to_receive_config[source_link]);
		decide_stp(net_packet, source_link);
	
	/* check if message is a data message */
	}else if(net_packet.at(0) == data_type){
		
		int message_source = net_packet.at(1) - '0';
		int message_dest = net_packet.at(2) - '0';
	
		
		/* if the message is destined to me */
		if(message_dest == my_id){
			/* and message originated from a neighbor */
			if(message_source == neighbor_id[source_link]){
				string net_decap = network_decap(net_packet); //decapsulated the network packet
				
				cout<<"node "<<my_id<<" net-decap "<<net_decap<<endl;
				transport_receive_from_network(net_decap);  //forward transport packet to transport layer
				return;
			}else{ //if not originated from neighbor
				/* then lets make sure it is not being forwarded or routed into a blocked port for data messages */
				if(!forward_data[source_link]){ //the channel is an STP blocked channel, DROP PACKET
					return;
				}else{
					/* receive packet, decapsulate ==> forward to transport layer */
					string net_decap = network_decap(net_packet); //decapsulated the packet
					transport_receive_from_network(net_decap);  //forward transport packet to transport layer
				}
			}
		
		}else{ 
		/* the message is not destined to me, has to routed if possible */
			/* DONT ROUTE PACKET IF THE SOURCE LINK IS BLOCKED BY STP */
			if(!forward_data[source_link]){
				return;
			
			}
			for(int i=0; i<neighbors; i++){
				/* if any port is not blocked, and the port is not the originated port => forward */
				if(forward_data[i] && (neighbor_id[source_link] != neighbor_id[i])){
					/* forward out of those applicable port */
					/* this route is unicast so the datalink to route it back to the source */
					datalink_receive_from_network(net_packet, net_packet.size(), neighbor_id[i] );
				}
			}
			return;
		}
	}else{
		cout<<"node "<<my_id<<" ERROR !! ERROR !! INVALID NETWORK MESSAGE TYPE!! BIG ERROR !! CANNOT HAPPEN\n";
		exit(1);
	}
}

void decide_stp(string config_packet, int source_mac){
	int your_RID = config_packet.at(1) - '0';
	int SID = neighbor_id[source_mac];
	int your_hop = atoi(config_packet.substr(2,2).c_str());
	int previous_parent;
	
	
	/* if I receive from my parent, I have no choice, but to update accordingly */
	if(SID == parent){
		
		if((your_hop == (hop_count - 1)) && (RID == your_RID)){
			/* then there was no change */
			return;
		}else{
			/* else there was a change that has to be updated */
			RID = your_RID;
			hop_count = your_hop + 1;
			event = true;  //event = true, means i have to tell others of the change
			//cout<<"your hop "<<your_hop<<" my hop "<<hop_count<<" your RID "<<your_RID<<" my RID "<<RID<<endl;
			cout<<"node "<<my_id<<" updating STP, RID: "<<RID<<" Parent: "<<parent<<" new Hop: "<<hop_count<<endl;
			return;
		}
	}

	
	if(your_RID < RID){
	
	/* This means that you will be my parent*/
		event = true;
		if(SID == parent){  // just update your info and return
		
			RID = your_RID;
			hop_count = your_hop + 1;
			return;
		}
		/* what if you are not my parent */
		/* get information of my previous parent, in order to change their status */
		if(parent >= 0){ //making sure i had a previous parent, since parent was initially -1
			previous_parent = parent;
			for(int i =0; i< neighbors; i++){
				if(neighbor_id[i] == previous_parent){
					forward_data[i] = false;          // set previous parent data forwarding to false
					forward_config[i] = false;
				}
			}
		}
		/* update new parent */
		RID = your_RID;
		parent = SID;
		hop_count = your_hop + 1;
		cout<<"node "<<my_id<<" updating STP, RID: "<<RID<<" new Parent: "<<parent<<" Hop: "<<hop_count<<endl;
		
		forward_config[source_mac] = false;  // I am not going to send config to this node
		forward_data[source_mac] = true;     // I will send data to this node
		return;
		
	}else if(your_RID == RID){
	
	/* If you are previously my parent, then i update my information according to you */
		
		if(SID == parent){  // just update information with my parents
		
			if(hop_count != your_hop + 1){
				event = true;
				hop_count = your_hop + 1;
				cout<<"node "<<my_id<<" updating STP, RID: "<<your_RID<<" Parent: "<<parent<<" new Hop: "<<hop_count<<endl;
				forward_config[source_mac] = false;
				forward_data[source_mac] = true;
				return;
			}else{
				event = false;
				return;
			}
		}
		
		/* you may have a better hop count than my parent*/
		if(your_hop + 1 < hop_count){  //if hop count is better, make you my parent
			event = true;
		/* get information of my previous parent, in order to change their status */
			if(parent >= 0){ //making sure i had a previous parent, since parent was initially -1
				previous_parent = parent;
				for(int i =0; i< neighbors; i++){
					if(neighbor_id[i] == previous_parent){
						forward_data[i] = false;          // set previous parent data forwarding to false
						forward_config[i] = false;
					}
				}
			}
			
			/* update new parent */
			parent = SID;
			hop_count = your_hop + 1;
			cout<<"node "<<my_id<<" updating STP, RID: "<<your_RID<<" new Parent: "<<parent<<" Hop: "<<hop_count<<endl;
			forward_config[source_mac] = false;   // I am not going to send config to this node
			forward_data[source_mac] = true;      // I will send data to this node
			return;
			
		}else if(hop_count == your_hop){
		/* Who becomes the designated bridge */
		if(stp_mode){
			cout<<"node "<<my_id<<" Deciding Designated Bridge with node "<<SID<<", My Hop= "<<hop_count<<" Your Hop= "<<your_hop<<endl;
		}	
			if(my_id < SID){
				forward_config[source_mac] = true;
				forward_data[source_mac] = false;
			}else if(my_id > SID){
				forward_config[source_mac] = false;
				forward_data[source_mac] = false;
			}
			
		}else if(hop_count < your_hop){ //if i have a hop count better than you, i can forward you data and config
			forward_config[source_mac] = true;
			forward_data[source_mac] = true;
			
		}else if(your_hop < hop_count){
			forward_config[source_mac] = false;
			forward_data[source_mac] = false;
			
		}
	}else if(RID < your_RID){
		forward_config[source_mac] = true;
		forward_data[source_mac] = true;
		
	}
	if(RID == my_id){  //if iam the root, then all my ports should be forwarding everything
	
		for(int i =0; i<neighbors; i++){
			forward_config[source_mac] = true;
			forward_data[source_mac] = true;
		}
		event = false;  
	}
	
}

string network_decap(string packet){
	int size = packet.size();
	string n_decap = packet.substr(3,size - 3);
	return n_decap;
}

void transport_receive_from_network(string trans_segment){
	
	int size = trans_segment.size();
	char type = trans_segment.at(0);
	int seq = atoi(trans_segment.substr(3,2).c_str());
	if(type == n_ack){
		cout<<"node "<<my_id<<" THIS PLACE WILL NEVER BE REACHED, AT LEAST NOT IN THIS PROJECT \n";
		int seq = 1;
   		transport_send(my_message, data_type, dest, seq);
	}else{
		
		int source = trans_segment.at(1) - '0';
		string received_message = trans_segment.substr(5, size - 5);
		int destination = trans_segment.at(2) - '0';
		
		if(destination != my_id){
			cout<<"node "<<my_id<<"ERROR !! WRONG RECIPIENT OF MESSAGE IN TRANSPORT LAYER, ERROR !! THIS CANNOT HAPPEN \n";
			exit(1);
		}
		cout<<"node "<<my_id<<" received <"<<received_message<<"["<<seq<<"]"<<"> from node "<<source<<endl;
		
	/* accept the message and store in buffer*/
		segment_buffer[source][seq] = received_message;
		segment_count[source]++;
		
		/* if sequence number is 0, then complete message received, write the message to file */
		if(seq == 0){
			output<<"from "<<source<<" :";
			for(int i = segment_count[source] - 1; i >= 0 ; i--){
				outstr.clear(); outstr.str("");
    			outstr<<segment_buffer[source][i];
    			output<<outstr.str().c_str();
    			outstr.clear(); outstr.str("");
			}
			output<<endl;
			outstr.clear(); outstr.str("");
		}
		
		
		
	






    	//outstr.clear(); outstr.str("");
    	//outstr<<"from "<<source<<": "<<segment_buffer[source][seq]<<" "<<seq;
    	//string temp_string = outstr.str().c_str();
    	//output<<temp_string<<endl;
	}
}

void run_stp(int how_many_times){ 
	stp_mode = true;
	cout<<"node "<<my_id<<" Running.. SPANNING TREE PROTOCOL "<<endl;
	time(&stp_start);
	double seconds;
	int multiplier = how_many_times;
	event = true;

	for(int i=0; i<neighbors; i++){
		forward_data[i] = false;
    	forward_config[i] = true;
	}
	RID = my_id;
	hop_count = 0;
	parent = -1;
	
	while( how_many_times > 0){
		how_many_times--;
	/* To forward STP config messages, and process */
		outstr.clear(); outstr.str("");

	/* Construct the STP config message */
		if(hop_count < 10){
			outstr<<config_type<<RID<<'0'<<hop_count;  //pad with leading 0, since it must be 2 bytes
		}else{
			outstr<<config_type<<RID<<hop_count;  //normal encapsulation without leading 0
		}
		stp_config = outstr.str();
		stp_len = stp_config.size();

	
	if(event == true){  //if you made a change , you forward new info, if not dont forward already forwarded info
	/* forward STP config messages */
		next_hop = broadcast; //I am using 99 to represent a broadcast address for the config message
		datalink_receive_from_network(stp_config, stp_len, next_hop);
		event = false;
		sleep(1);  // just to make sure everybody has sent their message //very important !!
   	 	
	}else{
		//cout<<"node "<<my_id<<" sleeping 1s for not having event"<<endl;
		sleep(1); //wait for before another read, perhaps new info
	}
	
    /* RECEIVE CONFIG MESSAGES AND PROCESS */
   		 datalink_receive_from_channel();
   		 
    }
    
    	// * A RULE TO ENFORCE THE TIME BOUND OF AN STP COMPUTATION * //
    	while(true){
    		time(&current_time);
    		seconds = difftime(current_time, stp_start);
  			
    		if(seconds > (4 * multiplier + 1)){
    			break;
    		}else{
    			cout<<"node "<<my_id<<" time remaining to converge "<<((4*multiplier)-seconds)<<"s"<<endl;
    			sleep(1);
    			datalink_receive_from_channel();
    		}
    	}
    	
   		 int neighbor;
   		 cout<<"node "<<my_id<<" RID: "<<RID<<" Parent: "<<parent<<" Hop Count: "<<hop_count<<endl;
   		 for(int i =0; i< neighbors; i++){
    	
    		neighbor = neighbor_id[i];
    		if(!forward_data[i]){
    			cout<<"Node "<<my_id<<" DATA port to "<<neighbor<<" BLOCKED\n";
    		}else{
    			cout<<"Node "<<my_id<<" DATA port to "<<neighbor<<" OPEN\n";
    		}
    		if(!forward_config[i]){
    			cout<<"Node "<<my_id<<" CONFIG port to "<<neighbor<<" BLOCKED\n";;
    		}else{
    			cout<<"Node "<<my_id<<" CONFIG port to "<<neighbor<<" OPEN\n";
    		}
    	}
    	cout<<"node "<<my_id<<" STP CONVERGED !!, lets get this money ! "<<endl;
    	stp_mode = false;
}











