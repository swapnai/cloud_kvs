/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 *              Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

void MP1Node::processJoinReq(Address *fromAddr, long hb) {
  //values dereferenced from toAddr
  int id = *(int *)(&(fromAddr->addr));
  short port = *(short *)(&(fromAddr->addr[4]));
  bool found = false;
  // can you have duplicate JOINREQ??? TBD
    for(auto entry : memberNode->memberList) {
    if(entry.getid() == id && entry.getport() == port) {
      found = true;
      break;
    }
  } 
  if(!found){
    MemberListEntry newEntry(id, port, hb, par->getcurrtime());
    cout << "Joined(3): rep in joinrep: "; print(newEntry); cout << endl; 
    // add this member to membership list
    this->memberNode->memberList.push_back(newEntry);
#ifdef DEBUGLOG
    log->logNodeAdd(&memberNode->addr, fromAddr);
#endif

  }
  //send fixed list in reply
  sendJoinRep(fromAddr);
} 

void MP1Node::updateEntries(char* data) {
  cout <<"UPDATE ENTRIES.." << endl;
  vector<MemberListEntry> newList = unpackMemberList(data);
  for(auto newEntry : newList) {
    processListEntry(newEntry);
  }
}

bool MP1Node::isSameAddress( MemberListEntry first, MemberListEntry next) {

  if(first.getid() == next.getid() && first.getport() == next.getport())
    return true;
  else 
    return false;
}

void MP1Node::print(MemberListEntry entry) {
  cout <<"Entry: " << entry.getid() << ":" << entry.getport() << ":" << entry.getheartbeat() << ":" << entry.gettimestamp() << endl;

}

Address MP1Node::getAddress(MemberListEntry entry) {
  return Address(to_string(entry.getid())+":"+to_string(entry.getport()));
}

// This function implements Push-based GOSSIP
void MP1Node::processListEntry(MemberListEntry entry) {

  //update entries in ML with newer heartbeat
  for(auto &currEntry : memberNode->memberList) {
    
    if(!isSameAddress(currEntry, entry)) {
      continue;
    }
    //update ML with failed entries first
    if(entry.getheartbeat() == -1) {
      currEntry.setheartbeat(-1);
      return;
    }
    //ignore already failes entries in ML
    if(currEntry.getheartbeat() == -1) return;

    //if we have a higher hb, update ML entry to match
    if (currEntry.getheartbeat() < entry.getheartbeat()) {
      currEntry.setheartbeat(entry.getheartbeat());
      currEntry.settimestamp(par->getcurrtime());
      return;
    }
    //no update needed for this ML entry
    return;
  } 
  //do not re-add failed entries
  if(entry.getheartbeat() == -1)
    return;
  //if a new entry is found, add it to ML
  memberNode->memberList.push_back(entry);
  cout << "Joined: Added entry: "; print(entry); cout << endl;
#ifdef DEBUGLOG
  Address addr = getAddress(entry);
  log->logNodeAdd(&memberNode->addr, &addr);
#endif
  
}

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->heartbeat = 0;
	memberNode->memberList.clear();
	MemberListEntry item(id, port, 0, par->getcurrtime());
	memberNode->memberList.push_back(item); 
#ifdef DEBUGLOG
	cout << "Joined(1): InitMemberList" << endl;
	Address addr = getAddress(item);
	log->logNodeAdd(&memberNode->addr, &addr);
#endif
	memberNode->myPos = memberNode->memberList.begin();
    return 0;
}
vector<MemberListEntry> MP1Node::unpackMemberList(char* data) {
  int count= *(int *)(data +sizeof(MessageHdr) + sizeof(Address));
  data = data + sizeof(MessageHdr) + sizeof(Address) + sizeof(int);
  cout << "Unpack sz: " << count << endl;
  vector<MemberListEntry> newList;
  size_t entrySz = sizeof(int)+sizeof(short)+sizeof(long)*2;
  for(int i = 0; i < count; ++i, data += entrySz) {
    int idd = *(int *)(data);
    short port = *(short *)(data +sizeof(int));
    long hb = *(long *)(data +sizeof(int)+sizeof(short));
    long ts = *(long *)(data +sizeof(int)+sizeof(short)+sizeof(long));
    MemberListEntry item(idd, port, hb, ts); 
    newList.push_back(item);
    cout << item.getid() <<":" <<item.getport() <<":"<<item.getheartbeat()<<":"<< item.gettimestamp() << endl;

  }
  return newList;

}

/*
 * Seralize the Gossip table to this format:
 * MSG FORMAT: 
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * + Msg |  To  |  No of |                        ||
 * +  Hdr|  Addr| Members+-> id,  port,   hb,   ts|| id, port
 * +     |      | In List|  int, short, long, long||
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Callee to free(msg)
 *
 */

size_t MP1Node::serialize(vector<MemberListEntry> ML, 
			  char* &msg, Address* addr) {

  size_t msgSz = sizeof(MessageHdr) + sizeof(Address);
  //now add size of all entries
  int len = ML.size();
  size_t entrySz = sizeof(int)+sizeof(short)+sizeof(long) * 2;
  //count, entries
  msgSz += sizeof(int) + len * entrySz;

  //callee needs to free this memory
  msg = (char *) malloc(msgSz * sizeof(char));

  char* cP = msg;
  cP = (cP + sizeof(MessageHdr));
  memcpy((char *)(cP), addr, sizeof(Address));
  cP = (char *)(cP+sizeof(Address));
  memcpy((char *)(cP), &len, sizeof(int));
  cP = cP+sizeof(int);

  for(auto item: ML) {
    memcpy((char *)(cP), &(item.id), sizeof(int));
    cP += sizeof(int);

    memcpy((char *)(cP), &(item.port), sizeof(short));
    cP += sizeof(short);

    memcpy((char *)(cP), &(item.heartbeat), sizeof(long));
    cP += sizeof(long);

    memcpy((char *)(cP), &(item.timestamp), sizeof(long));
    cP += sizeof(long);

  }
  return msgSz;
}

/*
 * Pack the Member List into a char buffer
 * Callee needs to free the buffer thus:
 *                   free(msg)
 *
   * MSG FORMAT: 
   * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   * + Msg |  To  |  No of |                        ||
   * +  Hdr|  Addr| Members+-> id,  port,   hb,   ts|| id, port
   * +     |      | In List|  int, short, long, long||
   * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   */
size_t MP1Node::packMemberList(char* &msg, Address* addr) {

  return serialize(memberNode->memberList, msg, addr);

}

void MP1Node::sendGossip(Address* addr) {
  char* msg = NULL;
  size_t msgsize = packMemberList(msg, addr);
  ((MessageHdr *)(msg))->msgType = GOSSIP;

  // send GOSSIP message to newly joined member
    emulNet->ENsend(&this->memberNode->addr, addr, (char *)(msg), msgsize);
    free(msg);
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
#ifdef DEBUGLOG
    static char s[1024];
#endif
    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
      //update ML entry for self
      memberNode->memberList[0].settimestamp(par->getcurrtime());
      // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
      log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        size_t msgsize = sizeof(MessageHdr) + sizeof(Address) + sizeof(long);
        char *msg = (char *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
	// does -> have higher precedence than *, TBD
        ((MessageHdr *)msg)->msgType = JOINREQ;
	char *cP = (char *)msg;
        memcpy((char *)(cP+sizeof(MessageHdr)), &memberNode->addr, sizeof(Address));
        memcpy((char *)(cP+sizeof(MessageHdr)+sizeof(Address)), &memberNode->heartbeat, sizeof(long));
#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);
        free(msg);

    }
    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
  return 0;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();
    cout << "done with message... " << endl;
    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr = NULL;
    int size = 0;
    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);

    }
    //    cout << "EXIT checkMessages:" << endl;
    return;
}

void MP1Node::sendJoinRep(Address* addr) {
  char *msg = NULL;
  size_t msgsize = packMemberList(msg, addr);
  // create JOINREP message: 
  ((MessageHdr *)(msg))->msgType = JOINREP;
  // send JOINREP message to newly joined member
  emulNet->ENsend(&memberNode->addr, addr, (msg), msgsize);
  free(msg);
}


/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
  /* JOINREQ MSG FORMAT: 
   * +++++++++++++++++++++++++++++++++
   * + Msg  | From    |Heart | 
   * +  Hdr |  Address| Beat | 
   * +++++++++++++++++++++++++++++++++*/
  Address *fromAddr =(Address *)(data + sizeof(MessageHdr));
  int type = ((MessageHdr *)(data))->msgType;

  switch(type) {
  case JOINREQ:
    if(memberNode->addr == getJoinAddress()) {
      long hb = *(long *)(data + sizeof(MessageHdr) + sizeof(Address));
      processJoinReq(fromAddr, hb);
    }
    break;

  case JOINREP:
    memberNode->inGroup = true;
    updateEntries(data);
    break;
   
  case GOSSIP:
    updateEntries(data);
    break;

  default:  
    cout << "ERROR: INVALID msg TYPE !!" << endl;
    break;
  }
  return true;
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a 
 *              timeout period and then delete the nodes
 * Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

  removeStaleEntries();
  markFailedEntries();
  gossip();
}

void MP1Node::removeStaleEntries() {
  cout << "Enter REMOVE...." << endl;
  // clean up stale entries.
  for(auto currEntry = begin(memberNode->memberList); currEntry != end(memberNode->memberList);) {
    //if failed and TREMOVE time passed since last HB... remove!
    if(par->getcurrtime() - currEntry->gettimestamp() > TREMOVE) {
      cout << "REMOVE: "; print(*currEntry); cout << endl;
      cout << "TIME:" << par->getcurrtime() << ":" << currEntry->gettimestamp() << endl; 
      
#ifdef DEBUGLOG
      Address addr = getAddress(*currEntry);
      log->logNodeRemove(&memberNode->addr, &addr);
#endif
      currEntry = memberNode->memberList.erase(currEntry);
    } else 
      ++currEntry;
  } 
}



void MP1Node::markFailedEntries() {
  for(auto currEntry = begin(memberNode->memberList); 
      currEntry != end(memberNode->memberList); ++currEntry) {
    
    if(par->getcurrtime() - currEntry->gettimestamp() > TFAIL) {
      cout << "FAILED: "; print(*currEntry); cout << endl;
      currEntry->setheartbeat(-1);  
    }
  }
}

void MP1Node::gossip() {

  if(memberNode->bFailed) return;
  //update this node's timestamp and heartbeat
  memberNode->memberList[0].setheartbeat(
		     memberNode->memberList[0].getheartbeat() + 1);
  memberNode->memberList[0].settimestamp(par->getcurrtime());
  cout << "HB updated: "; print(memberNode->memberList[0]); cout << endl;
  //gossip to 3 random neighbors
  size_t len = memberNode->memberList.size();
  if(len <= 0) {
    cout << "EMPTY ML list!!!! " << endl;
    return;
  }
  cout << "ML SZ @ GOSSIP: " << len << endl;
  for(int i = 0; i < 3; i++) {
    MemberListEntry entry = memberNode->memberList[rand() % len];
    if(!isSameAddress(entry, memberNode->memberList[0])) {
      
      Address addr = getAddress(entry);
      cout << "GOSSIP FROM: "; print(memberNode->memberList[0]); cout << " TO: "; print(entry);
      cout << endl;
      sendGossip(&addr);
    } 
  }

} 

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
