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

namespace exceloutput {

struct AvatarData {
	int id = 0; //ID
	int skill_depot_id = 0; //技能库ID
	int initial_weapon_id = 0; //初始武器
	WeaponType weapon_type = (WeaponType)0; //武器种类
	AvatarUseType use_type = (AvatarUseType)0; //是否使用
};

struct AvatarSkillDepotData {
	int id = 0; //ID
	int leader_talent = 0; //队长天赋
	std::vector<int> talent_groups; //天赋组x
};

struct AvatarSkillData {
	int id = 0; //ID
	ElementType energy_type = (ElementType)0; //消耗能量类型
	int energy_cost = 0; //消耗能量值
	int max_charges = 1; //可累积次数
};

struct ItemData {
	int id = 0; //ID
	ItemType type = (ItemType)0; //类型
	EquipType equip_type; //圣遗物类别 for ReliquaryData, implicit for others
	struct {} material;
	struct {
		int gadget_id = 0; //物件ID
		WeaponType weapon_type = (WeaponType)0; //武器种类
	} weapon;
	struct {} reliquary;
};

struct ShopPlanData {
	int id = 0; //ID
	int shop_type = 0; //商店类型
	int goods_id = 0; //商品ID
};

struct ShopGoodsData {
	int id = 0; //商品ID
	int result_item_id = 0; //对应物品ID
	int result_item_count = 0; //对应物品数量
	int mora_cost = 0; //消耗金币
	int primogem_cost = 0; //消耗水晶
	int consume_1_item_id = 0; //[消耗物品]1ID
	int consume_1_item_count = 0; //[消耗物品]1数量
	int consume_2_item_id = 0; //[消耗物品]2ID
	int consume_2_item_count = 0; //[消耗物品]2数量
	int max_purchases = 0; //限购数量
};

struct TalentSkillData {
	int id = 0; //天赋ID
	int talent_group_id = 0; //天赋组ID
	int rank = 0; //等级
};

struct GadgetData {
	int id = 0; //ID
	int default_camp = 0; //默认阵营
	bool is_interactive = false; //能否交互
};

struct MonsterData {
	int id = 0; //ID
};

struct NpcData {
	int id = 0; //ID
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

extern std::unordered_map<std::string, std::string> text_map;

}

namespace binoutput {

struct ConfigScenePoint {
	std::string type = "";
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
// future: need to actually implement suites

struct SceneGadget {
	int config_id;
	int gadget_id;
	Vec3f pos;
	Vec3f rot;
	int level;
	int route_id;
	bool save_route;
};

struct SceneGroup {
	int id;
	int refresh_time;
	int area;
	Vec3f pos;
	bool refresh_with_block;
	std::vector<SceneGadget> gadgets;
};

struct SceneBlock {
	int id;
	Vec2f min;
	Vec2f max;
	std::unordered_map<int, SceneGroup> groups;
	// pools would be here
};

struct SceneConfig {
	Vec2f begin_pos;
	Vec2f size;
	Vec3f born_pos;
	Vec3f born_rot;
	float die_y;
	std::unordered_map<int, SceneBlock> blocks;
};

extern std::unordered_map<int, SceneConfig> scene_configs;

};
