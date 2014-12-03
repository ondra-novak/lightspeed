/*
 * sendmail.cpp
 *
 *  Created on: 26.5.2013
 *      Author: ondra
 */

#include "sendmail.h"
#include "../base/text/textstream.tcc"
#include "../base/streams/memfile.tcc"
#include "../base/containers/autoArray.tcc"
#include "../base/streams/fileiobuff.tcc"


namespace LightSpeed {




LightSpeed::SendMail::SendMail(ConstStrA sender, ConstStrA receiver, ConstStrA textMsg)
	:sender(sender) {
	message.setStaticObj();
	SeqTextOutA txtout(openMessage(true));
	txtout.blockWrite(textMsg,true);
	addReceiver(receiver);

}

LightSpeed::SendMail::SendMail(ConstStrA sender):sender(sender) {
	message.setStaticObj();
}

void LightSpeed::SendMail::addReceiver(ConstStrA receiver) {
	receivers.add(receiver);
}

SeqFileOutput LightSpeed::SendMail::openMessage(bool create) {
	if (create) message.clear();
	return SeqFileOutput(&message);
}

static void handleError(ScanTextA &netscan,PrintTextA &netprint, ConstStrA desc) {
	if (netscan("%1\n%")) {
		netprint("rset\n");
		StringA msg = netscan[1].str();
		throw SendMailError(THISLOCATION,desc,msg);
	} else {
		throw SendMailError(THISLOCATION,desc,"unexpected reply");
	}
}

void LightSpeed::SendMail::send(SendMailConnection &stream, natural resetMode) {

	const char *state = "Init error";

	try {
		if (receivers.empty()) throw SendMailError(THISLOCATION,"No receivers","");
		byte criticalSequence[5] = {'\r','\n','.','\r','\n'};
		ConstStringT<byte> cseqbts(criticalSequence,5);

		AutoArray<byte> &buffer = message.getBuffer();
		natural k = buffer.find(cseqbts);
		while (k != naturalNull) {
			buffer(k+2) = ' ';
			k = buffer.find(cseqbts);
		}

		PrintTextA netprint(stream);
		ScanTextA netscan(stream);
		netprint.setNL("\r\n");
		netscan.setNL("\r\n");
		state = "Sender error";
		netprint("mail from: %1\n") << sender;
		if (!netscan("250 %0\n%")) {
			handleError(netscan,netprint,state);
		}
		state = "Receiver error";
		for (natural i = 0; i < receivers.length(); i++) {
			netprint("rcpt to: %1\n") << receivers[i];
			if (!netscan("250 %0\n%")) {
				handleError(netscan,netprint,state);
			}
		}
		state = "Error while creating message";
		netprint("data\n");
		if (!netscan("354 %0\n%")) {
				handleError(netscan,netprint, state);
		}
		state = "Error while sending message";
		AutoArray<byte>::Iterator iter(buffer.getFwIter());
		bool wascr = false;
		while (iter.hasItems()) {
			byte c = iter.getNext();
			if (c =='\r') wascr = true;
			else if (c == '\n') {
				if (!wascr)
					stream.write('\r');
				else
					wascr = false;
			} else {
				if (wascr)
					stream.write('\n');
				wascr = false;
			}
			stream.write(c);
		}
		netprint("\n.\n");
		if (!netscan("250 %0\n%")) {
				handleError(netscan,netprint, state);
		}
	} catch (SendMailError &e) {
		throw;
	} catch (Exception &e) {
		throw SendMailError(THISLOCATION,state,"Network I/O error") << e;
	}
	reset(resetMode);

}

void LightSpeed::SendMail::reset(natural resetMode) {
	if (resetMode & resetMessage) message.clear();
	if (resetMode & resetReceivers) receivers.clear();
}

void SendMail::send(SendMailConnection& stream, ConstStrA sender,
		ConstStrA receiver, ConstStrA textMsg) {
	SendMail(sender,receiver,textMsg).send(stream);
}

SendMailConnection::SendMailConnection(ConstStrA addr, natural port, ConstStrA heloText, natural timeout)
	:NetworkStream<>(connect(NetworkAddress(addr,port),timeout))
{
	welcome(heloText);
}

SendMailConnection::SendMailConnection(NetworkAddress addr, ConstStrA heloText, natural timeout)
	:NetworkStream<>(connect(addr,timeout))
{
	welcome(heloText);
}

void SendMailConnection::welcome(ConstStrA heloText) {
	try {
		ScanTextA netscan(*this);
		netscan.setNL("\r\n");
		PrintTextA netprint(*this);
		netprint.setNL("\r\n");

		if (!netscan("220 %0\n%")) {
			handleError(netscan,netprint,"Negotiation error");
		}
		netprint("helo %1\n") << heloText;
		if (!netscan("250 %0\n%")) {
			handleError(netscan,netprint,"helo failed");
		}

	} catch (SendMailError &e) {
		throw;
	} catch (Exception &e) {
		throw SendMailError(THISLOCATION,"Negotiation error","Network I/O error") << e;
	}
}

PNetworkStream SendMailConnection::connect(NetworkAddress addr, natural timeout) {
	try {
		NetworkStreamSource netsource(addr,1,timeout,timeout,StreamOpenMode::active);
		return netsource.getNext();
	} catch (Exception &e) {
		throw SendMailError(THISLOCATION,"Connect error",addr.asString()) << e;
	}

}

const char *SendMail::errorMsg = "SendMail error: %1 - %2";

}
