// game_data.hpp

#pragma once

// structs for exceloutput and binoutput

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "game_enums.hpp"
#include "vec.hpp"

extern bool load_game_data();

extern bool load_scene_script_data(int sceneid);

namespace exceloutput {

struct AvatarData {
	int id; //ID
	int skill_depot_id; //技能库ID
	std::vector<int> candidate_skill_depot_ids; //候选技能库ID
	int initial_weapon_id; //初始武器
	WeaponType weapon_type; //武器种类
	AvatarUseType use_type; //是否使用
};

struct AvatarSkillDepotData {
	int id; //ID
	int leader_talent; //队长天赋
	std::vector<int> talent_groups; //天赋组x
};

struct AvatarSkillData {
	int id; //ID
	ElementType energy_type; //消耗能量类型
	int energy_cost; //消耗能量值
	int max_charges; //可累积次数
};

struct ItemData {
	int id; //ID
	ItemType type; //类型
	EquipType equip_type; //圣遗物类别 for ReliquaryData, implicit for others
	struct {} material;
	struct {
		int gadget_id; //物件ID
		WeaponType weapon_type; //武器种类
	} weapon;
	struct {} reliquary;
};

struct ShopPlanData {
	int id; //ID
	int shop_type; //商店类型
	int goods_id; //商品ID
};

struct ShopGoodsData {
	int id; //商品ID
	int result_item_id; //对应物品ID
	int result_item_count; //对应物品数量
	int mora_cost; //消耗金币
	int primogem_cost; //消耗水晶
	int consume_1_item_id; //[消耗物品]1ID
	int consume_1_item_count; //[消耗物品]1数量
	int consume_2_item_id; //[消耗物品]2ID
	int consume_2_item_count; //[消耗物品]2数量
	int max_purchases; //限购数量
};

struct TalentSkillData {
	int id; //天赋ID
	int talent_group_id; //天赋组ID
	int rank; //等级
};

struct GadgetData {
	int id; //ID
	int default_camp; //默认阵营
	bool is_interactive; //能否交互
	EntityType entity_type; //类型
};

struct MonsterData {
	int id; //ID
	int equip_1; //装备1
	int equip_2; //装备2
};

struct NpcData {
	int id; //ID
};

struct SceneData {
	int id; //ID
	SceneType type; //类型
};

extern std::unordered_map<int, AvatarData> avatar_datas;
extern std::unordered_map<int, AvatarSkillDepotData> avatar_skill_depot_datas;
extern std::unordered_map<int, AvatarSkillData> avatar_skill_datas;
extern std::unordered_map<int, ItemData> item_datas; // consolidated MaterialData, WeaponData, and ReliquaryData
extern std::unordered_map<int, ShopPlanData> shop_plan_datas;
extern std::unordered_map<int, ShopGoodsData> shop_goods_datas;
extern std::unordered_map<int, TalentSkillData> talent_skill_datas;
extern std::unordered_map<int, GadgetData> gadget_datas;
extern std::unordered_map<int, MonsterData> monster_datas;
extern std::unordered_map<int, NpcData> npc_datas;
extern std::unordered_map<int, SceneData> scene_datas;

extern std::unordered_map<std::string, std::string> text_map;

}

namespace binoutput {

struct ConfigScenePoint {
	std::string $type;
	Vec3f tranpos;
	Vec3f tranrot;
	int transceneid = 0; // only used by PersonalSceneJumpPoint
};

struct ConfigScene {
	std::unordered_map<int, ConfigScenePoint> points;
};

extern std::unordered_map<int, ConfigScene> scene_points;

}

namespace luares {

// future: implement suites

struct SceneEntity {
	int config_id = 0;
	Vec3f pos;
	Vec3f rot;
	struct {
		int level = 0;
		int gadget_id = 0;
		int route_id = 0;
		bool save_route = false;
		bool persistent = false;
		GadgetState state = GadgetState::GADGET_STATE_Default;
		int chest_drop_id = 0; // only used by chests
		bool is_one_off = false;
		GadgetType type = GadgetType::GADGET_NONE;
		int point_type = 0; // only used by gatherpoints
		int owner = 0; // only used by gatherpoints
	} gadget;
	struct {
		int level = 0;
		int monster_id = 0;
		bool disable_wander = false;
		int drop_id = 0;
	} monster;
	struct {
		int npc_id = 0;
		int point_id = 0;
	} npc;
};

//struct SceneSuite {
//	std::vector<int> gadgets;
//	std::vector<int> monsters;
//	std::vector<int> npcs;
//};

struct SceneGroup {
	int id;
	Vec3f pos;
	int area = 0;
	int refresh_time = 0;
	bool refresh_with_block = true;
	std::vector<SceneEntity> gadgets;
	std::vector<SceneEntity> monsters;
	std::vector<SceneEntity> npcs;
	//std::vector<SceneSuite> suites;
};

struct SceneBlock {
	int id;
	Vec2f min;
	Vec2f max;
	std::unordered_map<int, SceneGroup> groups;
};

struct SceneConfig {
	Vec3f born_pos;
	Vec3f born_rot;
	float die_y;
	std::unordered_map<int, SceneBlock> blocks;
};

extern std::unordered_map<int, SceneConfig> scene_configs;

};
