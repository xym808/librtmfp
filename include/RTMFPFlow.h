/*
Copyright 2016 Thomas Jammet
mathieu.poux[a]gmail.com
jammetthomas[a]gmail.com

This file is part of Librtmfp.

Librtmfp is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Librtmfp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Librtmfp.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "Mona/Mona.h"
#include "FlashConnection.h"
#include "Mona/PoolBuffers.h"
#include "RTMFPWriter.h"
#include "BandWriter.h"

class RTMFPPacket;
class RTMFPFragment;
struct RTMFPGroupConfig;
/**************************************************************
RTMFPFlow is the receiving class for one NetStream of a 
connection, it is associated to an RTMFPWriter for
sending RTMFP answers
It manages acknowledgments and lost count of messages received
*/
class RTMFPFlow : public virtual Mona::Object {
public:
	RTMFPFlow(Mona::UInt64 id,const std::string& signature,const Mona::PoolBuffers& poolBuffers, BandWriter& band, const std::shared_ptr<FlashConnection>& pMainStream);
	RTMFPFlow(Mona::UInt64 id,const std::string& signature,const std::shared_ptr<FlashStream>& pStream, const Mona::PoolBuffers& poolBuffers, BandWriter& band);
	virtual ~RTMFPFlow();

	const Mona::UInt64		id;

	// Update the Id of session (called when a new NetStream has been created)
	void	setId(Mona::UInt64 idFlow);

	bool	critical() const { return _pWriter->critical; }

	// Handle fragments received
	void	receive(Mona::UInt64 stage,Mona::UInt64 deltaNAck,Mona::PacketReader& fragment,Mona::UInt8 flags);
	
	// Send acknowledgment
	void	commit();

	void	fail(const std::string& error);

	void	close();

	bool	consumed() { return _completed; }

	// Record the far peer Id to identify the media source
	void	setPeerId(const std::string& peerId) { if (_pStream) _pStream->setPeerId(peerId); }

private:
	// Handle on fragment received
	void	onFragment(Mona::UInt64 stage,Mona::PacketReader& fragment,Mona::UInt8 flags);

	void	complete();

	bool							_completed; // Indicates that the flow is consumed
	BandWriter&						_band; // RTMFP connection to send messages
	std::shared_ptr<RTMFPWriter>	_pWriter; // Writer for sending AMF messages on RTMFP
	const Mona::UInt64				_stage; // Current stage (index) of messages received
	std::shared_ptr<FlashStream>	_pStream; // NetStream handler of the flow

	// Receiving
	RTMFPPacket*					_pPacket; // current packet/message containing 1 or more fragments (if chunked)
	std::map<Mona::UInt64,RTMFPFragment>	_fragments; // map of all fragments received and not handled for now
	Mona::UInt32					_numberLostFragments;
	const Mona::PoolBuffers&		_poolBuffers;
};

