#include "stdafx.h"
#include "ChipDataWindow.h"
#include "ui_ChipDataWindow.h"
#include "GetChipWindow.h"
#include "CodeX/CodeX.h"
#include "ChipSolver/ChipSolver.h"

ChipDataWindow::ChipDataWindow(QWidget* parent, Qt::WindowFlags f):
	QDialog(parent,f),
	ui(new Ui::ChipDataWindow),
	getChipWindow(new GetChipWindow(this)),
	tableModel_(new ChipTableModel(this)),
	squadModel_(new ChipTableModel(this)),
	tableDelegate_(new ChipTableDelegate(this)),
	squadDelegate_(new ChipTableDelegate(this))
{
	ui->setupUi(this);
	connect();
}

ChipDataWindow::~ChipDataWindow()
{
}

void ChipDataWindow::init()
{
	getChipWindow->init();
	this->ui->tableView->setModel(this->tableModel_);
	this->ui->tableView->setItemDelegate(this->tableDelegate_);
	this->ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	this->ui->tableView->verticalHeader()->hide();
	this->ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	
	this->ui->squadTableView->setModel(this->squadModel_);
	this->ui->squadTableView->setItemDelegate(this->squadDelegate_);
	this->ui->squadTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	this->ui->squadTableView->verticalHeader()->hide();
	this->ui->squadTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	this->ui->squadTableView->setColumnHidden(0, true);
	this->ui->squadTableView->setColumnHidden(2, true);
	this->ui->squadTableView->setColumnHidden(8, true);
	this->squadModel_->setShowStatus(false);
}

void ChipDataWindow::recvChipJsonObject(const QJsonObject& object)
{
	auto squads = object["squad_with_user_info"].toObject();
	static bool first = true;
	std::map<int, int> squadID;
	for(const auto& it : squads)
	{
		auto obj = it.toObject();
		squadID[obj["id"].toString().toInt()] = obj["squad_id"].toString().toInt();
	}

	auto& chips = CodeX::instance()->chips;
	auto& gridChips = CodeX::instance()->gridChips;
	auto& squadChips = CodeX::instance()->squadChips;
	squadChips.clear();
	gridChips.clear();
	chips = getChips(object["chip_with_user_info"].toObject());
	for(auto i = 0;i < chips.size();++i)
	{
		auto& chip = chips[i];
		chip.no = i + 1;
		if(chip.squad > 0)
		{
			chip.squad = squadID[chip.squad];
			squadChips[chip.squad].push_back(chip);
		}
		// 拷贝一份+20
		auto t = chip;
		t.no = i;
		t.level = 20;
		t.calcValue();
		gridChips[t.color][t.gridID].push_back(t);
	}
	this->tableModel_->setChips(chips);
	this->ui->squadComboBox->setCurrentIndex(1);
	this->ui->squadComboBox->setCurrentIndex(0);
	if (!first)// 第一次是从配置读取数据，不算修改芯片
		emit chipsChanged();
	first = false;
}

void ChipDataWindow::squadChanged(int index)
{
	const auto& chips = CodeX::instance()->squadChips[index + 1];
	this->squadModel_->setChips(chips);
	GFChip sum, max;
	for(auto chip : chips)
	{
		sum += chip;
		chip.level = 20;
		chip.calcValue();
		max += chip;
	}
	auto M = CodeX::instance()->solver_->squadMaxValue(this->ui->squadComboBox->currentText());
	this->ui->damageLabel->setText(trUtf8(u8"杀伤：") + QString("%1/%2/%3/%4").arg(sum.damageValue).arg(max.damageValue).arg(std::min(0, max.damageValue - M.damageValue)).arg(sum.damageBlock));
	this->ui->dbkLabel->setText(trUtf8(u8"破防：") + QString("%1/%2/%3/%4").arg(sum.defbreakValue).arg(max.defbreakValue).arg(std::min(0, max.defbreakValue - M.defbreakValue)).arg(sum.defbreakBlock));
	this->ui->hitLabel->setText(trUtf8(u8"精度：") + QString("%1/%2/%3/%4").arg(sum.hitValue).arg(max.hitValue).arg(std::min(0, max.hitValue - M.hitValue)).arg(sum.hitBlock));
	this->ui->reloadLabel->setText(trUtf8(u8"装填：") + QString("%1/%2/%3/%4").arg(sum.reloadValue).arg(max.reloadValue).arg(std::min(0, max.reloadValue - M.reloadValue)).arg(sum.reloadBlock));
}

void ChipDataWindow::importChipJson()
{
	auto filename = QFileDialog::getOpenFileName(this, u8"打开芯片数据JSON", "", "JSON (*.json)");
	if(filename.isEmpty())
		return;
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::warning(this, u8"错误", u8"打开文件失败");
		return;
	}
	auto data = file.readAll();
	QJsonParseError jsonError;
	QJsonDocument doucment = QJsonDocument::fromJson(data, &jsonError);  // 转化为 JSON 文档
	if (!doucment.isNull() && jsonError.error == QJsonParseError::NoError && doucment.isObject())
	{  // 解析未发生错误 JSON 文档为对象
		auto obj = doucment.object();
		if(obj.contains("squad_with_user_info") && obj.contains("chip_with_user_info"))
		{
			try
			{
				recvChipJsonObject(obj);
				QSettings settings;
				settings.setValue("/User/Chip", data);
			}
			catch (const std::exception& e)
			{
				QMessageBox::warning(this, u8"错误", QString(u8"解析JSON失败！\n") + e.what());
				return;
			}
		}
		QMessageBox::information(this, u8"成功", u8"导入芯片数据成功！");
	}
	else
	{
		QMessageBox::warning(this, u8"错误", QString(u8"解析JSON失败！\n"));
	}
}

void ChipDataWindow::connect()
{
	QObject::connect(
		this->ui->getDataPushButton,
		&QPushButton::clicked,
		this->getChipWindow,
		&GetChipWindow::show
	);
	QObject::connect(
		this->ui->importDataPushButton,
		&QPushButton::clicked,
		this,
		&ChipDataWindow::importChipJson
	);
	QObject::connect(
		this->getChipWindow,
		&GetChipWindow::sendChipJsonObject,
		this,
		&ChipDataWindow::recvChipJsonObject
	);
	QObject::connect(
		this->ui->squadComboBox,
		static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		this,
		&ChipDataWindow::squadChanged
	);
}
