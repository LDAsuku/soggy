// packet.cpp

#include "packet.hpp"

// ENet
#include <enet/enet.h>

// protobuf
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

// protos
#include "proto/PacketHead.pb.h"
#include "proto/cmdids.hpp"

inline void put_uint16(uint8_t *buf, uint16_t v) {
	*(uint16_t *)buf = ENET_HOST_TO_NET_16(v);
}
inline void put_uint32(uint8_t *buf, uint32_t v) {
	*(uint32_t *)buf = ENET_HOST_TO_NET_32(v);
}
inline uint16_t get_uint16(uint8_t *buf) {
	return ENET_NET_TO_HOST_16(*(uint16_t *)buf);
}
inline uint32_t get_uint32(uint8_t *buf) {
	return ENET_NET_TO_HOST_32(*(uint32_t *)buf);
}

const google::protobuf::Message *decode_packet(const ENetPacket *enetpacket, unsigned short *pcmdid) {
	uint16_t header_magic = get_uint16(enetpacket->data + 0);
	uint16_t cmdid = get_uint16(enetpacket->data + 2);
	uint16_t head_length = get_uint16(enetpacket->data + 4);
	uint32_t body_length = get_uint32(enetpacket->data + 6);
	uint16_t footer_magic = get_uint16(enetpacket->data + 10 + head_length + body_length);

	assert(header_magic == 0x4567);
	assert(footer_magic == 0x89ab);

	//PacketHead head;
	//head.ParseFromArray(enetpacket->data + 10, head_length);

	const google::protobuf::Descriptor *body_desc = cmdids::cmd_id_to_detail[cmdid]->descriptor;
	google::protobuf::Message *body = google::protobuf::MessageFactory::generated_factory()->GetPrototype(body_desc)->New(); // ownership passed to caller

	body->ParseFromArray(enetpacket->data + 10 + head_length, body_length);

	*pcmdid = cmdid;

	return body;
}

ENetPacket *encode_packet(unsigned short cmdid, const google::protobuf::Message *body) {
	PacketHead head;
	head.set_packetid(cmdid);

	std::string serialized_head = head.SerializeAsString();
	std::string serialized_body = body->SerializeAsString();
	size_t total_serialized_size = 10 + serialized_head.length() + serialized_body.length() + 2;

	uint8_t *p = (uint8_t *)malloc(total_serialized_size);
	put_uint16(p + 0, 0x4567);
	put_uint16(p + 2, cmdid);
	size_t serialized_head_size = serialized_head.length();
	put_uint16(p + 4, serialized_head_size);
	size_t serialized_body_size = serialized_body.length();
	put_uint32(p + 6, serialized_body_size);
	memcpy(p + 10, serialized_head.data(), serialized_head_size);
	memcpy(p + 10 + serialized_head_size, serialized_body.data(), serialized_body_size);
	put_uint16(p + 10 + serialized_head_size + serialized_body_size, 0x89ab);

	ENetPacket *enet_packet = enet_packet_create(p, total_serialized_size, 0);
	free(p);

	return enet_packet;
}
