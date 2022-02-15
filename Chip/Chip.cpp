#include "Chip.h"
#include <QString>
#include <QStringList>
#include <cmath>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>
#include <QDebug>
#include <QResource>
#include <QFile>

// 各属性系数
static constexpr float ArgDmg = 4.4, ArgDbk = 12.7, ArgAcu = 7.1, ArgFil = 5.7;
// 密度系数
static constexpr float Den56 = 1.0, Den551 = 0.92, Den552 = 1.0;
// 等级系数
static constexpr float ArgLv[21] = { 1.0,1.08,1.16,1.24,1.32,1.4,1.48,1.56,1.64,1.72,1.8,1.87,1.94,2.01,2.08,2.15,2.22,2.29,2.36,2.43,2.5 };

static bool ChipResourceInit = false;

GFChip::GFChip(const QJsonObject& object)
{
	id = object.value("id").toString("0").toInt();
	exp = object.value("chip_exp").toString("0").toInt();
	level = object.value("chip_level").toString().toInt();
	color = object.value("color_id").toString().toInt();
	gridID = object.value("grid_id").toString().toInt();
	chipClass = object.value("chip_id").toString().toInt();  //ChipConfig::getConfig(gridID).chipClass; // 游戏内有的class不对。。。
	squad = object.value("squad_with_user_id").toString().toInt();
	auto t = object.value("position").toString("0,0").split(",");
	position.x = t[0].toInt();
	position.y = t[1].toInt();
	rotate = object.value("shape_info").toString("0,0")
		.split(",")[0].toInt();
	damageBlock = object.value("assist_damage").toString("0").toInt();
	reloadBlock = object.value("assist_reload").toString("0").toInt();
	hitBlock = object.value("assist_hit").toString("0").toInt();
	defbreakBlock = object.value("assist_def_break").toString("0").toInt();
	locked = object.value("is_locked").toString("0").toInt();
	no = 0;
	if(object.contains("damage_value"))
	{
		damageValue = object["damage_value"].toInt();
		reloadValue = object["reload_value"].toInt();
		hitValue = object["hit_value"].toInt();
		defbreakValue = object["def_break_value"].toInt();
	}
	else
	{
		calcValue();
	}
}

GFChip GFChip::fromJsonObject(const QJsonObject& object)
{
	return GFChip(object);
}

QJsonObject GFChip::toObject() const
{
	QJsonObject obj;
	obj["id"] = QString::number(id);
	obj["chip_exp"] = QString::number(exp);
	obj["chip_level"] = QString::number(level);
	obj["color_id"] = QString::number(color);
	obj["grid_id"] = QString::number(gridID);
	obj["chip_id"] = QString::number(chipClass);
	obj["squad_with_user_id"] = QString::number(squad);
	obj["position"] = QString("%1,%2").arg(position.x).arg(position.y);
	obj["shape_info"] = QString("%1,0").arg(rotate);
	obj["assist_damage"] = QString::number(damageBlock);
	obj["assist_reload"] = QString::number(reloadBlock);
	obj["assist_hit"] = QString::number(hitBlock);
	obj["assist_def_break"] = QString::number(defbreakBlock);
	obj["damage_value"] = damageValue;
	obj["reload_value"] = reloadValue;
	obj["hit_value"] = hitValue;
	obj["def_break_value"] = defbreakValue;
	obj["is_locked"] = locked;
	return obj;
}

QPixmap GFChip::icon() const
{
	if(!ChipResourceInit)
	{
		ChipResourceInit = true;
		Q_INIT_RESOURCE(Chip);
	}
	static const QString url(":/Chip/Resources/img/%1%2.png");
	return QPixmap(url.arg(color == Blue ? "b" : "o").arg(gridID));
}

QString GFChip::name() const
{
	return QString::fromStdString(ChipConfig::getConfig(this->gridID).name);
}

QString GFChip::squadName() const
{
	return squadString(squad);
}

GFChip::GFChip(): id(0), no(0), chipClass(0), exp(0), level(0), color(Orange), gridID(0), squad(0), position({0, 0}),
                  rotate(0),damageBlock(0),
                  reloadBlock(0), hitBlock(0), defbreakBlock(0), damageValue(0), reloadValue(0), hitValue(0),
                  defbreakValue(0), locked(false)
{
}

void GFChip::calcValue()
{
	auto den = Den56;
	switch (this->chipClass)
	{
	default:
	case Class56:
		den = Den56;
		break;
	case Class551:
		den = Den551;
		break;
	case Class552:
		den = Den552;
		break;
	}
	this->damageValue = ceilf(ceilf(this->damageBlock * ArgDmg * den) * ArgLv[this->level]);
	this->defbreakValue = ceilf(ceilf(this->defbreakBlock * ArgDbk * den) * ArgLv[this->level]);
	this->hitValue = ceilf(ceilf(this->hitBlock * ArgAcu * den) * ArgLv[this->level]);
	this->reloadValue = ceilf(ceilf(this->reloadBlock * ArgFil * den) * ArgLv[this->level]);
}

GFChip GFChip::operator+(const GFChip& t) const
{
	auto r = *this;
	r.defbreakBlock += t.defbreakBlock;
	r.reloadBlock += t.reloadBlock;
	r.damageBlock += t.damageBlock;
	r.hitBlock += t.hitBlock;
	r.defbreakValue += t.defbreakValue;
	r.reloadValue += t.reloadValue;
	r.damageValue += t.damageValue;
	r.hitValue += t.hitValue;
	return r;
}

GFChip GFChip::operator+=(const GFChip& t)
{
	return *this = *this + t;
}

GFChip GFChip::operator-(const GFChip& t) const
{
	auto r = *this;
	r.defbreakBlock -= t.defbreakBlock;
	r.reloadBlock -= t.reloadBlock;
	r.damageBlock -= t.damageBlock;
	r.hitBlock -= t.hitBlock;
	r.defbreakValue -= t.defbreakValue;
	r.reloadValue -= t.reloadValue;
	r.damageValue -= t.damageValue;
	r.hitValue -= t.hitValue;
	return r;
}

GFChip GFChip::operator-=(const GFChip& t)
{
	return *this = *this - t;
}

Chips getChips(const QJsonObject& obj)
{
	Chips chips;
	for(const auto& k : obj.keys())
	{
		auto chip = GFChip::fromJsonObject(obj[k].toObject());
		if ((chip.chipClass == GFChip::Class56 || chip.chipClass == GFChip::Class551 || chip.chipClass == GFChip::Class552) && chip.
			gridID > 11)
		{
			chip.rotate %= ChipConfig::getConfig(chip.gridID).direction;
			chips.push_back(chip);
		}
	}
	// 按ID排序
	std::sort(chips.begin(), chips.end(),
		[](const GFChip& a, const GFChip& b) -> bool
		{
			return a.id < b.id;
		});
	for(auto i = 0;i < chips.size();++i)
	{
		chips[i].no = i + 1;
	}
	return chips;
}

QString squadString(int i)
{
	switch (i)
	{
	case 1:
		return QString("BGM");
	case 2:
		return QString("AGS");
	case 3:
		return QString("2B");
	case 4:
		return QString("M2");
	case 5:
		return QString("AT4");
	case 6:
		return QString("QLZ");
	case 7:
		return QString("Mk153");
	default:
		return QString("");
	}
}

std::map<int, ChipConfig> ChipConfig::configs_ = std::map<int, ChipConfig>();

const ChipConfig& ChipConfig::getConfig(int id)
{
	if(configs_.empty())
	{
		if (!ChipResourceInit)
		{
			ChipResourceInit = true;
			Q_INIT_RESOURCE(Chip);
		}
		// 初始化
		QFile json(":/Chip/Resources/chips.json");
		json.open(QIODevice::ReadOnly);
		auto data = json.readAll();
		auto doc = QJsonDocument::fromJson(data);
		for(const auto& it : doc.array())
		{
			auto config = ChipConfig(it.toObject());
			configs_[config.gridID] = config;
		}
	}
	return configs_[id];
}

ChipConfig ChipConfig::rotate90(int n) const
{
	auto rotate = [](const Map& map) -> std::vector<std::string>
	{
		Map r;
		std::string t;
		t.resize(map.size(), '0');
		r.resize(map[0].length(), t);
		for (auto i = 0u; i < r.size(); ++i)
		{
			for (auto j = 0u; j < r[i].length(); ++j)
			{
				r[i][j] = map[map.size() - j - 1][i];
			}
		}
		return r;
	};
	auto t = *this;
	for (auto i = 0; i < n; ++i)
	{
		t.map = rotate(t.map);
		std::swap(t.height, t.width);
	}
	return t;
}

ChipConfig::ChipConfig(const QJsonObject& object)
{
	gridID = object.value("ID").toInt();
	chipClass = object.value("class").toInt();
	width = object.value("width").toInt();
	height = object.value("height").toInt();
	blocks = object.value("blocks").toInt();
	direction = object.value("direction").toInt();
	name = object.value("name").toString().toStdString();
	auto oMap = object["map"].toArray();
	for(const auto& it : oMap)
	{
		map.push_back(it.toString().toStdString());
	}
}

ChipPuzzleOption::ChipPuzzleOption(const QJsonObject& object)
{
	no = object.value("ID").toInt();
	x = object.value("x").toInt();
	y = object.value("y").toInt();
	rotate = object.value("rotate").toInt();
}

QJsonObject ChipPuzzleOption::toObject() const
{
	QJsonObject obj;
	obj["ID"] = no;
	obj["x"] = x;
	obj["y"] = y;
	obj["rotate"] = rotate;
	return obj;
}

ChipPuzzleOption ChipPuzzleOption::fromJsonObject(const QJsonObject& object)
{
	return ChipPuzzleOption(object);
}

bool Solution::operator<(const Solution& r) const
{
	return this->totalValue.id > r.totalValue.id;
}

bool Solution::operator>(const Solution& r) const
{
	return this->totalValue.id < r.totalValue.id;
}

QJsonObject Solution::toObject() const
{
	QJsonObject obj;
	QJsonArray arr;
	for(const auto& it:chips)
	{
		arr.append(it.toObject());
	}
	obj["totalValue"] = totalValue.toObject();
	obj["squad"] = squad;
	obj["chips"] = arr;
	return obj;
}

Solution Solution::fromJsonObject(const QJsonObject& obj)
{
	Solution s;
	s.totalValue = GFChip::fromJsonObject(obj["totalValue"].toObject());
	auto arr = obj["chips"].toArray();
	for(const auto& it : arr)
	{
		s.chips.push_back(ChipPuzzleOption::fromJsonObject(it.toObject()));
	}
	s.squad = obj["squad"].toString();
	return s;
}
