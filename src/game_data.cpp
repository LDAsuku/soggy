// game_data.cpp

// loads stuff from exceloutput/ and binoutput/

#include "game_data.hpp"

// C++
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <regex>

// nlohmann json
#include <nlohmann/json.hpp>

// lua
extern "C" {
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

// soggy
#include "game_enums.hpp"
#include "vec.hpp"
#include "log.hpp"

//
// structs
//

namespace exceloutput {
	std::unordered_map<int, AvatarData> avatar_datas;
	std::unordered_map<int, AvatarSkillDepotData> avatar_skill_depot_datas;
	std::unordered_map<int, AvatarSkillData> avatar_skill_datas;
	std::unordered_map<int, ItemData> item_datas;
	std::unordered_map<int, ShopPlanData> shop_plan_datas;
	std::unordered_map<int, ShopGoodsData> shop_goods_datas;
	std::unordered_map<int, TalentSkillData> talent_skill_datas;
	std::unordered_map<int, GadgetData> gadget_datas;
	std::unordered_map<int, MonsterData> monster_datas;
	std::unordered_map<int, NpcData> npc_datas;

	std::unordered_map<std::string, std::string> text_map;
}

void from_json(const nlohmann::json &json, Vec3f &);

namespace binoutput {
	std::unordered_map<int, ConfigScene> scene_points;

	void from_json(const nlohmann::json &json, ConfigScene &);
	void from_json(const nlohmann::json &json, ConfigScenePoint &);
	void from_json(const nlohmann::json &json, ConfigSceneArea &);
}

namespace luares {
	std::unordered_map<int, SceneConfig> scene_configs;
};

//
// excel helper
//

struct Excel {
private:
	char sep;
	std::istream *is;
	std::vector<std::string> headers;
	std::unordered_map<std::string, std::string> fields;
public:
	Excel(std::istream *is, char sep = '\t');
	void next();
	bool eof();
	std::string get_str(const std::string key);
	int get_int(const std::string key, int def = 0);
	std::vector<int> get_ints(const std::string key);
};

Excel::Excel(std::istream *is, char sep) {
	this->sep = sep;
	this->is = is;
	std::string headers_str;
	std::getline(*this->is, headers_str);

	std::istringstream headers_iss(headers_str);
	std::string header;
	while (std::getline(headers_iss, header, this->sep)) {
		this->headers.emplace_back(header);
	}
}
void Excel::next() {
	std::string line_str;
	std::getline(*this->is, line_str);

	std::istringstream line_iss(line_str);
	std::string token;
	int i = 0;
	while (std::getline(line_iss, token, this->sep)) {
		this->fields.insert_or_assign(this->headers.at(i++), token);
	}
}
bool Excel::eof() {
	return this->is->eof();
}

std::string Excel::get_str(const std::string key) {
	return this->fields.at(key);
}
int Excel::get_int(const std::string key, int def) {
	auto it = this->fields.find(key);
	if (it == this->fields.cend()) {
		return def;
	}
	std::string *val = &it->second;
	if (val->empty()) {
		return def;
	}
	return std::stoi(it->second);
}
std::vector<int> Excel::get_ints(const std::string key) {
	std::vector<int> ints;
	std::istringstream iss(this->fields.at(key));
	std::string token;
	while (std::getline(iss, token, ',')) {
		ints.emplace_back(4);
	}
	return ints;
}

//
// small wrapper for lua stack access
//

/*struct LuaIndex {
public:
	LuaIndex(lua_State *L);
	bool push_global_table(const char *global);
	bool push_field_table(const char *key);
	void push_nil();
	bool next();
	lua_Integer get_int(int idx);
	lua_Number get_field_number(const char *key, lua_Number def = 0.0f);
	lua_Integer get_field_int(const char *key, lua_Integer def = 0);
	bool get_field_bool(const char *key, bool def = false);
	void pop(int n = 1);
private:
	lua_State *L;
};

LuaIndex::LuaIndex(lua_State *L) : L(L) {}
bool LuaIndex::push_global_table(const char *global) {
	return lua_getglobal(this->L, global) == LUA_TTABLE;
}
bool LuaIndex::push_field_table(const char *key) {
	lua_pushstring(this->L, key);
	return lua_gettable(this->L, -2) == LUA_TTABLE;
}
void LuaIndex::push_nil() {
	lua_pushnil(this->L);
}
bool LuaIndex::next() {
	return lua_next(this->L, -2);
}
lua_Integer LuaIndex::get_int(int idx) {
	return lua_tointeger(this->L, idx);
}
lua_Number LuaIndex::get_field_number(const char *key, lua_Number def) {
	lua_Number r;
	if (lua_getfield(this->L, -1, key) == LUA_TNUMBER) {
		r = lua_tonumber(this->L, -1);
	} else {
		r = def;
	}
	lua_pop(this->L, 1);
	return r;
}
lua_Integer LuaIndex::get_field_int(const char *key, lua_Integer def) {
	lua_Integer r;
	if (lua_getfield(this->L, -1, key) == LUA_TNUMBER) {
		r = lua_tointeger(this->L, -1);
	} else {
		r = def;
	}
	lua_pop(this->L, 1);
	return r;
}
bool LuaIndex::get_field_bool(const char *key, bool def) {
	lua_Integer r;
	if (lua_getfield(this->L, -1, key) == LUA_TNUMBER) {
		r = lua_toboolean(this->L, -1);
	} else {
		r = def;
	}
	lua_pop(this->L, 1);
	return r;
}
void LuaIndex::pop(int n) {
	lua_pop(this->L, n);
}*/

//
// binoutput json stuff
//

void from_json(const nlohmann::json &json, Vec3f &vec) {
	vec.x = json.value("x", 0.0f);
	vec.y = json.value("y", 0.0f);
	vec.z = json.value("z", 0.0f);
}
void binoutput::from_json(const nlohmann::json &json, binoutput::ConfigScenePoint &configscenepoint) {
	json.at("$type").get_to(configscenepoint.type);
	nlohmann::json::const_iterator json_tranpos = json.find("tranPos");
	if (json_tranpos != json.end() && json_tranpos->is_object()) {
		json_tranpos->get_to(configscenepoint.tranpos);
	}
	nlohmann::json::const_iterator json_tranrot = json.find("tranRot");
	if (json_tranrot != json.end() && json_tranrot->is_object()) {
		json_tranrot->get_to(configscenepoint.tranrot);
	}
}
void binoutput::from_json(const nlohmann::json &json, binoutput::ConfigSceneArea &configscenearea) {
	//
}
void binoutput::from_json(const nlohmann::json &json, binoutput::ConfigScene &configscene) {
	nlohmann::json::const_iterator json_points = json.find("points");
	if (json_points != json.end() && json_points->is_object()) {
		for (nlohmann::json::const_iterator it = json_points->cbegin(); it != json_points->cend(); it++) {
			configscene.points.emplace(std::stoi(it.key()), it.value());
		}
	}
	nlohmann::json::const_iterator json_areas = json.find("areas"); // "pboints"
	if (json_areas != json.end() && json_areas->empty()) {
		for (nlohmann::json::const_iterator it = json_areas->cbegin(); it != json_areas->cend(); it++) {
			configscene.areas.emplace(std::stoi(it.key()), it.value());
		}
	}
}

//
// loading data
//

static bool error = false;

template <typename T>
void load_excel(const char *filename, std::unordered_map<int, T> *map, const char *key_column, void (*proc)(T *data, Excel *excel)) {
	if (!std::filesystem::exists(filename)) {
		soggy_log("error!!!: %s does not exist", filename);
		error = true;
		return;
	}
	std::ifstream ifs(filename);
	Excel excel(&ifs);
	while (!excel.eof()) {
		excel.next();
		int key = excel.get_int(key_column);
		T *data = &(*map)[key];
		data->id = key;
		proc(data, &excel);
	}
}

bool load_game_data() {
	// AvatarData
	load_excel<exceloutput::AvatarData>("resources/exceloutput/AvatarData.tsv", &exceloutput::avatar_datas, "ID", [](exceloutput::AvatarData *avatar, Excel *excel) {
		avatar->skill_depot_id = excel->get_int("技能库ID");
		avatar->initial_weapon_id = excel->get_int("初始武器");
		avatar->weapon_type = (WeaponType)excel->get_int("武器种类");
		avatar->use_type = (AvatarUseType)excel->get_int("是否使用");
	});

	// AvatarSkillDepotData
	load_excel<exceloutput::AvatarSkillDepotData>("resources/exceloutput/AvatarSkillDepotData.tsv", &exceloutput::avatar_skill_depot_datas, "ID", [](exceloutput::AvatarSkillDepotData *avatar_skill_depot, Excel *excel) {
		avatar_skill_depot->leader_talent = excel->get_int("队长天赋");
		for (int i = 0; i < 10; i++) {
			int group = excel->get_int("天赋组" + std::to_string(i + 1));
			if (group != 0) {
				avatar_skill_depot->talent_groups.push_back(group);
			}
		}
	});

	// AvatarSkillData
	load_excel<exceloutput::AvatarSkillData>("resources/exceloutput/AvatarSkillData.tsv", &exceloutput::avatar_skill_datas, "ID", [](exceloutput::AvatarSkillData *avatar_skill, Excel *excel) {
		avatar_skill->energy_type = (ElementType)excel->get_int("消耗能量类型");
		avatar_skill->energy_cost = excel->get_int("消耗能量值");
		avatar_skill->max_charges = excel->get_int("可累积次数", 1);
	});

	// MaterialData
	load_excel<exceloutput::ItemData>("resources/exceloutput/MaterialData.tsv", &exceloutput::item_datas, "ID", [](exceloutput::ItemData *item, Excel *excel) {
		item->type = (ItemType)excel->get_int("类型");
		item->equip_type = EquipType::EQUIP_NONE;
	});

	// WeaponData
	load_excel<exceloutput::ItemData>("resources/exceloutput/WeaponData.tsv", &exceloutput::item_datas, "ID", [](exceloutput::ItemData *item, Excel *excel) {
		item->type = (ItemType)excel->get_int("类型");
		item->weapon.gadget_id = excel->get_int("物件ID");
		item->weapon.weapon_type = (WeaponType)excel->get_int("武器阶数");
		item->equip_type = EquipType::EQUIP_WEAPON;
	});

	// ReliquaryData
	load_excel<exceloutput::ItemData>("resources/exceloutput/ReliquaryData.tsv", &exceloutput::item_datas, "ID", [](exceloutput::ItemData *item, Excel *excel) {
		item->type = (ItemType)excel->get_int("类型");
		item->equip_type = (EquipType)excel->get_int("圣遗物类别");
	});

	// ShopPlanData
	load_excel<exceloutput::ShopPlanData>("resources/exceloutput/ShopPlanData.tsv", &exceloutput::shop_plan_datas, "ID", [](exceloutput::ShopPlanData *shop_plan, Excel *excel) {
		shop_plan->shop_type = excel->get_int("商店类型");
		shop_plan->goods_id = excel->get_int("商品ID");
	});

	// ShopGoodsData
	load_excel<exceloutput::ShopGoodsData>("resources/exceloutput/ShopGoodsData.tsv", &exceloutput::shop_goods_datas, "商品ID", [](exceloutput::ShopGoodsData *shop_goods, Excel *excel) {
		shop_goods->result_item_id = excel->get_int("对应物品ID");
		shop_goods->result_item_count = excel->get_int("对应物品数量");
		shop_goods->mora_cost = excel->get_int("消耗金币");
		shop_goods->primogem_cost = excel->get_int("消耗水晶");
		shop_goods->consume_1_item_id = excel->get_int("[消耗物品]1ID");
		shop_goods->consume_1_item_count = excel->get_int("[消耗物品]1数量");
		shop_goods->consume_2_item_id = excel->get_int("[消耗物品]2ID");
		shop_goods->consume_2_item_count = excel->get_int("[消耗物品]2数量");
		shop_goods->max_purchases = excel->get_int("限购数量");
	});

	// TalentSkillData
	load_excel<exceloutput::TalentSkillData>("resources/exceloutput/TalentSkillData.tsv", &exceloutput::talent_skill_datas, "天赋ID", [](exceloutput::TalentSkillData *talent_skill, Excel *excel) {
		talent_skill->talent_group_id = excel->get_int("天赋组ID");
		talent_skill->rank = excel->get_int("等级");
	});

	// GadgetData
	load_excel<exceloutput::GadgetData>("resources/exceloutput/GadgetData.tsv", &exceloutput::gadget_datas, "ID", [](exceloutput::GadgetData *gadget, Excel *excel) {
		gadget->default_camp = excel->get_int("默认阵营");
		gadget->is_interactive = (bool)excel->get_int("能否交互");
	});

	// MonsterData
	load_excel<exceloutput::MonsterData>("resources/exceloutput/MonsterData.tsv", &exceloutput::monster_datas, "ID", [](exceloutput::MonsterData *monster, Excel *excel) {
	});

	// NpcData
	load_excel<exceloutput::NpcData>("resources/exceloutput/NpcData.tsv", &exceloutput::npc_datas, "ID", [](exceloutput::NpcData *npc, Excel *excel) {
	});

	// scene configs
	if (!std::filesystem::exists("resources/binoutput/Scene/Point")) {
		soggy_log("error!!!: resources/binoutput/Scene/Point does not exist");
		error = true;
	} else {
		std::regex re_scene_points(R"(^scene(\d+)_point\.json$)");
		for (const std::filesystem::directory_entry &dirent : std::filesystem::directory_iterator("resources/binoutput/Scene/Point")) {
			std::filesystem::path path = dirent.path();
			std::smatch m;
			std::string filename = path.filename().string();
			if (std::regex_match(filename, m, re_scene_points)) {
				std::string s = m[1].str();
				int sceneid = std::stoi(s);

				std::ifstream ifs(path);
				nlohmann::json json;
				ifs >> json;

				binoutput::scene_points.emplace(sceneid, json); // calls ConfigScene ctor and from_json
			}
		}
	}

	return error;
}
