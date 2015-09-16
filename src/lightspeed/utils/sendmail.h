/*
 * sendmail.h
 *
 *  Created on: 26.5.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_SENDMAIL_H_
#define LIGHTSPEED_UTILS_SENDMAIL_H_
#include "../base/containers/constStr.h"
#include "../base/streams/fileio.h"
#include "../base/streams/memfile.h"
#include "../base/containers/autoArray.h"
#include "../base/memory/smallAlloc.h"
#include "../base/streams/netio.h"
#include "../base/exceptions/genexcept.h"

namespace LightSpeed {

///Object keeps connection to MTA server.
/**You have to create this object before message can be sent to the server. There
 * should be one connection per server, but is allowed to have multiple instances of
 * SendMail objects per connection
 *
 * @note Connection have to be created to the local MTA server. Application that uses
 * SendMail and SendMailConnection classes depends on such MTA server. You have to install
 * some if there is none in current local network.
 */
class SendMailConnection: public NetworkStream<> {
public:
	///Initializes connection
	/**
	 * @param addr domain name of MTA. It can contain port number after colon :
	 * @param port port number. If domain does contain port number, this field is ignored
	 * @param timeout timeout for all I/O operations in milliseconds, default is 30 seconds
	 */
	SendMailConnection(ConstStrA addr, natural port, ConstStrA heloText, natural timeout=30000);
	///Initializes connection
	/**
	 * @param addr address specified using NetworkAddress object
	 * @param timeout timeout for all I/O operations in milliseconds, default is 30 seconds
	 */
	SendMailConnection(NetworkAddress addr, ConstStrA heloText, natural timeout=30000);
protected:
	void welcome(ConstStrA heloText);

};

///Prepared message with feature to be send using SendMailConnecton
/**
 * Object is used to prepare receivers and message body. After message is prepared, it can be send using
 * SendMailConnection object. You can create multiple SendMail object per connection
 */
class SendMail {
public:


	///Create simple mail message
	/**
	 * Message can be send immediately
	 *
	 * @param sender sender e-mail
	 * @param receiver receiver e-mail. Function doesn't support multiple receivers (separate senders by semicolons will not help)
	 * @param textMsg text of message INCLUDING ALL REQUIRED HEADERS such a From: Subject: Cc: fields, content type and encoding defintions.
	 *     Message should not contain line with dot. Object will replace this dot with space.
	 */
	SendMail(ConstStrA sender, ConstStrA receiver, ConstStrA textMsg);

	///Create mail message which can be edited and send multiple times
	/**
	 * @param sender sender's e-mail
	 */
	SendMail(ConstStrA sender);

	///Adds receiver
	/**
	 * @param receiver receiver's e-mail. You can specify multiple receivers. Message with no receivers
	 * cannot be sent.
	 */
	void addReceiver(ConstStrA receiver);

	///Opens message body
	/**
	 * @param create true to create message, otherwise new text is appended to existing body
	 * @return stream object
	 */
	SeqFileOutput openMessage(bool create);

	///Only message will be reset
	static const natural resetMessage = 1;
	///Only receivers will be reset
	static const natural resetReceivers = 2;

	///Sends message using connection
	/**
	 * @param stream opened network connection to the MTA
	 * @param resetMode how message will be reset after succesful send
	 * @exception SendMailError describes an issue happened during sending
	 */
	void send(SendMailConnection &stream, natural resetMode = resetMessage | resetReceivers);
	///resets message without sending
	/**
	 * @param reseMode how message will be reset
	 */
	void reset(natural reseMode = resetMessage | resetReceivers);
	///Inline send
	/**
	 * Creates message and sends it without need to create intermediate object
	 * @param stream connected stream
	 * @param sender sender e-mail
	 * @param receiver receiver e-mail. Function doesn't support multiple receivers
	 * @param textMsg body of message
	 */
	static void send(SendMailConnection &stream, ConstStrA sender, ConstStrA receiver, ConstStrA textMsg);

	static const char *errorMsg;


protected:
	MemFile<> message;
	StringA sender;
	AutoArray<StringA, SmallAlloc<32> > receivers;


};

typedef GenException2<SendMail::errorMsg, StringA, StringA> SendMailError;

}


#endif /* LIGHTSPEED_UTILS_SENDMAIL_H_ */
