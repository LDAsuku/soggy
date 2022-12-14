// soggy.cpp

// C++
#include <algorithm>
#include <atomic>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <math.h>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <variant>
#ifdef _WIN32
#include <io.h>
#endif

// ENet
#include <enet/enet.h>

// replxx
#include <replxx.hxx>

// protobuf
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

// inih
#include <ini.h>

// protos
#include "proto/AvatarSkillDepotChangeNotify.pb.h"
#include "proto/AvatarTeam.pb.h"
#include "proto/BuyGoodsReq.pb.h"
#include "proto/BuyGoodsRsp.pb.h"
#include "proto/ChangeAvatarReq.pb.h"
#include "proto/ChangeAvatarRsp.pb.h"
#include "proto/EquipParam.pb.h"
#include "proto/GetAllMailReq.pb.h"
#include "proto/GetAllMailRsp.pb.h"
#include "proto/GetPlayerTokenRsp.pb.h"
#include "proto/GetShopReq.pb.h"
#include "proto/GetShopRsp.pb.h"
#include "proto/Material.pb.h"
#include "proto/ProtEntityType.pb.h"
#include "proto/Reliquary.pb.h"
#include "proto/Retcode.pb.h"
#include "proto/AvatarDataNotify.pb.h"
#include "proto/MotionState.pb.h"
#include "proto/AvatarInfo.pb.h"
#include "proto/GetPlayerTokenReq.pb.h"
#include "proto/OpenStateUpdateNotify.pb.h"
#include "proto/PlayerDataNotify.pb.h"
#include "proto/PlayerLoginReq.pb.h"
#include "proto/PlayerEnterSceneNotify.pb.h"
#include "proto/PlayerLoginRsp.pb.h"
#include "proto/PingReq.pb.h"
#include "proto/PingRsp.pb.h"
#include "proto/EnterSceneReadyReq.pb.h"
#include "proto/EnterSceneReadyRsp.pb.h"
#include "proto/GetSceneAreaReq.pb.h"
#include "proto/GetSceneAreaRsp.pb.h"
#include "proto/SceneEntityDisappearNotify.pb.h"
#include "proto/SceneEntityInfo.pb.h"
#include "proto/SceneGadgetInfo.pb.h"
#include "proto/SceneGetAreaExplorePercentReq.pb.h"
#include "proto/SceneGetAreaExplorePercentRsp.pb.h"
#include "proto/SceneInitFinishReq.pb.h"
#include "proto/SceneInitFinishRsp.pb.h"
#include "proto/EnterSceneDoneReq.pb.h"
#include "proto/EnterSceneDoneRsp.pb.h"
#include "proto/PlayerSetPauseReq.pb.h"
#include "proto/PlayerSetPauseRsp.pb.h"
#include "proto/EnterScenePeerNotify.pb.h"
#include "proto/GetScenePointReq.pb.h"
#include "proto/GetScenePointRsp.pb.h"
#include "proto/SceneNpcInfo.pb.h"
#include "proto/ScenePlayerInfo.pb.h"
#include "proto/AvatarEnterSceneInfo.pb.h"
#include "proto/AvatarEquipChangeNotify.pb.h"
#include "proto/HostPlayerNotify.pb.h"
#include "proto/ScenePlayerInfoNotify.pb.h"
#include "proto/PlayerEnterSceneInfoNotify.pb.h"
#include "proto/SceneEntityAppearNotify.pb.h"
#include "proto/ScenePlayerLocationNotify.pb.h"
#include "proto/PlayerStoreNotify.pb.h"
#include "proto/SceneReliquaryInfo.pb.h"
#include "proto/SceneWeaponInfo.pb.h"
#include "proto/ShopGoods.pb.h"
#include "proto/StoreWeightLimitNotify.pb.h"
#include "proto/SceneTransToPointReq.pb.h"
#include "proto/SceneTransToPointRsp.pb.h"
#include "proto/SceneEntityMoveReq.pb.h"
#include "proto/Vector.pb.h"
#include "proto/WearEquipReq.pb.h"
#include "proto/WearEquipRsp.pb.h"
#include "proto/TakeoffEquipReq.pb.h"
#include "proto/TakeoffEquipRsp.pb.h"
#include "proto/SetUpAvatarTeamReq.pb.h"
#include "proto/SetUpAvatarTeamRsp.pb.h"
#include "proto/AvatarTeamUpdateNotify.pb.h"
#include "proto/ClientScriptEventNotify.pb.h"
#include "proto/ChangeGameTimeReq.pb.h"
#include "proto/ChangeGameTimeRsp.pb.h"
#include "proto/SceneTimeNotify.pb.h"
#include "proto/PlayerGameTimeNotify.pb.h"
#include "proto/PersonalSceneJumpReq.pb.h"
#include "proto/PersonalSceneJumpRsp.pb.h"
#include "proto/AllSeenMonsterNotify.pb.h"
#include "proto/DungeonEntryInfoReq.pb.h"
#include "proto/DungeonEntryInfoRsp.pb.h"
#include "proto/DungeonEntryInfo.pb.h"
#include "proto/PlayerEnterDungeonReq.pb.h"
#include "proto/PlayerEnterDungeonRsp.pb.h"
#include "proto/PlayerQuitDungeonReq.pb.h"
#include "proto/PlayerQuitDungeonRsp.pb.h"
#include "proto/cmdids.hpp"

// soggy
#include "game_enums.hpp"
#include "game_data.hpp"
#include "packet.hpp"
#include "log.hpp"
#include "vec.hpp"
#include "dispatch.hpp"
#include "configfile.hpp"

const char *const HISTORY_FILE_NAME = ".soggy_history";

// temporary(?) constants
const int PLAYER_UID = 1;
const int PLAYER_PEER_ID = 0;
const char *const PLAYER_NAME = "soggy:)";
const int PLAYER_TEAM_ID = 1;

// actual correct value is 5000
const int STORE_WEIGHT_LIMIT = 1234567;

const float GROUP_LOAD_DISTANCE = 100.0f;
const float GROUP_UNLOAD_DISTANCE = 120.0f;

int get_next_guid() {
	// this would be saved to disk
	static std::atomic_int guid = 0;
	return ++guid;
}

inline bool is_valid_runtime_id_category(RuntimeIDCategory cat) {
	return (cat >= 1 && cat <= 4) || (cat >= 15 && cat <= 21);
}

// specification from MoleMole.RuntimeIDManager
int make_entity_id(RuntimeIDCategory cat, int peer_id, int sequence) {
	assert(is_valid_runtime_id_category(cat));
	bool is_synced = false;
	return ((cat << RuntimeIDBits::RUNTIME_ID_CATEGORY_SHIFT) & RuntimeIDBits::RUNTIME_ID_CATEGORY_MASK) |
		((peer_id << RuntimeIDBits::RUNTIME_ID_PEER_SHIFT) & RuntimeIDBits::RUNTIME_ID_PEER_MASK) |
		((is_synced << RuntimeIDBits::RUNTIME_ID_IS_SYNCED_SHIFT) & RuntimeIDBits::RUNTIME_ID_IS_SYNCED_MASK) |
		((sequence << RuntimeIDBits::RUNTIME_ID_SEQUENCE_SHIFT) & RuntimeIDBits::RUNTIME_ID_SEQUENCE_MASK);
}

int get_next_runtime_entity_id(RuntimeIDCategory cat) {
	static std::atomic_int seq[21];
	assert(is_valid_runtime_id_category(cat));
	return make_entity_id(cat, PLAYER_PEER_ID, ++seq[cat]);
}

bool is_safe_motion_state(MotionState state) {
	switch (state) {
		case MotionState::MOTION_RESET:
		case MotionState::MOTION_STANDBY:
		case MotionState::MOTION_STANDBY_MOVE:
		case MotionState::MOTION_WALK:
		case MotionState::MOTION_RUN:
		case MotionState::MOTION_DASH:
			return true;
		default:
			return false;
	}
}

struct YSPlayer;
struct YSConnection;

struct YSPlayerItem {
	// persist data
	int guid = 0;
	int item_id = 0;
	struct {
		int count = 1;
	} material;
	struct {
		int level = 1;
		int exp = 1;
		int promote_level = 0;
	} weapon;
	struct {
		int level = 0;
		int exp = 1;
		int promote_level = 0;
		int main_prop_id = 10001; // main stat (default hp)
		std::vector<int> append_prop_ids; // substats
	} reliquary;

	// methods
	const exceloutput::ItemData *get_excel() const;
	void make_item_proto(Item *item) const;
};

struct YSRuntimeEntity {
	struct None {};
	struct Gadget {
		int config_id = 0;
		int gadget_id = 0;
		GadgetState gadget_state = GadgetState::GADGET_STATE_Default;
		int route_id = 0;

		const exceloutput::GadgetData *get_excel() const;
	};
	struct Monster {
		int config_id = 0;
		int monster_id = 0;
		std::vector<int> weapon_gadget_ids;

		const exceloutput::MonsterData *get_excel() const;
	};
	struct Npc {
		int npc_id = 0;

		const exceloutput::NpcData *get_excel() const;
	};

	int runtime_entity_id = 0;
	ProtEntityType entity_type = ProtEntityType::PROT_ENTITY_NONE;
	bool alive = true;
	Vec3f pos;
	Vec3f rot;
	int belong_group_id = 0;
	// don't modify this outside of flush_entities
	bool has_sent_appear = false;

	std::variant<None, Gadget, Monster, Npc> var = None();

	// methods
	inline Gadget *gadget() { return &std::get<Gadget>(this->var); }
	inline Monster *monster() { return &std::get<Monster>(this->var); }
	inline Npc *npc() { return &std::get<Npc>(this->var); }
	inline const Gadget *gadget() const { return &std::get<Gadget>(this->var); }
	inline const Monster *monster() const { return &std::get<Monster>(this->var); }
	inline const Npc *npc() const { return &std::get<Npc>(this->var); }
	void make_scene_entity_info_proto(SceneEntityInfo *sceneentityinfo) const;
};

struct YSAvatar {
	// runtime state
	YSPlayer *owning_player = NULL;
	int avatar_entity_id = 0;
	int weapon_entity_id = 0;

	// persist data
	int avatar_id = 0;
	int avatar_guid = 0;
	int skill_depot_id = 0;
	int weapon_guid = 0;
	int reliquary_bracer_guid = 0;
	int reliquary_dress_guid = 0;
	int reliquary_shoes_guid = 0;
	int reliquary_ring_guid = 0;
	int reliquary_necklace_guid = 0;
	std::vector<int> talent_ids;

	// methods
	const exceloutput::AvatarData *get_excel() const;
	void fill_avatar_props(google::protobuf::Map<uint32_t, PropValue> *prop_map) const;
	void fill_avatar_fight_props(google::protobuf::Map<uint32_t, float> *fight_prop_map) const;
	void make_scene_entity_info_proto(SceneEntityInfo *sceneentityinfo) const;
	void make_scene_weapon_info_proto(SceneWeaponInfo *sceneweaponinfo, const YSPlayerItem *item, int weapon_gadget_id) const;
	void make_scene_reliquary_info_proto(SceneReliquaryInfo *scenereliquaryinfo, const YSPlayerItem *item) const;
	int get_equip_guid_by_slot(EquipType equip_type);
	void set_equip_guid_by_slot(EquipType equip_type, int guid);
	void do_equip(YSPlayerItem *item);
	void do_unequip(EquipType equip_type);
	void recalc_talent_ids();
};

struct YSLocation {
	int sceneid = 1001;
	Vec3f pos;
	Vec3f rot;
	MotionState motionstate = MotionState::MOTION_NONE;
};

enum class SceneLoadState {
	// when the scene is done loading:
	// - any entity packets can be sent
	// - SceneEntityMoveReq may be recorded
	DONE,
	// when a new scene is loading:
	// - don't send any entity packets
	// - ignore SceneEntityMoveReq
	LOADING_NEW_SCENE,
	// when the same scene is loading:
	// - send entity disappear packets, don't send entity appear packets
	// - ignore SceneEntityMoveReq
	LOADING_SAME_SCENE
};

// scene related stuff may be moved to its own struct
struct YSPlayer {
	// runtime state
	YSConnection *conn;
	SceneLoadState current_scene_load_state;
	bool has_sent_avatardatanotify = false;
	YSLocation current_location;
	YSLocation safe_location;
	YSLocation return_location;
	std::unordered_map<int, YSRuntimeEntity> scene_entities;
	int team_entity_id = 0;
	std::unordered_set<int> loaded_groups;

	// persist data
	//int uid = 0;
	//std::string name;
	std::unordered_map<int, YSAvatar> avatars;
	std::unordered_map<int, int> equips_ref; // redundant; item guid -> avatar guid
	std::unordered_map<int, int> avatars_by_id_ref; // redundant; avatar id -> avatar guid
	std::vector<int> team_avatars;
	int team_current_avatar_guid = 0;
	std::unordered_map<int, YSPlayerItem> items;

	// methods
	YSAvatar *get_current_avatar();
	YSAvatar *get_avatar_by_avatar_id(int avatar_id);
	YSAvatar *get_avatar_by_guid(int guid);
	void fill_player_props(google::protobuf::Map<uint32_t, PropValue> *prop_map);
	YSAvatar *add_avatar(int avatar_id);
	void remove_avatar(int guid);
	YSPlayerItem *add_item(int item_id);
	YSRuntimeEntity *scene_spawn_gadget(int gadget_id);
	YSRuntimeEntity *scene_spawn_monster(int monster_id);
	YSRuntimeEntity *scene_spawn_npc(int npc_id);
	void scene_despawn_entity(int entity_id);
	void switch_avatar(int guid);
	void make_player_enter_scene_notify_proto(PlayerEnterSceneNotify *playerenterscenenotify, EnterType entertype, int prev_sceneid);
	void enter_scene(int sceneid, Vec3f pos, Vec3f rot);
	void enter_scene_spawn(int sceneid);
	void scene_flush_entities();
	void changed_position(); // updates loaded groups
	void scene_load_group(const luares::SceneGroup *group);
	void scene_unload_group(const luares::SceneGroup *group);
};

struct YSConnection {
	ENetPeer *peer = NULL;
	YSPlayer *player = NULL;

	void send_packet(const google::protobuf::Message *body);
	template <typename P>
	void send_empty_rsp_with_retcode(Retcode retcode);
};

//
// general proto stuff
//

void pb_make_vector(Vector *to, const Vec3f *from) {
	to->set_x(from->x);
	to->set_y(from->y);
	to->set_z(from->z);
}
Vec3f pb_convert_vector(const Vector *from) {
	Vec3f v;
	v.x = from->x();
	v.y = from->y();
	v.z = from->z();
	return v;
}
void pb_add_prop_pair(google::protobuf::Map<uint32_t, PropValue> *map, PropType prop_type, uint32_t value) {
	PropValue *propvalue = &(*map)[prop_type];
	propvalue->set_type(prop_type);
	propvalue->set_val(value);
	propvalue->set_ival(value);
}

//
// YSConnection
//

#define COLOR_CLIENT_TO_SERVER "\x1b[38;5;111m"
#define COLOR_SERVER_TO_CLIENT "\x1b[38;5;215m"
#define COLOR_RESET "\x1b[0m"

std::unordered_set<unsigned short> dont_log_cmds = { CMD_ID_PingReq, CMD_ID_SceneEntityMoveReq, CMD_ID_EvtAnimatorParameterNotify, CMD_ID_EntityForceSyncReq, CMD_ID_SceneAvatarStaminaStepReq, CMD_ID_EvtFaceToDirNotify, CMD_ID_EvtAvatarUpdateFocusNotify, CMD_ID_AbilityInvocationsNotify, CMD_ID_AbilityInvocationFixedNotify, CMD_ID_ClientAbilityInitFinishNotify, CMD_ID_PingRsp, CMD_ID_PlayerStoreNotify, CMD_ID_EvtSetAttackTargetNotify, CMD_ID_EvtBeingHitNotify, CMD_ID_EvtEntityRenderersChangedNotify, CMD_ID_SceneEntityDrownReq };

void YSConnection::send_packet(const google::protobuf::Message *body) {
	const cmdids::CmdIdDetail *detail = cmdids::cmd_name_to_detail[body->GetTypeName()];
	ENetPacket *packet = encode_packet(detail->cmdid, body);
	if (detail->enet_is_reliable) {
		packet->flags |= ENET_PACKET_FLAG_RELIABLE;
	}

	enet_peer_send(this->peer, detail->enet_channel_id, packet);

	if (dont_log_cmds.find(detail->cmdid) == dont_log_cmds.end()) {
		std::string json_string;
		google::protobuf::util::MessageToJsonString(*body, &json_string);
		soggy_log(COLOR_SERVER_TO_CLIENT "[server->client] %hu %s %s" COLOR_RESET, detail->cmdid, detail->name.c_str(), json_string.c_str());
	}
}
template <typename P>
void YSConnection::send_empty_rsp_with_retcode(Retcode retcode) {
	P rsp;
	rsp.set_retcode(retcode);
	this->send_packet(&rsp);
}

//
// YSPlayer
//

YSAvatar *YSPlayer::get_current_avatar() {
	return &this->avatars.at(this->team_current_avatar_guid);
}
YSAvatar *YSPlayer::get_avatar_by_avatar_id(int avatar_id) {
	int guid = this->avatars_by_id_ref.at(avatar_id);
	return &this->avatars.at(guid);
}
YSAvatar *YSPlayer::get_avatar_by_guid(int guid) {
	return &this->avatars.at(guid);
}
void YSPlayer::fill_player_props(google::protobuf::Map<uint32_t, PropValue> *prop_map) {
	// future: unhardcode this
	pb_add_prop_pair(prop_map, PropType::PROP_LAST_CHANGE_AVATAR_TIME, 0);
	pb_add_prop_pair(prop_map, PropType::PROP_IS_FLYABLE, 1);
	pb_add_prop_pair(prop_map, PropType::PROP_IS_WEATHER_LOCKED, 0);
	pb_add_prop_pair(prop_map, PropType::PROP_IS_GAME_TIME_LOCKED, 0);
	pb_add_prop_pair(prop_map, PropType::PROP_IS_TRANSFERABLE, 1);
	pb_add_prop_pair(prop_map, PropType::PROP_MAX_STAMINA, 15000); // actual stamina maximum needs to be confirmed
	pb_add_prop_pair(prop_map, PropType::PROP_CUR_PERSIST_STAMINA, 15000);
	pb_add_prop_pair(prop_map, PropType::PROP_CUR_TEMPORARY_STAMINA, 15000);
	pb_add_prop_pair(prop_map, PropType::PROP_PLAYER_LEVEL, 20); // adventure rank
	pb_add_prop_pair(prop_map, PropType::PROP_PLAYER_EXP, 0); // adventure exp
	pb_add_prop_pair(prop_map, PropType::PROP_PLAYER_HCOIN, 1); // primogem
	pb_add_prop_pair(prop_map, PropType::PROP_PLAYER_SCOIN, 1); // mora
	pb_add_prop_pair(prop_map, PropType::PROP_IS_WORLD_ENTERABLE, 1);
	pb_add_prop_pair(prop_map, PropType::PROP_IS_MP_MODE_AVAILABLE, 1);
}

void YSPlayer::make_player_enter_scene_notify_proto(PlayerEnterSceneNotify *playerenterscenenotify, EnterType entertype, int prev_sceneid) {
	playerenterscenenotify->set_sceneid(this->current_location.sceneid);
	if (this->current_location.sceneid != prev_sceneid) {
		playerenterscenenotify->set_prevsceneid(prev_sceneid);
		playerenterscenenotify->mutable_prevpos();
	}
	pb_make_vector(playerenterscenenotify->mutable_pos(), &this->current_location.pos);
	playerenterscenenotify->set_scenebegintime(0);
	playerenterscenenotify->set_type(entertype);
	playerenterscenenotify->set_targetuid(PLAYER_UID);
}

void YSPlayer::enter_scene(int sceneid, Vec3f pos, Vec3f rot) {
	int prev_sceneid = this->current_location.sceneid;
	this->current_location.sceneid = sceneid;
	this->current_location.pos = pos;
	this->current_location.rot = rot;

	EnterType entertype;
	if (this->current_location.sceneid != prev_sceneid) {
		// entering a different scene
		entertype = EnterType::ENTER_JUMP;
		this->current_scene_load_state = SceneLoadState::LOADING_NEW_SCENE;

		load_scene_script_data(sceneid);
		this->loaded_groups.clear();
		this->scene_entities.clear();
	} else {
		// same scene
		entertype = EnterType::ENTER_GOTO;
		this->current_scene_load_state = SceneLoadState::LOADING_SAME_SCENE;
	}

	this->changed_position();

	PlayerEnterSceneNotify playerenterscenenotify;
	this->make_player_enter_scene_notify_proto(&playerenterscenenotify, entertype, prev_sceneid);
	conn->send_packet(&playerenterscenenotify);
}

void YSPlayer::enter_scene_spawn(int sceneid) {
	load_scene_script_data(sceneid);

	Vec3f spawn_pos;
	Vec3f spawn_rot;
	try {
		luares::SceneConfig *config = &luares::scene_configs.at(sceneid);
		spawn_pos = config->born_pos;
		spawn_rot = config->born_rot;
	} catch (std::out_of_range &) {
		soggy_log("warn: no spawn point for scene %d, using (0, 300, 0)", sceneid);
		spawn_pos.y = 300.0f;
	}

	this->enter_scene(sceneid, spawn_pos, spawn_rot);
}

float distance(const Vec3f v1, const Vec3f v2) {
	float dx = v2.x - v1.x;
	float dy = v2.y - v1.y;
	float dz = v2.z - v1.z;
	return sqrtf((dx * dx) + (dy * dy) + (dz * dz));
}

// recalc loaded groups. load or unload them if necessary
void YSPlayer::changed_position() {
	if (is_safe_motion_state(this->current_location.motionstate)) {
		this->safe_location = this->current_location;
	}

	const luares::SceneConfig *scene_config;
	try {
		scene_config = &luares::scene_configs.at(this->current_location.sceneid);
	} catch (std::out_of_range &) {
		return;
	}

	for (const auto &it : scene_config->blocks) {
		const luares::SceneBlock *block = &it.second;
		for (const auto &it : block->groups) {
			const luares::SceneGroup *group = &it.second;

			auto it2 = this->loaded_groups.find(group->id);
			if (it2 != this->loaded_groups.end()) {
				// the group is loaded, try unloading it
				if (distance(this->current_location.pos, group->pos) > GROUP_UNLOAD_DISTANCE) {
					// unload it
					this->scene_unload_group(group);
					it2 = this->loaded_groups.erase(it2);
				}
			} else {
				// the group isn't loaded, try loading it
				if (distance(this->current_location.pos, group->pos) < GROUP_LOAD_DISTANCE) {
					// load it
					this->scene_load_group(group);
					this->loaded_groups.emplace(group->id);
				}
			}
		}
	}

	this->scene_flush_entities();
}

void YSPlayer::scene_load_group(const luares::SceneGroup *group) {
	soggy_log("loading group id %d", group->id);

	for (const auto &it : group->gadgets) {
		const luares::SceneEntity *gadget = &it;

		YSRuntimeEntity *entity = this->scene_spawn_gadget(gadget->gadget.gadget_id);
		entity->belong_group_id = group->id;
		entity->pos = gadget->pos;
		entity->rot = gadget->rot;
		YSRuntimeEntity::Gadget *entity_gadget = entity->gadget();
		entity_gadget->config_id = gadget->config_id;
		entity_gadget->gadget_state = gadget->gadget.state;
		entity_gadget->route_id = gadget->gadget.route_id;
	}

	for (const auto &it : group->monsters) {
		const luares::SceneEntity *monster = &it;

		YSRuntimeEntity *entity = this->scene_spawn_monster(monster->monster.monster_id);
		entity->belong_group_id = group->id;
		entity->pos = monster->pos;
		entity->rot = monster->rot;
		YSRuntimeEntity::Monster *entity_monster = entity->monster();
		entity_monster->config_id = monster->config_id;
	}

	for (const auto &it : group->npcs) {
		const luares::SceneEntity *npc = &it;

		YSRuntimeEntity *entity = this->scene_spawn_npc(npc->npc.npc_id);
		entity->belong_group_id = group->id;
		entity->pos = npc->pos;
		entity->rot = npc->rot;
	}
}

void YSPlayer::scene_unload_group(const luares::SceneGroup *group) {
	soggy_log("unloading group id %d", group->id);
	for (auto &it : this->scene_entities) {
		const YSRuntimeEntity *entity = &it.second;
		if (entity->belong_group_id == group->id) {
			this->scene_despawn_entity(entity->runtime_entity_id);
		}
	}
}

void YSPlayer::scene_flush_entities() {
	if (this->current_scene_load_state == SceneLoadState::LOADING_NEW_SCENE) {
		return;
	}

	SceneEntityAppearNotify appearnotify;
	SceneEntityDisappearNotify disappearnotify;

	for (auto it = this->scene_entities.begin(); it != this->scene_entities.end();) {
		int entity_id = it->first;
		YSRuntimeEntity *entity = &it->second;

		if (entity->alive) {
			if (this->current_scene_load_state == SceneLoadState::DONE && !entity->has_sent_appear) {
				// send appear
				entity->make_scene_entity_info_proto(appearnotify.add_entitylist());
				entity->has_sent_appear = true;
			}
		} else {
			// send disappear and delete
			if (entity->has_sent_appear) {
				disappearnotify.add_entitylist(entity_id);
			}
			it = this->scene_entities.erase(it);
			continue;
		}
		it++;
	}

	if (appearnotify.entitylist_size() > 0) {
		appearnotify.set_appeartype(VisionType::VISION_MEET);
		this->conn->send_packet(&appearnotify);
	}
	if (disappearnotify.entitylist_size() > 0) {
		disappearnotify.set_disappeartype(VisionType::VISION_MISS);
		this->conn->send_packet(&disappearnotify);
	}
}

//
// YSPlayerItem
//

const exceloutput::ItemData *YSPlayerItem::get_excel() const {
	return &exceloutput::item_datas.at(this->item_id);
}

void YSPlayerItem::make_item_proto(Item *item) const {
	item->set_itemid(this->item_id);
	item->set_guid(this->guid);

	switch (this->get_excel()->type) {
		case ItemType::ITEM_NONE:
		case ItemType::ITEM_VIRTUAL:
			break;
		case ItemType::ITEM_MATERIAL: {
			Material *material = item->mutable_material();
			material->set_count(this->material.count);
			break;
		}
		case ItemType::ITEM_RELIQUARY: {
			Reliquary *reliquary = item->mutable_equip()->mutable_reliquary();
			reliquary->set_level(this->reliquary.level);
			reliquary->set_exp(this->reliquary.exp);
			reliquary->set_promotelevel(this->reliquary.promote_level);
			reliquary->set_mainpropid(this->reliquary.main_prop_id);
			for (const int value : this->reliquary.append_prop_ids) {
				reliquary->add_appendpropidlist(value);
			}
			break;
		}
		case ItemType::ITEM_WEAPON: {
			Weapon *weapon = item->mutable_equip()->mutable_weapon();
			weapon->set_level(this->weapon.level);
			weapon->set_exp(this->weapon.exp);
			weapon->set_promotelevel(this->weapon.promote_level);
			break;
		}
	}
}

//
// YSRuntimeEntity
//

void YSRuntimeEntity::make_scene_entity_info_proto(SceneEntityInfo *sceneentityinfo) const {
	sceneentityinfo->set_entitytype(this->entity_type);
	sceneentityinfo->set_entityid(this->runtime_entity_id);
	pb_make_vector(sceneentityinfo->mutable_motioninfo()->mutable_pos(), &this->pos);
	pb_make_vector(sceneentityinfo->mutable_motioninfo()->mutable_rot(), &this->rot);
	sceneentityinfo->set_lifestate(LifeState::LIFE_ALIVE);
	sceneentityinfo->mutable_abilityinfo()->set_isinited(false);
	switch (this->entity_type) {
		case ProtEntityType::PROT_ENTITY_GADGET: {
			const YSRuntimeEntity::Gadget *entity_gadget = this->gadget();
			const exceloutput::GadgetData *excel_gadget = entity_gadget->get_excel();

			SceneGadgetInfo *gadgetinfo = sceneentityinfo->mutable_gadget();
			gadgetinfo->set_configid(entity_gadget->config_id);
			gadgetinfo->set_gadgetid(entity_gadget->gadget_id);
			gadgetinfo->set_gadgetstate(entity_gadget->gadget_state);
			gadgetinfo->set_isenableinteract(true);
			gadgetinfo->set_authoritypeerid(PLAYER_PEER_ID);
			switch (excel_gadget->entity_type) {
				case EntityType::ENTITY_GatherPoint: {
					// future
					break;
				}
				case EntityType::ENTITY_Platform: {
					gadgetinfo->mutable_platform()->set_routeid(entity_gadget->route_id);
					gadgetinfo->mutable_platform()->set_isstarted(true);
					break;
				}
				default:
					break;
			}
			break;
		}
		case ProtEntityType::PROT_ENTITY_MONSTER: {
			const YSRuntimeEntity::Monster *entity_monster = this->monster();

			SceneMonsterInfo *monsterinfo = sceneentityinfo->mutable_monster();
			monsterinfo->set_configid(entity_monster->config_id);
			monsterinfo->set_monsterid(entity_monster->monster_id);
			monsterinfo->set_authoritypeerid(PLAYER_PEER_ID);
			for (int gadget_id : entity_monster->weapon_gadget_ids) {
				// these don't seem to need an entity id (yet)
				SceneWeaponInfo *weaponinfo = monsterinfo->add_weaponlist();
				weaponinfo->set_gadgetid(gadget_id);
				weaponinfo->mutable_abilityinfo()->set_isinited(false);
			}
			break;
		}
		case ProtEntityType::PROT_ENTITY_NPC: {
			const YSRuntimeEntity::Npc *entity_npc = this->npc();

			SceneNpcInfo *npcinfo = sceneentityinfo->mutable_npc();
			npcinfo->set_npcid(entity_npc->npc_id);
			break;
		}
		default:
			break;
	}
}

const exceloutput::GadgetData *YSRuntimeEntity::Gadget::get_excel() const {
	return &exceloutput::gadget_datas.at(this->gadget_id);
}
const exceloutput::MonsterData *YSRuntimeEntity::Monster::get_excel() const {
	return &exceloutput::monster_datas.at(this->monster_id);
}
const exceloutput::NpcData *YSRuntimeEntity::Npc::get_excel() const {
	return &exceloutput::npc_datas.at(this->npc_id);
}

//
// YSAvatar
//

void YSAvatar::fill_avatar_props(google::protobuf::Map<uint32_t, PropValue> *prop_map) const {
	// future: unhardcode this
	pb_add_prop_pair(prop_map, PropType::PROP_EXP, 1); // avatar exp
	pb_add_prop_pair(prop_map, PropType::PROP_BREAK_LEVEL, 0); // avatar promote level
	pb_add_prop_pair(prop_map, PropType::PROP_LEVEL, 1); // avatar level
}

void YSAvatar::fill_avatar_fight_props(google::protobuf::Map<uint32_t, float> *fight_prop_map) const {
	// future: unhardcode this
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_BASE_HP, 123456.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_HP, 123456.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_HP_PERCENT, 0.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_BASE_ATTACK, 123456.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_ATTACK, 123456.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_ATTACK_PERCENT, 0.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_BASE_DEFENSE, 123456.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_DEFENSE, 123456.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_DEFENSE_PERCENT, 0.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_CHARGE_EFFICIENCY, 1.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_CUR_HP, 123456.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_MAX_HP, 123456.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_CUR_ATTACK, 123456.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_CUR_DEFENSE, 123456.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_CUR_SPEED, 0.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_CUR_FIRE_ENERGY, 100.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_CUR_ELEC_ENERGY, 100.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_CUR_WATER_ENERGY, 100.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_CUR_GRASS_ENERGY, 100.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_CUR_WIND_ENERGY, 100.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_CUR_ICE_ENERGY, 100.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_CUR_ROCK_ENERGY, 100.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_MAX_FIRE_ENERGY, 100.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_MAX_ELEC_ENERGY, 100.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_MAX_WATER_ENERGY, 100.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_MAX_GRASS_ENERGY, 100.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_MAX_WIND_ENERGY, 100.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_MAX_ICE_ENERGY, 100.0f });
	fight_prop_map->insert({ FightPropType::FIGHT_PROP_MAX_ROCK_ENERGY, 100.0f });
}

void YSAvatar::make_scene_entity_info_proto(SceneEntityInfo *sceneentityinfo) const {
	//const exceloutput::AvatarData *excel_avatar = &exceloutput::avatar_datas.at(this->avatar_guid);
	//const exceloutput::AvatarSkillDepotData *excel_avatar_skill_depot = &exceloutput::avatar_skill_depot_datas.at(this->skill_depot_id);

	const YSPlayerItem *weapon_item = &this->owning_player->items.at(this->weapon_guid);
	int weapon_gadget_id = weapon_item->get_excel()->weapon.gadget_id;

	sceneentityinfo->set_entitytype(ProtEntityType::PROT_ENTITY_AVATAR);
	sceneentityinfo->set_entityid(this->avatar_entity_id);
	sceneentityinfo->mutable_abilityinfo()->set_isinited(false);
	MotionInfo *motioninfo = sceneentityinfo->mutable_motioninfo();
	pb_make_vector(motioninfo->mutable_pos(), &this->owning_player->current_location.pos);
	pb_make_vector(motioninfo->mutable_rot(), &this->owning_player->current_location.rot);
	motioninfo->mutable_speed();
	motioninfo->set_state(MotionState::MOTION_STANDBY);
	sceneentityinfo->set_lifestate(LifeState::LIFE_ALIVE);
	this->fill_avatar_props(sceneentityinfo->mutable_propmap());
	this->fill_avatar_fight_props(sceneentityinfo->mutable_fightpropmap());
	SceneAvatarInfo *sceneavatarinfo = sceneentityinfo->mutable_avatar();
	sceneavatarinfo->set_uid(PLAYER_UID);
	sceneavatarinfo->set_avatarid(this->avatar_id);
	sceneavatarinfo->set_guid(this->avatar_guid);
	sceneavatarinfo->set_peerid(PLAYER_PEER_ID);
	sceneavatarinfo->add_equipidlist(weapon_item->item_id);
	sceneavatarinfo->set_skilldepotid(this->skill_depot_id);
	this->make_scene_weapon_info_proto(sceneavatarinfo->mutable_weapon(), weapon_item, weapon_gadget_id);

	auto add_reliquary_equip = [&](int guid) -> void {
		if (guid != 0) {
			const YSPlayerItem *item = &this->owning_player->items.at(guid);
			sceneavatarinfo->add_equipidlist(item->item_id);
			this->make_scene_reliquary_info_proto(sceneavatarinfo->add_reliquarylist(), item);
		}
	};
	add_reliquary_equip(this->reliquary_bracer_guid);
	add_reliquary_equip(this->reliquary_dress_guid);
	add_reliquary_equip(this->reliquary_shoes_guid);
	add_reliquary_equip(this->reliquary_ring_guid);
	add_reliquary_equip(this->reliquary_necklace_guid);

	for (int talent_id : this->talent_ids) {
		sceneavatarinfo->add_talentidlist(talent_id);
	}
}

void YSAvatar::make_scene_weapon_info_proto(SceneWeaponInfo *sceneweaponinfo, const YSPlayerItem *item, int weapon_gadget_id) const {
	sceneweaponinfo->set_entityid(this->weapon_entity_id);
	sceneweaponinfo->set_gadgetid(weapon_gadget_id);
	sceneweaponinfo->set_itemid(item->item_id);
	sceneweaponinfo->set_guid(item->guid);
	sceneweaponinfo->set_level(item->weapon.level);
	sceneweaponinfo->set_promotelevel(item->weapon.promote_level);
}

void YSAvatar::make_scene_reliquary_info_proto(SceneReliquaryInfo *scenereliquaryinfo, const YSPlayerItem *item) const {
	scenereliquaryinfo->set_itemid(item->item_id);
	scenereliquaryinfo->set_guid(item->guid);
	scenereliquaryinfo->set_level(item->reliquary.level);
	scenereliquaryinfo->set_promotelevel(item->reliquary.promote_level);
}

int YSAvatar::get_equip_guid_by_slot(EquipType equip_type) {
	switch (equip_type) {
		case EquipType::EQUIP_WEAPON: return this->weapon_guid;
		case EquipType::EQUIP_BRACER: return this->reliquary_bracer_guid;
		case EquipType::EQUIP_DRESS: return this->reliquary_dress_guid;
		case EquipType::EQUIP_SHOES: return this->reliquary_shoes_guid;
		case EquipType::EQUIP_RING: return this->reliquary_ring_guid;
		case EquipType::EQUIP_NECKLACE: return this->reliquary_necklace_guid;
		default: return 0;
	}
}

void YSAvatar::set_equip_guid_by_slot(EquipType equip_type, int guid) {
	int prev_guid = this->get_equip_guid_by_slot(equip_type);
	if (prev_guid != 0) {
		this->owning_player->equips_ref.erase(prev_guid);
	}
	this->owning_player->equips_ref.insert({ guid, this->avatar_guid });
	switch (equip_type) {
		case EquipType::EQUIP_WEAPON: this->weapon_guid = guid; break;
		case EquipType::EQUIP_BRACER: this->reliquary_bracer_guid = guid; break;
		case EquipType::EQUIP_DRESS: this->reliquary_dress_guid = guid; break;
		case EquipType::EQUIP_SHOES: this->reliquary_shoes_guid = guid; break;
		case EquipType::EQUIP_RING: this->reliquary_ring_guid = guid; break;
		case EquipType::EQUIP_NECKLACE: this->reliquary_necklace_guid = guid; break;
		default: break;
	}
}

void YSAvatar::do_equip(YSPlayerItem *item) {
	const exceloutput::ItemData *excel_item = &exceloutput::item_datas.at(item->item_id);

	AvatarEquipChangeNotify notify;
	notify.set_avatarguid(this->avatar_guid);
	notify.set_equiptype(excel_item->equip_type);
	notify.set_itemid(item->item_id);
	notify.set_equipguid(item->guid);
	switch (excel_item->type) {
		case ItemType::ITEM_WEAPON:
			this->make_scene_weapon_info_proto(notify.mutable_weapon(), item, excel_item->weapon.gadget_id);
			break;
		case ItemType::ITEM_RELIQUARY:
			this->make_scene_reliquary_info_proto(notify.mutable_reliquary(), item);
			break;
		default:
			break;
	}
	this->owning_player->conn->send_packet(&notify);

	this->set_equip_guid_by_slot(excel_item->equip_type, item->guid);
}

void YSAvatar::do_unequip(EquipType equip_type) {
	AvatarEquipChangeNotify notify;
	notify.set_avatarguid(this->avatar_guid);
	notify.set_equiptype(equip_type);
	notify.set_equipguid(0);
	this->owning_player->conn->send_packet(&notify);

	this->set_equip_guid_by_slot(equip_type, 0);
}

const exceloutput::AvatarData *YSAvatar::get_excel() const {
	return &exceloutput::avatar_datas.at(this->avatar_id);
}

void YSAvatar::recalc_talent_ids() {
	this->talent_ids.clear();
	const exceloutput::AvatarSkillDepotData *excel_avatar_skill_depot;
	try {
		excel_avatar_skill_depot = &exceloutput::avatar_skill_depot_datas.at(this->skill_depot_id);
	} catch (const std::out_of_range &) {
		return;
	}
	for (int talent_group : excel_avatar_skill_depot->talent_groups) {
		for (const auto &e : exceloutput::talent_skill_datas) {
			const exceloutput::TalentSkillData *talent_skill = &e.second;
			if (talent_skill->talent_group_id == talent_group) {
				this->talent_ids.push_back(talent_skill->id);
			}
		}
	}
}

void YSPlayer::switch_avatar(int guid) {
	const YSAvatar *prev_avatar = this->get_current_avatar();
	this->team_current_avatar_guid = guid;
	const YSAvatar *new_avatar = this->get_current_avatar();

	// disappear the previously on-field avatar
	SceneEntityDisappearNotify disappearnotify;
	disappearnotify.add_entitylist(prev_avatar->avatar_entity_id);
	disappearnotify.set_disappeartype(VisionType::VISION_REPLACE);
	this->conn->send_packet(&disappearnotify);

	// appear a new one
	SceneEntityAppearNotify appearnotify;
	new_avatar->make_scene_entity_info_proto(appearnotify.add_entitylist());
	appearnotify.set_param(prev_avatar->avatar_entity_id); // old entity id
	appearnotify.set_appeartype(VisionType::VISION_REPLACE);
	this->conn->send_packet(&appearnotify);
}

YSPlayerItem *YSPlayer::add_item(int item_id) {
	// future: check materials for stacking
	int item_guid = get_next_guid();

	YSPlayerItem *item = &this->items[item_guid];
	item->guid = item_guid;
	item->item_id = item_id;
	return item;
}

YSAvatar *YSPlayer::add_avatar(int avatar_id) {
	const exceloutput::AvatarData *excel_avatar = &exceloutput::avatar_datas.at(avatar_id);

	YSPlayerItem *initial_weapon = this->add_item(excel_avatar->initial_weapon_id);

	int avatar_guid = get_next_guid();
	YSAvatar *avatar = &this->avatars[avatar_guid];
	avatar->owning_player = this;
	avatar->avatar_id = avatar_id;
	avatar->avatar_guid = avatar_guid;
	avatar->avatar_entity_id = get_next_runtime_entity_id(RuntimeIDCategory::AVATAR_CATE);
	avatar->weapon_guid = initial_weapon->guid;
	avatar->weapon_entity_id = get_next_runtime_entity_id(RuntimeIDCategory::GADGET_CATE);
	avatar->reliquary_bracer_guid = 0;
	avatar->reliquary_dress_guid = 0;
	avatar->reliquary_shoes_guid = 0;
	avatar->reliquary_ring_guid = 0;
	avatar->reliquary_necklace_guid = 0;

	this->equips_ref.insert({ initial_weapon->guid, avatar_guid });
	this->avatars_by_id_ref.insert({ avatar_id, avatar_guid });

	// (temporary?) exceptions for lumine and aether
	switch (avatar_id) {
		case 10000005: avatar->skill_depot_id = 506; break; // aether, geo
		case 10000007: avatar->skill_depot_id = 704; break; // lunime, anemo
		default: avatar->skill_depot_id = excel_avatar->skill_depot_id; break;
	}

	avatar->recalc_talent_ids();

	if (this->has_sent_avatardatanotify) {
		// future: send AvatarAddNotify
	}

	return avatar;
}

void YSPlayer::remove_avatar(int avatar_guid) {
	// future
	// send AvatarDelNotify
}

YSRuntimeEntity *YSPlayer::scene_spawn_gadget(int gadget_id) {
	int entity_id = get_next_runtime_entity_id(RuntimeIDCategory::GADGET_CATE);

	YSRuntimeEntity *entity = &this->scene_entities[entity_id];

	entity->runtime_entity_id = entity_id;
	entity->entity_type = ProtEntityType::PROT_ENTITY_GADGET;
	YSRuntimeEntity::Gadget *entity_gadget = &entity->var.emplace<YSRuntimeEntity::Gadget>(YSRuntimeEntity::Gadget());
	entity_gadget->gadget_id = gadget_id;

	return entity;
}

YSRuntimeEntity *YSPlayer::scene_spawn_monster(int monster_id) {
	int entity_id = get_next_runtime_entity_id(RuntimeIDCategory::MONSTER_CATE);

	YSRuntimeEntity *entity = &this->scene_entities[entity_id];
	entity->runtime_entity_id = entity_id;
	entity->entity_type = ProtEntityType::PROT_ENTITY_MONSTER;
	YSRuntimeEntity::Monster *entity_monster = &entity->var.emplace<YSRuntimeEntity::Monster>(YSRuntimeEntity::Monster());
	entity_monster->monster_id = monster_id;

	const exceloutput::MonsterData *excel_monster = entity_monster->get_excel();

	if (excel_monster->equip_1 != 0) {
		entity_monster->weapon_gadget_ids.push_back(excel_monster->equip_1);
	}
	if (excel_monster->equip_2 != 0) {
		entity_monster->weapon_gadget_ids.push_back(excel_monster->equip_2);
	}

	return entity;
}

YSRuntimeEntity *YSPlayer::scene_spawn_npc(int npc_id) {
	int entity_id = get_next_runtime_entity_id(RuntimeIDCategory::NPC_CATE);

	YSRuntimeEntity *entity = &this->scene_entities[entity_id];
	entity->runtime_entity_id = entity_id;
	entity->entity_type = ProtEntityType::PROT_ENTITY_NPC;
	YSRuntimeEntity::Npc *entity_npc = &entity->var.emplace<YSRuntimeEntity::Npc>(YSRuntimeEntity::Npc());
	entity_npc->npc_id = npc_id;

	return entity;
}

void YSPlayer::scene_despawn_entity(int entity_id) {
	YSRuntimeEntity *entity = &this->scene_entities.at(entity_id);
	entity->alive = false;
}


YSPlayer mainplayer;
YSConnection mainconn;

void init_mainplayer() {
	mainplayer.team_entity_id = get_next_runtime_entity_id(RuntimeIDCategory::MANAGER_CATE);

	// give a bunch of avatars
	for (const auto &it : exceloutput::avatar_datas) {
		const exceloutput::AvatarData *excel_avatar = &it.second;
		if (excel_avatar->use_type == AvatarUseType::AVATAR_FORMAL) {
			mainplayer.add_avatar(excel_avatar->id);
		}
	}

	// add some avatars to the current team
	mainplayer.team_avatars.push_back(mainplayer.get_avatar_by_avatar_id(10000007)->avatar_guid); // lumine
	mainplayer.team_avatars.push_back(mainplayer.get_avatar_by_avatar_id(10000003)->avatar_guid); // jean
	mainplayer.team_avatars.push_back(mainplayer.get_avatar_by_avatar_id(10000006)->avatar_guid); // lisa

	mainplayer.team_current_avatar_guid = mainplayer.team_avatars.at(0);

	// give every single item
	for (const auto &it : exceloutput::item_datas) {
		const exceloutput::ItemData *excel_item = &it.second;
		switch (excel_item->type) {
			case ItemType::ITEM_MATERIAL:
			case ItemType::ITEM_WEAPON:
			case ItemType::ITEM_RELIQUARY:
				mainplayer.add_item(excel_item->id);
				break;
			default:
				break;
		}
	}

	// in front of temple of the falcon
	mainplayer.current_location.sceneid = 3;
	mainplayer.current_location.pos.x = 2136.926f;
	mainplayer.current_location.pos.y = 208.130f;
	mainplayer.current_location.pos.z = -1172.234f;
	mainplayer.current_location.rot.x = 0.0f;
	mainplayer.current_location.rot.y = 157.219f;
	mainplayer.current_location.rot.z = 0.0f;
}

//
// packet handlers
//

void handle_GetPlayerTokenReq(YSConnection *conn, const GetPlayerTokenReq *getplayertokenreq) {
	GetPlayerTokenRsp getplayertokenrsp;
	getplayertokenrsp.set_retcode(Retcode::RET_SUCC);
	getplayertokenrsp.set_uid(PLAYER_UID);
	getplayertokenrsp.set_token("token1");
	getplayertokenrsp.set_accounttype(1);
	getplayertokenrsp.set_accountuid(std::to_string(PLAYER_UID));
	conn->send_packet(&getplayertokenrsp);
}

void handle_PlayerLoginReq(YSConnection *conn, const PlayerLoginReq *playerloginreq) {
	conn->player->current_scene_load_state = SceneLoadState::LOADING_NEW_SCENE;

	PlayerDataNotify playerdatanotify;
	playerdatanotify.set_nickname(PLAYER_NAME);
	playerdatanotify.set_servertime(0);
	conn->player->fill_player_props(playerdatanotify.mutable_propmap());
	conn->send_packet(&playerdatanotify);

	OpenStateUpdateNotify openstateupdatenotify;
	google::protobuf::Map<uint32_t, uint32_t> *openstatemap = openstateupdatenotify.mutable_openstatemap();
	openstatemap->insert({ OpenStateType::OPEN_STATE_PAIMON, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_PAIMON_NAVIGATION, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_AVATAR_PROMOTE, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_AVATAR_TALENT, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_WEAPON_PROMOTE, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_WEAPON_AWAKEN, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_QUEST_REMIND, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_GAME_GUIDE, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_COOK, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_WEAPON_UPGRADE, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_RELIQUARY_UPGRADE, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_RELIQUARY_PROMOTE, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_WEAPON_PROMOTE_GUIDE, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_WEAPON_CHANGE_GUIDE, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_PLAYER_LVUP_GUIDE, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_FRESHMAN_GUIDE, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_SKIP_FRESHMAN_GUIDE, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_GUIDE_MOVE_CAMERA, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_GUIDE_SCALE_CAMERA, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_GUIDE_KEYBOARD, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_GUIDE_MOVE, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_GUIDE_JUMP, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_GUIDE_SPRINT, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_GUIDE_MAP, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_GUIDE_ATTACK, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_GUIDE_FLY, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_GUIDE_TALENT, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_GUIDE_RELIC, 1 });
	openstatemap->insert({ OpenStateType::OPEN_STATE_GUIDE_RELIC_PROM, 1 });
	conn->send_packet(&openstateupdatenotify);

	PlayerStoreNotify playerstorenotify;
	playerstorenotify.set_storetype(StoreType::STORE_PACK);
	playerstorenotify.set_weightlimit(STORE_WEIGHT_LIMIT);
	for (const auto &it : conn->player->items) {
		const YSPlayerItem *player_item = &it.second;
		player_item->make_item_proto(playerstorenotify.add_itemlist());
	}
	conn->send_packet(&playerstorenotify);

	AvatarDataNotify avatardatanotify;

	for (const auto &it : conn->player->avatars) {
		const YSAvatar *player_avatar = &it.second;

		int skill_depot_id = player_avatar->skill_depot_id;
		int weapon_guid = player_avatar->weapon_guid;

		AvatarInfo *avatarinfo = avatardatanotify.add_avatarlist();
		avatarinfo->set_avatarid(player_avatar->avatar_id);
		avatarinfo->set_guid(player_avatar->avatar_guid);
		avatarinfo->set_lifestate(LifeState::LIFE_ALIVE);
		avatarinfo->add_equipguidlist(weapon_guid);
		player_avatar->fill_avatar_props(avatarinfo->mutable_propmap());
		player_avatar->fill_avatar_fight_props(avatarinfo->mutable_fightpropmap());
		avatarinfo->set_skilldepotid(skill_depot_id);

		for (int talent_id : player_avatar->talent_ids) {
			avatarinfo->add_talentidlist(talent_id);
		}
	}

	AvatarTeam *avatarteam = &(*avatardatanotify.mutable_avatarteammap())[PLAYER_TEAM_ID];
	for (int guid : conn->player->team_avatars) {
		avatarteam->add_avatarguidlist(guid);
	}

	avatardatanotify.set_curavatarteamid(PLAYER_TEAM_ID);
	avatardatanotify.set_chooseavatarguid(conn->player->team_current_avatar_guid);
	conn->send_packet(&avatardatanotify);

	conn->player->has_sent_avatardatanotify = true;

	AllSeenMonsterNotify allseenmonsternotify;
	for (const auto it : exceloutput::monster_datas) {
		int monster_id = it.first;
		allseenmonsternotify.add_monsteridlist(monster_id);
	}
	conn->send_packet(&allseenmonsternotify);

	PlayerEnterSceneNotify playerenterscenenotify;
	conn->player->make_player_enter_scene_notify_proto(&playerenterscenenotify, EnterType::ENTER_SELF, 0);
	conn->send_packet(&playerenterscenenotify);

	load_scene_script_data(conn->player->current_location.sceneid);
	conn->player->loaded_groups.clear();
	conn->player->scene_entities.clear();
	conn->player->changed_position();

	PlayerLoginRsp playerloginrsp;
	playerloginrsp.set_retcode(Retcode::RET_SUCC);
	playerloginrsp.set_targetuid(PLAYER_UID);
	playerloginrsp.set_dataversion(soggy_cfg.common_design_data_version);
	playerloginrsp.set_resversion(soggy_cfg.common_game_res_version);
	conn->send_packet(&playerloginrsp);
}

void handle_PingReq(YSConnection *conn, const PingReq *pingreq) {
	PingRsp pingrsp;
	pingrsp.set_retcode(Retcode::RET_SUCC);
	pingrsp.set_seq(pingreq->seq());
	pingrsp.set_clienttime(pingreq->clienttime());
	conn->send_packet(&pingrsp);
}

void handle_EnterSceneReadyReq(YSConnection *conn, const EnterSceneReadyReq *enterscenereadyreq) {
	EnterScenePeerNotify enterscenepeernotify;
	enterscenepeernotify.set_destsceneid(conn->player->current_location.sceneid);
	enterscenepeernotify.set_peerid(PLAYER_PEER_ID);
	enterscenepeernotify.set_hostpeerid(PLAYER_PEER_ID);
	conn->send_packet(&enterscenepeernotify);

	EnterSceneReadyRsp enterscenereadyrsp;
	enterscenereadyrsp.set_retcode(Retcode::RET_SUCC);
	conn->send_packet(&enterscenereadyrsp);
}

void handle_GetScenePointReq(YSConnection *conn, const GetScenePointReq *getscenepointreq) {
	// future: check belonguid in multiplayer

	const binoutput::ConfigScene *scene = &binoutput::scene_points.at(getscenepointreq->sceneid());

	GetScenePointRsp getscenepointrsp;
	getscenepointrsp.set_retcode(Retcode::RET_SUCC);
	getscenepointrsp.set_sceneid(getscenepointreq->sceneid());
	getscenepointrsp.set_belonguid(getscenepointreq->belonguid());
	for (const auto &it : scene->points) {
		getscenepointrsp.add_unlockedpointlist(it.first);
	}
	for (int i = 0; i < 5; i++) {
		getscenepointrsp.add_unlockarealist(i);
	}
	conn->send_packet(&getscenepointrsp);
}

void handle_GetSceneAreaReq(YSConnection *conn, const GetSceneAreaReq *getsceneareareq) {
	GetSceneAreaRsp getscenearearsp;
	getscenearearsp.set_retcode(Retcode::RET_SUCC);
	getscenearearsp.set_sceneid(getsceneareareq->sceneid());
	for (int i = 0; i < 5; i++) {
		getscenearearsp.add_areaidlist(i);
	}
	conn->send_packet(&getscenearearsp);
}

void handle_SceneInitFinishReq(YSConnection *conn, const SceneInitFinishReq *sceneinitfinishreq) {
	ScenePlayerInfoNotify sceneplayerinfonotify;
	ScenePlayerInfo *sceneplayerinfo = sceneplayerinfonotify.add_playerinfolist();
	sceneplayerinfo->set_uid(PLAYER_UID);
	sceneplayerinfo->set_peerid(PLAYER_PEER_ID);
	sceneplayerinfo->set_name(PLAYER_NAME);
	sceneplayerinfo->set_isconnected(true);
	conn->send_packet(&sceneplayerinfonotify);

	PlayerEnterSceneInfoNotify playerentersceneinfonotify;
	playerentersceneinfonotify.set_curavatarentityid(conn->player->get_current_avatar()->avatar_entity_id);
	for (int avatar_guid : conn->player->team_avatars) {
		YSAvatar *player_avatar = &conn->player->avatars.at(avatar_guid);

		AvatarEnterSceneInfo *avatarentersceneinfo = playerentersceneinfonotify.add_avatarenterinfo();
		avatarentersceneinfo->set_avatarguid(avatar_guid);
		avatarentersceneinfo->set_avatarentityid(player_avatar->avatar_entity_id);
		avatarentersceneinfo->mutable_avatarabilityinfo()->set_isinited(false);
		avatarentersceneinfo->set_weaponguid(player_avatar->weapon_guid);
		avatarentersceneinfo->set_weaponentityid(player_avatar->weapon_entity_id);
		avatarentersceneinfo->mutable_weaponabilityinfo()->set_isinited(false);
	}
	playerentersceneinfonotify.mutable_teamenterinfo()->set_teamentityid(conn->player->team_entity_id);
	playerentersceneinfonotify.mutable_teamenterinfo()->mutable_teamabilityinfo()->set_isinited(false);
	conn->send_packet(&playerentersceneinfonotify);

	PlayerGameTimeNotify playergametimenotify;
	playergametimenotify.set_gametime(5 * 60 * 60);
	playergametimenotify.set_uid(PLAYER_UID);
	conn->send_packet(&playergametimenotify);

	SceneTimeNotify scenetimenotify;
	scenetimenotify.set_sceneid(conn->player->current_location.sceneid);
	scenetimenotify.set_scenetime(9000);
	conn->send_packet(&scenetimenotify);

	HostPlayerNotify hostplayernotify;
	hostplayernotify.set_hostuid(PLAYER_UID);
	hostplayernotify.set_hostpeerid(PLAYER_PEER_ID);
	conn->send_packet(&hostplayernotify);

	SceneInitFinishRsp sceneinitfinishrsp;
	sceneinitfinishrsp.set_retcode(Retcode::RET_SUCC);
	conn->send_packet(&sceneinitfinishrsp);
}

void handle_ClientScriptEventNotify(YSConnection *conn, const ClientScriptEventNotify *clientscripteventnotify) {
	// ...
}

void handle_EnterSceneDoneReq(YSConnection *conn, const EnterSceneDoneReq *enterscenedonereq) {
	SceneEntityAppearNotify avatar_appear;
	conn->player->get_current_avatar()->make_scene_entity_info_proto(avatar_appear.add_entitylist());
	avatar_appear.set_appeartype(VisionType::VISION_MEET);
	conn->send_packet(&avatar_appear);

	ScenePlayerLocationNotify sceneplayerlocationnotify;
	PlayerLocationInfo *playerlocationinfo = sceneplayerlocationnotify.add_playerloclist();
	playerlocationinfo->set_uid(PLAYER_UID);
	pb_make_vector(playerlocationinfo->mutable_pos(), &conn->player->current_location.pos);
	pb_make_vector(playerlocationinfo->mutable_rot(), &conn->player->current_location.rot);
	conn->send_packet(&sceneplayerlocationnotify);

	conn->player->current_scene_load_state = SceneLoadState::DONE;
	conn->player->scene_flush_entities();

	EnterSceneDoneRsp enterscenedonersp;
	enterscenedonersp.set_retcode(Retcode::RET_SUCC);
	conn->send_packet(&enterscenedonersp);
}

void handle_PlayerSetPauseReq(YSConnection *conn, const PlayerSetPauseReq *playersetpausereq) {
	PlayerSetPauseRsp playersetpausersp;
	playersetpausersp.set_retcode(Retcode::RET_SUCC);
	conn->send_packet(&playersetpausersp);
}

void handle_SceneEntityMoveReq(YSConnection *conn, const SceneEntityMoveReq *sceneentitymovereq) {
	// the client sends this packet with position zero sometimes. don't record it.
	// possibly todo?: figure out all conditions this occurs in
	const Vector *pos = &sceneentitymovereq->motioninfo().pos();
	if (pos->x() == 0.0f && pos->y() == 0.0f && pos->z() == 0.0f) {
		return;
	}

	int avatar_entity_id = conn->player->get_current_avatar()->avatar_entity_id;
	// this check is to prevent SceneEntityMoveReq packets from being recorded and clobbering positions after teleport packets are sent.
	// in the future this would use the received entityId to determine whether to record movement.
	// all scene entities would be despawned as soon as a transpoint is used, so the entity id in this packet would be invalid.
	// for now, just ignore it if the scene isn't done loading.
	if (conn->player->current_scene_load_state == SceneLoadState::DONE) {
		if (sceneentitymovereq->entityid() == avatar_entity_id) {
			conn->player->current_location.pos = pb_convert_vector(pos);
			conn->player->current_location.rot = pb_convert_vector(&sceneentitymovereq->motioninfo().rot());
			conn->player->current_location.motionstate = sceneentitymovereq->motioninfo().state();
			conn->player->changed_position();
		}
	}
}

void handle_SceneTransToPointReq(YSConnection *conn, const SceneTransToPointReq *scenetranstopointreq) {
	const binoutput::ConfigScene *scene = &binoutput::scene_points.at(scenetranstopointreq->sceneid());
	const binoutput::ConfigScenePoint *point = &scene->points.at(scenetranstopointreq->pointid());

	conn->player->enter_scene(scenetranstopointreq->sceneid(), point->tranpos, point->tranrot);

	SceneTransToPointRsp scenetranstopointrsp;
	scenetranstopointrsp.set_retcode(Retcode::RET_SUCC);
	scenetranstopointrsp.set_sceneid(scenetranstopointreq->sceneid());
	scenetranstopointrsp.set_pointid(scenetranstopointreq->pointid());
	conn->send_packet(&scenetranstopointrsp);
}

void handle_PersonalSceneJumpReq(YSConnection *conn, const PersonalSceneJumpReq *personalscenejumpreq) {
	const binoutput::ConfigScene *scene = &binoutput::scene_points.at(conn->player->current_location.sceneid);
	const binoutput::ConfigScenePoint *point = &scene->points.at(personalscenejumpreq->pointid());

	conn->player->enter_scene(point->transceneid, point->tranpos, point->tranrot);

	PersonalSceneJumpRsp personalscenejumprsp;
	personalscenejumprsp.set_retcode(Retcode::RET_SUCC);
	personalscenejumprsp.set_destsceneid(point->transceneid);
	pb_make_vector(personalscenejumprsp.mutable_destpos(), &point->tranpos);
	conn->send_packet(&personalscenejumprsp);
}

void handle_DungeonEntryInfoReq(YSConnection *conn, const DungeonEntryInfoReq *dungeonentryinforeq) {
	DungeonEntryInfoRsp dungeonentryinforsp;
	dungeonentryinforsp.set_retcode(Retcode::RET_SUCC);
	dungeonentryinforsp.set_pointid(dungeonentryinforeq->pointid());

	const binoutput::ConfigScene *scene = &binoutput::scene_points.at(conn->player->current_location.sceneid);
	const binoutput::ConfigScenePoint *point = &scene->points.at(dungeonentryinforeq->pointid());

	for (const int dungeon_id : point->dungeon_ids) {
		const exceloutput::DungeonData *excel_dungeon = &exceloutput::dungeon_datas.at(dungeon_id);

		DungeonEntryInfo *dungeonentryinfo = dungeonentryinforsp.add_dungeonentrylist();
		dungeonentryinfo->set_dungeonid(dungeon_id);
		dungeonentryinfo->set_ispassed(false);
		dungeonentryinfo->set_lefttimes(excel_dungeon->daily_limit);
	}
	conn->send_packet(&dungeonentryinforsp);
}

void handle_PlayerEnterDungeonReq(YSConnection *conn, const PlayerEnterDungeonReq *playerenterdungeonreq) {
	const binoutput::ConfigScene *scene = &binoutput::scene_points.at(conn->player->current_location.sceneid);
	const binoutput::ConfigScenePoint *entry_point = &scene->points.at(playerenterdungeonreq->pointid());
	const binoutput::ConfigScenePoint *exit_point = &scene->points.at(entry_point->dungeon_exit_point_id);
	const exceloutput::DungeonData *dungeon = &exceloutput::dungeon_datas.at(playerenterdungeonreq->dungeonid());

	conn->player->return_location.sceneid = conn->player->current_location.sceneid;
	conn->player->return_location.pos = exit_point->pos;
	conn->player->return_location.rot = exit_point->rot;
	conn->player->enter_scene_spawn(dungeon->scene_id);

	PlayerEnterDungeonRsp playerenterdungeonrsp;
	playerenterdungeonrsp.set_retcode(Retcode::RET_SUCC);
	playerenterdungeonrsp.set_pointid(playerenterdungeonreq->pointid());
	playerenterdungeonrsp.set_dungeonid(playerenterdungeonreq->dungeonid());
	conn->send_packet(&playerenterdungeonrsp);
}

void handle_PlayerQuitDungeonReq(YSConnection *conn, const PlayerQuitDungeonReq *playerquitdungeonreq) {
	conn->player->enter_scene(conn->player->return_location.sceneid, conn->player->return_location.pos, conn->player->return_location.rot);

	PlayerQuitDungeonRsp playerquitdungeonrsp;
	playerquitdungeonrsp.set_retcode(Retcode::RET_SUCC);
	conn->send_packet(&playerquitdungeonrsp);
}

void handle_ChangeAvatarReq(YSConnection *conn, const ChangeAvatarReq *changeavatarreq) {
	if (std::find(conn->player->team_avatars.cbegin(), conn->player->team_avatars.cend(), changeavatarreq->guid()) == conn->player->team_avatars.cend()) {
		conn->send_empty_rsp_with_retcode<ChangeAvatarRsp>(Retcode::RET_AVATAR_NOT_EXIST_IN_TEAM);
		return;
	}

	conn->player->switch_avatar(changeavatarreq->guid());

	ChangeAvatarRsp changeavatarrsp;
	changeavatarrsp.set_retcode(Retcode::RET_SUCC);
	changeavatarrsp.set_curguid(changeavatarreq->guid());
	conn->send_packet(&changeavatarrsp);
}

void handle_GetAllMailReq(YSConnection *conn, const GetAllMailReq *getallmailreq) {
	GetAllMailRsp getallmailrsp;
	MailData *maildata = getallmailrsp.add_maillist();
	maildata->set_mailid(1);
	EquipParam *item = maildata->add_itemlist();
	item->set_itemid(202); // mora
	item->set_itemnum(5);
	MailTextContent *mailtextcontent = maildata->mutable_mailtextcontent();
	mailtextcontent->set_sender("\x47\x65\x6e\x73\x68\x69\x6e \x49\x6d\x70\x61\x63\x74");
	mailtextcontent->set_title("Dear Travelers~");
	mailtextcontent->set_content("Sorry for the short notice! We will be removing 5 star character <color=#00ffff>[Kamisato Ayaka]</color> from the game tomorrow at 18:00 (UTC-8)! As compensation we will be sending out 5 mora. Stay homosexual");
	maildata->set_sendtime(1648800000); // apr 1, 2022
	maildata->set_expiretime(0);
	maildata->set_isread(true);
	maildata->set_isattachmentgot(false);
	getallmailrsp.set_retcode(Retcode::RET_SUCC);
	conn->send_packet(&getallmailrsp);
}

void handle_GetShopReq(YSConnection *conn, const GetShopReq *getshopreq) {
	GetShopRsp getshoprsp;
	getshoprsp.set_retcode(Retcode::RET_SUCC);
	getshoprsp.mutable_shop()->set_shoptype(getshopreq->shoptype());

	for (const auto &it : exceloutput::shop_plan_datas) {
		const exceloutput::ShopPlanData *excel_shop_plan = &it.second;

		if (excel_shop_plan->shop_type == getshopreq->shoptype()) {
			const exceloutput::ShopGoodsData *excel_shop_goods = &exceloutput::shop_goods_datas.at(excel_shop_plan->goods_id);

			ShopGoods *goods = getshoprsp.mutable_shop()->add_goodslist();
			goods->set_goodsid(excel_shop_plan->goods_id);
			goods->mutable_goodsitem()->set_itemid(excel_shop_goods->result_item_id);
			goods->mutable_goodsitem()->set_count(excel_shop_goods->result_item_count);
			goods->set_scoin(excel_shop_goods->mora_cost);
			goods->set_hcoin(excel_shop_goods->primogem_cost);
			goods->set_boughtnum(0);
			goods->set_buylimit(excel_shop_goods->max_purchases);
			if (excel_shop_goods->consume_1_item_id != 0) {
				ItemParam *cost = goods->add_costitemlist();
				cost->set_itemid(excel_shop_goods->consume_1_item_id);
				cost->set_count(excel_shop_goods->consume_1_item_count);
			}
			if (excel_shop_goods->consume_2_item_id != 0) {
				ItemParam *cost = goods->add_costitemlist();
				cost->set_itemid(excel_shop_goods->consume_2_item_id);
				cost->set_count(excel_shop_goods->consume_2_item_count);
			}
		}
	}

	conn->send_packet(&getshoprsp);
}

void handle_WearEquipReq(YSConnection *conn, const WearEquipReq *wearequipreq) {
	// on wording in this function:
	// "target avatar" is the on-screen avatar
	// "target equip" is the desired item to equip onto the on-screen avatar
	// "swap avatar" is the off-screen avatar using the "target equip" prior to this transaction
	// "swap equip" is the equip that the on-screen avatar has prior to this transaction. the equip that the off-screen (swap) avatar will have after this transaction

	int target_avatar_guid = wearequipreq->avatarguid();
	int target_equip_guid = wearequipreq->equipguid();

	YSAvatar *target_avatar = &conn->player->avatars.at(target_avatar_guid);
	YSPlayerItem *target_equip = &conn->player->items.at(target_equip_guid);

	const exceloutput::ItemData *excel_target_equip = &exceloutput::item_datas.at(target_equip->item_id);
	EquipType equip_type = excel_target_equip->equip_type;

	YSAvatar *swap_avatar = NULL;
	YSPlayerItem *swap_equip = NULL;
	const auto &it = conn->player->equips_ref.find(target_equip_guid);
	if (it != conn->player->equips_ref.cend()) {
		// target equip is being used by another avatar
		int swap_avatar_guid = it->second;
		swap_avatar = &conn->player->avatars.at(swap_avatar_guid);
		// swap equip may be null if the target avatar isn't wearing anything
		int swap_equip_guid = target_avatar->get_equip_guid_by_slot(equip_type);
		if (swap_equip_guid != 0) {
			swap_equip = &conn->player->items.at(swap_equip_guid);
		}
	}

	// target avatar equipping the target equip
	target_avatar->do_equip(target_equip);

	if (swap_avatar != NULL) {
		// swap avatar equipping the swap equip
		if (swap_equip == NULL) {
			swap_avatar->do_unequip(equip_type);
		} else {
			swap_avatar->do_equip(swap_equip);
		}
	}

	WearEquipRsp wearequiprsp;
	wearequiprsp.set_retcode(Retcode::RET_SUCC);
	wearequiprsp.set_avatarguid(target_avatar_guid);
	wearequiprsp.set_equipguid(target_equip_guid);
	conn->send_packet(&wearequiprsp);
}

void handle_TakeoffEquipReq(YSConnection *conn, const TakeoffEquipReq *takeoffequipreq) {
	int avatar_guid = takeoffequipreq->avatarguid();
	EquipType equip_type = (EquipType)takeoffequipreq->slot();

	if (equip_type == EquipType::EQUIP_WEAPON) {
		conn->send_empty_rsp_with_retcode<TakeoffEquipRsp>(Retcode::RET_EQUIP_CAN_NOT_WAKE_OFF_WEAPON);
		return;
	}

	YSAvatar *avatar = &conn->player->avatars.at(avatar_guid);
	avatar->do_unequip(equip_type);

	TakeoffEquipRsp takeoffequiprsp;
	takeoffequiprsp.set_retcode(Retcode::RET_SUCC);
	takeoffequiprsp.set_avatarguid(avatar_guid);
	takeoffequiprsp.set_slot(equip_type);
	conn->send_packet(&takeoffequiprsp);
}

void handle_SetUpAvatarTeamReq(YSConnection *conn, const SetUpAvatarTeamReq *setupavatarteamreq) {
	const AvatarTeam *req_avatarteam = &setupavatarteamreq->avatarteam();

	if (req_avatarteam->avatarguidlist().size() == 0) {
		conn->send_empty_rsp_with_retcode<SetUpAvatarTeamRsp>(Retcode::RET_CAN_NOT_REMOVE_CUR_AVATAR_FROM_TEAM);
		return;
	}

	conn->player->team_avatars.clear();
	for (int guid : req_avatarteam->avatarguidlist()) {
		conn->player->team_avatars.push_back(guid);
	}

	AvatarTeamUpdateNotify avatarteamupdatenotify;
	AvatarTeam *avatarteam = &(*avatarteamupdatenotify.mutable_avatarteammap())[PLAYER_TEAM_ID];
	for (int guid : conn->player->team_avatars) {
		avatarteam->add_avatarguidlist(guid);
		YSAvatar *avatar = &conn->player->avatars.at(guid);
		avatarteamupdatenotify.mutable_avatarentityidmap()->insert({ (uint64_t)guid, (uint32_t)avatar->avatar_entity_id });
	}
	conn->send_packet(&avatarteamupdatenotify);

	// this holds the guid of the on-field avatar prior to this transaction.
	// a new avatar needs to be respawned if this avatar is removed from the team.
	int prev_field_avatar_guid = conn->player->team_current_avatar_guid;

	if (std::find(conn->player->team_avatars.cbegin(), conn->player->team_avatars.cend(), prev_field_avatar_guid) == conn->player->team_avatars.cend()) {
		// switch to avatar at index 0
		conn->player->switch_avatar(conn->player->team_avatars.at(0));
	}

	SetUpAvatarTeamRsp setupavatarteamrsp;
	setupavatarteamrsp.set_retcode(Retcode::RET_SUCC);
	setupavatarteamrsp.set_teamid(PLAYER_TEAM_ID);
	setupavatarteamrsp.mutable_avatarteam()->CopyFrom(*req_avatarteam);
	setupavatarteamrsp.set_curavatarguid(setupavatarteamreq->curavatarguid());
	conn->send_packet(&setupavatarteamrsp);
}

void handle_SceneGetAreaExplorePercentReq(YSConnection *conn, const SceneGetAreaExplorePercentReq *scenegetareaexplorepercentreq) {
	SceneGetAreaExplorePercentRsp scenegetareaexplorepercentrsp;
	scenegetareaexplorepercentrsp.set_retcode(Retcode::RET_SUCC);
	scenegetareaexplorepercentrsp.set_areaid(scenegetareaexplorepercentreq->areaid());
	scenegetareaexplorepercentrsp.set_explorepercent(0.01f);
	conn->send_packet(&scenegetareaexplorepercentrsp);
}

void handle_BuyGoodsReq(YSConnection *conn, const BuyGoodsReq *buygoodsreq) {
	conn->send_empty_rsp_with_retcode<BuyGoodsRsp>(Retcode::RET_GOODS_NOT_EXIST);
}

void handle_ChangeGameTimeReq(YSConnection *conn, const ChangeGameTimeReq *changehametimereq) {
	SceneTimeNotify scenetimenotify;
	scenetimenotify.set_sceneid(conn->player->current_location.sceneid);
	scenetimenotify.set_scenetime(changehametimereq->gametime());
	scenetimenotify.set_ispaused(false);
	conn->send_packet(&scenetimenotify);

	ChangeGameTimeRsp changegametimersp;
	changegametimersp.set_curgametime(changehametimereq->gametime());
	changegametimersp.set_retcode(Retcode::RET_SUCC);
	conn->send_packet(&changegametimersp);
}


bool queued_shutdown = false;

void soggy_shutdown() {
	interrupt_dispatch_server();
	queued_shutdown = true;
}

void print_socket_error(const char *str) {
#ifdef _WIN32
	char msg[256];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, WSAGetLastError(), 0, msg, 256, 0);
	printf("%s: %s\n", str, msg);
#else
	perror(str);
#endif
}

void game_server_main() {
	// future: for multiple connections
	//std::map<int, YSConnection> connections;

	enet_initialize();

	ENetAddress addr;
	enet_address_set_host(&addr, soggy_cfg.game_bind_addr.c_str());
	addr.port = soggy_cfg.game_bind_port;

	ENetHost *host = enet_host_create(&addr, /*peerCount*/ soggy_cfg.game_max_clients, /*channelLimit*/ 0, /*incomingBandwidth*/ 0, /*outgoingBandwidth*/ 0);
	if (host == NULL) {
		print_socket_error("enet_create_host");
		exit(1);
	}
	enet_host_compress_with_range_coder(host);
	host->checksum = enet_crc32;

	soggy_log("game listening on port %d", addr.port);

	ENetEvent ev;
	while (!queued_shutdown) {
		int ret = enet_host_service(host, &ev, 20); // setting this timeout to 0 blasts the cpu
		if (ret < 0) {
			print_socket_error("enet_host_service");
			break;
		}

		// future: look this up based on ev.peer
		YSConnection *conn = &mainconn;

		switch (ev.type) {
			case ENET_EVENT_TYPE_CONNECT: {
				conn->player = &mainplayer;
				conn->player->conn = conn;
				conn->peer = ev.peer;
				soggy_log("\x1b[97mEVENT_TYPE_CONNECT\x1b[0m");
				break;
			}
			case ENET_EVENT_TYPE_DISCONNECT: {
				conn->player = NULL;
				conn->peer = NULL;
				soggy_log("\x1b[97mEVENT_TYPE_DISCONNECT\x1b[0m");
				break;
			}
			case ENET_EVENT_TYPE_RECEIVE: {
				unsigned short cmdid;
				const google::protobuf::Message *body = decode_packet(ev.packet, &cmdid);
				if (body == NULL) {
					break;
				}

				if (dont_log_cmds.find(cmdid) == dont_log_cmds.end()) {
					const char *cmdname = cmdids::cmd_id_to_detail[cmdid]->name.c_str();
					std::string json_string;
					google::protobuf::util::MessageToJsonString(*body, &json_string);
					soggy_log(COLOR_CLIENT_TO_SERVER "[client->server] %hu %s %s" COLOR_RESET, cmdid, cmdname, json_string.c_str());
				}

// handle a packet, sending a specific rsp packet if an exception is caught
#define HANDLE_RSP(req_cmdname, rsp_cmdname) \
					case CMD_ID_##req_cmdname: \
						try { \
							handle_##req_cmdname(conn, reinterpret_cast<const req_cmdname *>(body)); \
						} catch (const std::exception &exc) {\
							soggy_log("%s", exc.what()); \
							conn->send_empty_rsp_with_retcode<rsp_cmdname>(Retcode::RET_FAIL); \
						} \
						break;

// handle a packet
#define HANDLE(req_cmdname) \
					case CMD_ID_##req_cmdname: \
						try { \
							handle_##req_cmdname(conn, reinterpret_cast<const req_cmdname *>(body)); \
						} catch (const std::exception &exc) {\
							soggy_log("%s", exc.what()); \
						} \
						break;

				switch (cmdid) {
					HANDLE(PingReq)
					HANDLE_RSP(GetPlayerTokenReq, GetPlayerTokenRsp)
					HANDLE_RSP(PlayerLoginReq, PlayerLoginRsp)
					HANDLE_RSP(EnterSceneReadyReq, EnterSceneReadyRsp)
					HANDLE_RSP(GetScenePointReq, GetScenePointRsp)
					HANDLE_RSP(GetSceneAreaReq, GetSceneAreaRsp)
					HANDLE_RSP(SceneInitFinishReq, SceneInitFinishRsp)
					HANDLE_RSP(EnterSceneDoneReq, EnterSceneDoneRsp)
					HANDLE_RSP(PlayerSetPauseReq, PlayerSetPauseRsp)
					HANDLE_RSP(SceneTransToPointReq, SceneTransToPointRsp)
					HANDLE(SceneEntityMoveReq)
					HANDLE_RSP(ChangeAvatarReq, ChangeAvatarRsp)
					HANDLE_RSP(GetAllMailReq, GetAllMailRsp)
					HANDLE_RSP(GetShopReq, GetShopRsp)
					HANDLE_RSP(WearEquipReq, WearEquipRsp)
					HANDLE_RSP(TakeoffEquipReq, TakeoffEquipRsp)
					HANDLE_RSP(SetUpAvatarTeamReq, SetUpAvatarTeamRsp)
					HANDLE_RSP(SceneGetAreaExplorePercentReq, SceneGetAreaExplorePercentRsp)
					HANDLE_RSP(BuyGoodsReq, BuyGoodsRsp)
					HANDLE(ClientScriptEventNotify)
					HANDLE_RSP(ChangeGameTimeReq, ChangeGameTimeRsp)
					HANDLE_RSP(PersonalSceneJumpReq, PersonalSceneJumpRsp)
					HANDLE_RSP(DungeonEntryInfoReq, DungeonEntryInfoRsp)
					HANDLE_RSP(PlayerEnterDungeonReq, PlayerEnterDungeonRsp)
					HANDLE_RSP(PlayerQuitDungeonReq, PlayerQuitDungeonRsp)
				}
#undef HANDLE

				delete body;
				break;
			}
			default:
				break;
		}
	}

	enet_host_destroy(host);
	enet_deinitialize();
}

typedef void (*soggy_rlcmd_proc_with_target)(YSConnection *target, std::string label, std::vector<std::string> args);
typedef void (*soggy_rlcmd_proc_no_target)(std::string label, std::vector<std::string> args);

struct RlCmdProcDesc {
	bool need_target;
	union {
		soggy_rlcmd_proc_with_target proc_with_target;
		soggy_rlcmd_proc_no_target proc_no_target;
	};
};
std::unordered_map<std::string, RlCmdProcDesc> rlcmd_map;

std::vector<std::string> string_split(std::string str, char sep = ' ') {
	std::vector<std::string> parts;
	std::istringstream iss(str);
	std::string token;
	while (std::getline(iss, token, sep)) {
		parts.emplace_back(token);
	}
	return parts;
}

template <typename U>
struct NumRel {
	U value;
	bool relative;
	inline U resolve(U base) {
		if (this->relative) {
			return base + this->value;
		} else {
			return this->value;
		}
	}
};

template <typename T>
struct Parse {
	static void parse_one(std::string arg, T *out);
};
template <> void Parse<int>::parse_one(std::string arg, int *out) { *out = std::stoi(arg); }
template <> void Parse<float>::parse_one(std::string arg, float *out) { *out = std::stof(arg); }
template <> void Parse<std::string>::parse_one(std::string arg, std::string *out) { *out = arg; }

template <typename U>
struct Parse<std::optional<U>> {
	static void parse_one(std::string arg, std::optional<U> *out) {
		if (!arg.empty()) {
			U value;
			Parse<U>::parse_one(arg, &value);
			*out = std::optional<U>(value);
		}
	}
};

template <typename U>
struct Parse<NumRel<U>> {
	static void parse_one(std::string arg, NumRel<U> *out) {
		out->relative = (arg[0] == '~');
		if (out->relative) {
			if (arg.size() == 1) {
				out->value = U();
			} else {
				Parse<U>::parse_one(arg.substr(1), &out->value);
			}
		} else {
			Parse<U>::parse_one(arg, &out->value);
		}
	}
};

template <typename... Args>
void parse_args(std::vector<std::string> args_in, Args *...args_out) {
	while (args_in.size() < sizeof...(args_out)) {
		args_in.emplace_back();
	}
	int i = 0;
	([&]{
		Parse<Args>::parse_one(args_in[i++], args_out);
	}(), ...);
}

void cmd_warp(YSConnection *target, std::string label, std::vector<std::string> args) {
	if (args.size() < 3) {
		soggy_log("usage: %s <x> <y> <z> [<sceneid>]", label.c_str());
		return;
	}
	NumRel<float> x;
	NumRel<float> y;
	NumRel<float> z;
	std::optional<int> sceneid_opt;

	try {
		parse_args(args, &x, &y, &z, &sceneid_opt);
	} catch (std::exception &) {
		soggy_log("%s: failed to parse arguments", label.c_str());
		return;
	}

	int sceneid;
	if (sceneid_opt.has_value()) {
		sceneid = sceneid_opt.value();
		try {
			exceloutput::scene_datas.at(sceneid);
		} catch (std::out_of_range &) {
			soggy_log("%s: no such scene %d", label.c_str(), sceneid);
			return;
		}
	} else {
		sceneid = target->player->current_location.sceneid;
	}

	Vec3f pos;
	pos.x = x.resolve(target->player->current_location.pos.x);
	pos.y = y.resolve(target->player->current_location.pos.y);
	pos.z = z.resolve(target->player->current_location.pos.z);
	target->player->enter_scene(sceneid, pos, Vec3f());
}

// warp to spawn of scene
void cmd_scene(YSConnection *target, std::string label, std::vector<std::string> args) {
	if (args.size() < 1) {
		soggy_log("usage: %s <sceneid>", label.c_str());
		return;
	}

	int sceneid;
	try {
		parse_args(args, &sceneid);
	} catch (std::exception &) {
		soggy_log("%s: failed to parse arguments", label.c_str());
		return;
	}

	try {
		exceloutput::scene_datas.at(sceneid);
	} catch (std::out_of_range &) {
		soggy_log("%s: no such scene %d", label.c_str(), sceneid);
		return;
	}

	target->player->enter_scene_spawn(sceneid);
}

void cmd_help(std::string label, std::vector<std::string> args) {
	soggy_log("commands available:");
	for (const auto &it : rlcmd_map) {
		const char *cmdname = it.first.c_str();
		soggy_log("- %s", cmdname);
	}
}

void cmd_stop(std::string label, std::vector<std::string> args) {
	soggy_shutdown();
}

void cmd_elfie(YSConnection *target, std::string label, std::vector<std::string> args) {
	SceneEntityAppearNotify appear;
	appear.set_appeartype(VisionType::VISION_MEET);

	SceneEntityInfo *entityinfo = appear.add_entitylist();
	entityinfo->set_entitytype(ProtEntityType::PROT_ENTITY_NPC);
	entityinfo->set_entityid(get_next_runtime_entity_id(RuntimeIDCategory::NPC_CATE));
	pb_make_vector(entityinfo->mutable_motioninfo()->mutable_pos(), &target->player->current_location.pos);
	entityinfo->mutable_motioninfo()->mutable_rot();
	entityinfo->mutable_motioninfo()->mutable_speed();
	entityinfo->set_lifestate(LifeState::LIFE_ALIVE);
	entityinfo->mutable_abilityinfo()->set_isinited(false);
	SceneNpcInfo *npcinfo = entityinfo->mutable_npc();
	npcinfo->set_npcid(1469); // elfie

	target->send_packet(&appear);

	soggy_log("elfie :)");
}

void cmd_switchelement(YSConnection *target, std::string label, std::vector<std::string> args) {
	if (args.size() < 1) {
		soggy_log("usage: se <element>");
		return;
	}

	YSAvatar *avatar = target->player->get_current_avatar();

	int depot_base;
	switch (avatar->avatar_id) {
		case 10000005: depot_base = 500; break;
		case 10000007: depot_base = 700; break;
		default:
			soggy_log("se: active avatar must be lumine or aether");
			return;
	}

	std::string *element_name = &args[0];

	int depot_offset;
	if (!element_name->compare("anemo")) {
		depot_offset = 4;
	} else if (!element_name->compare("geo")) {
		depot_offset = 6;
	} else if (!element_name->compare("none")) {
		depot_offset = 1;
	} else {
		soggy_log("se: invalid element \"%s\"", element_name->c_str());
		return;
	}

	avatar->skill_depot_id = depot_base + depot_offset;
	avatar->recalc_talent_ids();

	AvatarSkillDepotChangeNotify avatarskilldepotchangenotify;
	avatarskilldepotchangenotify.set_skilldepotid(avatar->skill_depot_id);
	avatarskilldepotchangenotify.set_avatarguid(avatar->avatar_guid);
	avatarskilldepotchangenotify.set_entityid(avatar->avatar_entity_id);
	for (int talent_id : avatar->talent_ids) {
		avatarskilldepotchangenotify.add_talentidlist(talent_id);
	}
	target->send_packet(&avatarskilldepotchangenotify);
}

void cmd_pos(YSConnection *target, std::string label, std::vector<std::string> args) {
	const char *state;
	switch (target->player->current_scene_load_state) {
		case SceneLoadState::DONE: state = "done";
		case SceneLoadState::LOADING_NEW_SCENE: state = "loading new scene";
		case SceneLoadState::LOADING_SAME_SCENE: state = "loading same scene";
	}
	soggy_log("sceneid=%d (%s)", target->player->current_location.sceneid, state);
	soggy_log("pos x=%.3f y=%.3f z=%.3f", target->player->current_location.pos.x, target->player->current_location.pos.y, target->player->current_location.pos.z);
	soggy_log("rot x=%.3f y=%.3f z=%.3f", target->player->current_location.rot.x, target->player->current_location.rot.y, target->player->current_location.rot.z);
}

// todo commands:

// packetlog on -> enable packet logging
// packetlog off -> disable packet logging
// packetlog ignore/unignore -> show ignores
// packetlog ignore CmdNames... -> add CmdNames... to ignore
// packetlog unignore CmdNames... -> remove CmdNames... from ignore

// avatars add ...
// avatars remove ...
// team add ...
// team remove ...

void run_cmd(YSConnection *target, std::string label, std::string args_line = std::string()) {
	auto it = rlcmd_map.find(label);
	if (it != rlcmd_map.end()) {
		RlCmdProcDesc *rlcmd = &it->second;
		if (rlcmd->need_target) {
			if (target == NULL) {
				soggy_log("command \"%s\" requires a target", label.c_str());
			} else {
				rlcmd->proc_with_target(target, label, string_split(args_line));
			}
		} else {
			rlcmd->proc_no_target(label, string_split(args_line));
		}
	} else {
		soggy_log("no such command \"%s\"", label.c_str());
	}
}

void rlcmd_add_with_target(std::string label, soggy_rlcmd_proc_with_target proc) {
	RlCmdProcDesc *desc = &rlcmd_map[label];
	desc->need_target = true;
	desc->proc_with_target = proc;
}
void rlcmd_add_no_target(std::string label, soggy_rlcmd_proc_no_target proc) {
	RlCmdProcDesc *desc = &rlcmd_map[label];
	desc->need_target = false;
	desc->proc_no_target = proc;
}

void execute_repl_line(std::string line) {
	if (line[0] == '#') {
		return;
	}

	// target the main connection if it's online. otherwise have no target
	// in the future this this would be changed with a "target" command, like in grasscutter
	YSConnection *conn = mainconn.peer == NULL ? NULL : &mainconn;

	size_t first_word_begin_idx = line.find_first_not_of(" /!.");
	if (first_word_begin_idx == std::string::npos) {
		return;
	}
	size_t first_word_end_idx = line.find(' ', first_word_begin_idx);
	if (first_word_end_idx == std::string::npos) {
		std::string cmdname = line.substr(first_word_begin_idx);
		run_cmd(conn, cmdname);
		return;
	}
	std::string cmdname = line.substr(first_word_begin_idx, first_word_end_idx - first_word_begin_idx);
	size_t second_word_begin_idx = line.find_first_not_of(" ", first_word_end_idx);
	if (second_word_begin_idx == std::string::npos) {
		run_cmd(conn, cmdname);
		return;
	}
	std::string argline = line.substr(second_word_begin_idx);
	run_cmd(conn, cmdname, argline);
}

void interactive_main() {
	// interactive main: run the repl

	rlcmd_add_no_target("help", cmd_help);
	rlcmd_add_no_target("stop", cmd_stop);
	rlcmd_add_with_target("elfie", cmd_elfie);
	rlcmd_add_with_target("se", cmd_switchelement);
	rlcmd_add_with_target("pos", cmd_pos);
	rlcmd_add_with_target("warp", cmd_warp);
	rlcmd_add_with_target("scene", cmd_scene);

	soggy_rx.install_window_change_handler();

	signal(SIGINT, [](int) -> void {
		std::thread interrupt([] {
			soggy_rx.emulate_key_press(replxx::Replxx::KEY::control('U'));
			soggy_rx.emulate_key_press(replxx::Replxx::KEY::control('K'));
			soggy_rx.emulate_key_press(replxx::Replxx::KEY::control('D'));
		});
		interrupt_dispatch_server();
		interrupt.join();
	});

	{
		std::ifstream historyf(HISTORY_FILE_NAME);
		soggy_rx.history_load(historyf);
	}

	while (!queued_shutdown) {
		const char *line;
		do {
			line = soggy_rx.input("\x1b[90msoggy > \x1b[0m");
		} while (line == NULL && errno == EAGAIN);
		if (line != NULL) {
			if (line[0] != '\0') {
				soggy_rx.history_add(line);
				execute_repl_line(std::string(line));
			}
		} else {
			printf("EOF\n");
			soggy_shutdown();
		}
	}

	soggy_rx.history_sync(HISTORY_FILE_NAME);
}

void non_interactive_main() {
	// non-interactive main: just wait until SIGINT is received

	static std::condition_variable cond;
	static bool sigint_handled = false;

	signal(SIGINT, [](int) -> void {
		sigint_handled = true;
		cond.notify_one();
	});

	std::mutex mutex;
	std::unique_lock lock(mutex);
	cond.wait(lock, []{ return sigint_handled; });
}

int main(int argc, char *argv[]) {
	puts("█ Soggy is free software: you can redistribute it and/or modify it under the terms of");
	puts("█   the GNU Affero General Public License as published by the Free Software Foundation,");
	puts("█   either version 3 of the License, or (at your option) any later version.");
	puts("█ Homepage: https://github.com/LDAsuku/soggy");

	if (soggy_load_config_file("soggy.cfg") == -1) {
		soggy_log("error: config file soggy.cfg not found");
		return 1;
	}

	if (load_game_data()) {
		soggy_log("error loading resources");
		return 1;
	}

	init_mainplayer(); // initialize a demo player

	std::thread game_server(&game_server_main);
	std::thread dispatch_server(&dispatch_server_main);

	if (isatty(fileno(stdout))) {
		interactive_main();
	} else {
		non_interactive_main();
	}

	soggy_shutdown();
	game_server.join();
	dispatch_server.join();

	printf("exit\n");

	return 0;
}
