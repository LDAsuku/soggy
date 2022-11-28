// packet.hpp

#pragma once

// ENet
#include <enet/enet.h>

// protobuf
#include <google/protobuf/message.h>

// ownership of the returned message belongs to the caller
extern const google::protobuf::Message *decode_packet(const ENetPacket *enetpacket, unsigned short *pcmdid);

extern ENetPacket *encode_packet(unsigned short cmdid, const google::protobuf::Message *body);
