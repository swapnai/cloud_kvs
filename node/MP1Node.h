/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5
#define TGOSSIP 0
/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */
enum MsgTypes{
    JOINREQ,
    JOINREP,
    GOSSIP,
    DUMMYLASTMSGTYPE
};

/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct MessageHdr {
	enum MsgTypes msgType;
}MessageHdr;

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
	EmulNet *emulNet;
	Log *log;
	Params *par;
	Member *memberNode;
	char NULLADDR[6];

	//helper methods
	bool isSameAddress(MemberListEntry first, MemberListEntry next);
	size_t packMemberList(char* &msg, Address* addr); 
	vector<MemberListEntry> unpackMemberList(char* data); 
	void print(MemberListEntry entry);
	void processListEntry(MemberListEntry entry);
	void checkMessages();
	static int enqueueWrapper(void *env, char *buff, int size);
	int initThisNode(Address *joinaddr);
	int introduceSelfToGroup(Address *joinAddress);
	bool recvCallBack(void *env, char *data, int size);
	void sendGossip(Address *addr);
	void sendJoinRep(Address *addr);
	void processJoinReq(Address *toAddr, long hb);
	void updateEntries(char* data);
	void nodeLoopOps();
	void removeStaleEntries();
	void markFailedEntries();
	void gossip();
	int isNullAddress(Address *addr);
	Address getJoinAddress();
	void printAddress(Address *addr);
	Address getAddress(MemberListEntry entry);

public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
	Member * getMemberNode() {
		return memberNode;
	}
	int finishUpThisNode();
	int recvLoop();
	void nodeStart(char *servaddrstr, short serverport);
	void nodeLoop();
	// static functions made public for testing- TBD
	static size_t serialize(vector<MemberListEntry> ML, 
				char* &msg, Address* addr); 

	virtual ~MP1Node();
};

#endif /* _MP1NODE_H_ */
