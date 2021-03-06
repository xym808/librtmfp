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

#include "RTMFP.h"
#include "Mona/Util.h"

using namespace std;
using namespace Mona;

bool RTMFP::ReadAddress(BinaryReader& reader, SocketAddress& address, UInt8 addressType) {
	string data;
	reader.read<string>((addressType & 0x80) ? sizeof(in6_addr) : sizeof(in_addr), data);
	in_addr addrV4;
	in6_addr addrV6;
	IPAddress addr;
	if (addressType & 0x80) {
		memcpy(&addrV6, data.data(), sizeof(in6_addr));
		addr.set(addrV6);
	}
	else {
		memcpy(&addrV4, data.data(), sizeof(in_addr));
		addr.set(addrV4);
	}
	address.set(addr, reader.read16());
	return true;
}

BinaryWriter& RTMFP::WriteAddress(BinaryWriter& writer,const SocketAddress& address,AddressType type) {
	const IPAddress& host = address.host();
	if (host.family() == IPAddress::IPv6)
		writer.write8(type | 0x80);
	else
		writer.write8(type);
	NET_SOCKLEN size(host.size());
	const UInt8* bytes = (const UInt8*)host.addr();
	for(NET_SOCKLEN i=0;i<size;++i)
		writer.write8(bytes[i]);
	return writer.write16(address.port());
}

UInt32 RTMFP::Unpack(BinaryReader& reader) {
	reader.reset();
	UInt32 id=0;
	for(int i=0;i<3;++i)
		id ^= reader.read32();
	reader.reset(4);
	return id;
}

void RTMFP::Pack(BinaryWriter& writer,UInt32 farId) {
	BinaryReader reader(writer.data()+4,writer.size()-4);
	BinaryWriter(writer.data(),4).write32(reader.read32()^reader.read32()^farId);
}


void RTMFP::ComputeAsymetricKeys(const Buffer& sharedSecret, const UInt8* initiatorNonce,UInt16 initNonceSize,
														    const UInt8* responderNonce,UInt16 respNonceSize,
														    UInt8* requestKey,UInt8* responseKey) {
	UInt8 mdp1[Crypto::HMAC::SIZE];
	UInt8 mdp2[Crypto::HMAC::SIZE];
	Crypto::HMAC hmac;

	// doing HMAC-SHA256 of one side
	hmac.compute(EVP_sha256(),responderNonce,respNonceSize,initiatorNonce,initNonceSize,mdp1);
	// doing HMAC-SHA256 of the other side
	hmac.compute(EVP_sha256(),initiatorNonce,initNonceSize,responderNonce,respNonceSize,mdp2);

	// now doing HMAC-sha256 of both result with the shared secret DH key
	hmac.compute(EVP_sha256(),sharedSecret.data(),sharedSecret.size(),mdp1,Crypto::HMAC::SIZE,requestKey);
	hmac.compute(EVP_sha256(),sharedSecret.data(),sharedSecret.size(),mdp2,Crypto::HMAC::SIZE,responseKey);
}

void RTMFP::Write7BitValue(string& buff,UInt64 value) {
	UInt8 shift = (Util::Get7BitValueSize(value)-1)*7;
	bool max = false;
	if(shift>=21) { // 4 bytes maximum
		shift = 22;
		max = true;
	}

	while(shift>=7) {
		String::Append(buff, (char)(0x80 | ((value>>shift)&0x7F)));
		shift -= 7;
	}
	String::Append(buff, (char)(max ? value&0xFF : value&0x7F));
}

bool RTMFP::ReadAddresses(BinaryReader& reader, PEER_LIST_ADDRESS_TYPE& addresses, SocketAddress& hostAddress) {

	// Read all addresses
	SocketAddress address;
	while (reader.available()) {

		UInt8 addressType = reader.read8();
		RTMFP::ReadAddress(reader, address, addressType);
		if (address.family() == IPAddress::IPv4) { // TODO: Handle ivp6

			switch (addressType & 0x0F) {
			case RTMFP::ADDRESS_LOCAL:
			case RTMFP::ADDRESS_PUBLIC:
				addresses.emplace(address, (RTMFP::AddressType)addressType);
				break;
			case RTMFP::ADDRESS_REDIRECTION:
				hostAddress = address; break;
			}
			TRACE("IP Address : ", address.toString(), " - type : ", addressType)
		}
	}
	return !addresses.empty() || hostAddress;
}