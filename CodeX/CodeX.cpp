#include "stdafx.h"
#include "CodeX.h"
#include "ui_CodeX.h"
#include "SettingWindow.h"
#include "AboutDialog.h"
#include "AltSolutionWindow.h"

#define CURRENT_SOLUTION (this->solutions_[this->ui->squadsComboBox->currentText()][this->ui->configsComboBox->currentText()])

CodeX* CodeX::instance()
{
	if(!CodeX::singleton)
	{
		new CodeX();
	}
	return singleton;
}

bool CodeX::chipUsed(int no)
{
	return this->altSolutionWindow_->chipUsed(no);
}

void CodeX::solve()
{
	this->ui->solutionTable->setEnabled(false);
	this->ui->chipsTable->setEnabled(false);
	this->ui->squadsComboBox->setEnabled(false);
	this->ui->configsComboBox->setEnabled(false);
	this->ui->useAltCheckBox->setEnabled(false);
	this->ui->useEquippedCheckBox->setEnabled(false);
	this->ui->solveButton->setEnabled(false);
	this->ui->useEquippedCheckBox->setEnabled(false);
	this->ui->useLockedCheckBox->setEnabled(false);
	
	this->solver_->setTargetBlock(this->settingWindow_->getTargetBlock(this->ui->squadsComboBox->currentText()));
	this->timer_->start(100);
	this->startTime_ = QDateTime::currentDateTime();
	this->solver_->start();
}

void CodeX::solveFinished()
{
	this->ui->solutionTable->setEnabled(true);
	this->ui->chipsTable->setEnabled(true);
	this->ui->squadsComboBox->setEnabled(true);
	this->ui->configsComboBox->setEnabled(true);
	this->ui->useEquippedCheckBox->setEnabled(true);
	this->ui->solveButton->setEnabled(true);
	this->ui->useEquippedCheckBox->setEnabled(true);
	this->ui->useLockedCheckBox->setEnabled(true);
	this->ui->useAltCheckBox->setEnabled(true);
	CURRENT_SOLUTION = this->solver_->solutions;
	this->solutionTableModel_->setSolution(&CURRENT_SOLUTION);
	this->solutionTableModel_->setMaxValue(this->solver_->squadMaxValue(this->ui->squadsComboBox->currentText()));
	this->solutionTableModel_->sort(5, Qt::DescendingOrder);
	this->timer_->stop();
	if(this->solver_->solutions.empty())
	{
		QMessageBox::information(this, u8"提示", u8"没有算出可行解哦！\n攒更多芯片后再来试试吧~\n也可以修改格数方案以及自由格数尝试哦~");
	}
}

void CodeX::selectSolution(int index)
{
	Chips solutionChips;
	const auto& solution = CURRENT_SOLUTION[index];
	for(const auto& it : solution.chips)
	{
		solutionChips.push_back(this->chips[it.no]);
	}
	this->chipTableModel_->setChips(solutionChips);
	this->ui->chipView->setView(this->solver_->solution2ChipView(solution));
}

void CodeX::addAltSolution()
{
	auto index = this->ui->solutionTable->currentIndex().row();
	if (index < 0)
		return;
	auto s = CURRENT_SOLUTION[index];
	s.squad = this->ui->squadsComboBox->currentText();
	this->altSolutionWindow_->addSolution(s);
}

void CodeX::chipsChanged()
{
	this->solutions_.clear();
	this->altSolutionWindow_->clearSolution();
}

void CodeX::solutionNumberChanged(long long n)
{
	this->solveNumberLabel_->setText(trUtf8(u8"方案数：") + QString::number(n));
}

CodeX::CodeX(QWidget *parent)
	: QMainWindow(parent),
	solver_(new ChipSolver(this)),
	ui(new Ui::CodeX),
	chipDataWindow_(new ChipDataWindow(this)),
	settingWindow_(new SettingWindow(this)),
	progressBar_(new QProgressBar(this)),
	solveNumberLabel_(new QLabel(u8"方案数：0",this)),
	timeLabel_(new QLabel(u8"耗时：0s", this)),
	chipTableModel_(new ChipTableModel(this)),
	chipTableDelegate_(new ChipTableDelegate(this)),
	solutionTableModel_(new SolutionTableModel(this)),
	aboutDialog_(new AboutDialog(this)),
	altSolutionWindow_(new AltSolutionWindow(this)),
	timer_(new QTimer(this))
{
	CodeX::singleton = this;
	ui->setupUi(this);
	this->ui->statusBar->addWidget(progressBar_);
	this->ui->statusBar->addWidget(solveNumberLabel_);
	this->ui->statusBar->addWidget(timeLabel_);
	
	this->ui->chipView->setChipSize(QSize(8,8));

	this->ui->chipsTable->setModel(chipTableModel_);
	this->ui->chipsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	this->ui->chipsTable->setItemDelegate(chipTableDelegate_);
	this->ui->chipsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	this->ui->chipsTable->verticalHeader()->hide();
	//this->ui->chipsTable->setColumnHidden(0,true);
	this->ui->chipsTable->setColumnHidden(2,true);
	this->chipTableModel_->setShowBlocks(false);
	this->chipTableModel_->setShowStatus(false);
	this->chipTableModel_->setShowColor(true);
	this->ui->solutionTable->setModel(solutionTableModel_);
	this->ui->solutionTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	this->ui->solutionTable->verticalHeader()->hide();
	this->ui->solutionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	this->ui->solutionTable->setSelectionMode(QAbstractItemView::SingleSelection);
	this->ui->solutionTable->setSortingEnabled(true);
	this->ui->solutionTable->setColumnHidden(8, true);

	connect();

	this->chipDataWindow_->init();
	this->ui->squadsComboBox->addItems(this->solver_->squadList());
	this->solver_->setTargetBlock(TargetBlock(20,2,4,6,0));
	this->altSolutionWindow_->init();
	this->show();
	this->aboutDialog_->checkUpdate();
}

void CodeX::connect()
{
	QObject::connect(
		this->ui->chipDataButton,
		&QPushButton::clicked,
		this->chipDataWindow_,
		&ChipDataWindow::show
	);
	QObject::connect(
		this->solver_,
		&ChipSolver::solvePercentChanged,
		this->progressBar_,
		&QProgressBar::setValue
	);
	QObject::connect(
		this->solver_,
		&ChipSolver::solveNumberChanged,
		this,
		&CodeX::solutionNumberChanged,
		Qt::QueuedConnection
	);
	QObject::connect(
		this->solver_,
		&ChipSolver::finished,
		this,
		&CodeX::solveFinished
	);
	QObject::connect(
		this->ui->solveButton,
		&QPushButton::clicked,
		this,
		&CodeX::solve
	);
	QObject::connect(
		this->ui->squadsComboBox,
		&QComboBox::currentTextChanged,
		[&](const QString& squad)
		{
			this->ui->configsComboBox->clear();
			this->ui->configsComboBox->addItems(this->solver_->configList(squad));
			this->solutionTableModel_->setSolution(&CURRENT_SOLUTION);
		});
	QObject::connect(
		this->ui->configsComboBox,
		&QComboBox::currentTextChanged,
		this->solver_,
		&ChipSolver::setTargetConfig
	);
	QObject::connect(
		this->ui->configsComboBox,
		&QComboBox::currentTextChanged,
		[&](const QString config)
		{
			this->solutionTableModel_->setSolution(&CURRENT_SOLUTION);
		});
	QObject::connect(
		this->ui->useLockedCheckBox,
		&QCheckBox::stateChanged,
		this->solver_,
		&ChipSolver::setUseLocked
	);
	QObject::connect(
		this->ui->useEquippedCheckBox,
		&QCheckBox::stateChanged,
		this->solver_,
		&ChipSolver::setUseEquipped
	);
	QObject::connect(
		this->ui->useAltCheckBox,
		&QCheckBox::stateChanged,
		this->solver_,
		&ChipSolver::setUseAlt
	);
	QObject::connect(
		this->ui->solutionTable,
		&QTableView::clicked,
		[&](QModelIndex index)
		{
			this->selectSolution(index.row());
		});
	QObject::connect(
		this->ui->settingPushButton,
		&QPushButton::clicked,
		this->settingWindow_,
		&SettingWindow::show
	);
	QObject::connect(
		this->ui->aboutPushButton,
		&QPushButton::clicked,
		this->aboutDialog_,
		&AboutDialog::show
	);
	QObject::connect(
		this->ui->altButton,
		&QPushButton::clicked,
		this->altSolutionWindow_,
		&AltSolutionWindow::show
	);
	QObject::connect(
		this->ui->stopPushButton,
		&QPushButton::clicked,
		this->solver_,
		&ChipSolver::stop,
		Qt::DirectConnection
	);
	QObject::connect(
		this->ui->addAltButton,
		&QPushButton::clicked,
		this,
		&CodeX::addAltSolution
	);
	QObject::connect(
		this->chipDataWindow_,
		&ChipDataWindow::chipsChanged,
		this,
		&CodeX::chipsChanged
	);
	QObject::connect(
		this->timer_,
		&QTimer::timeout,
		[&]()
		{
			auto t = startTime_.msecsTo(QDateTime::currentDateTime()) / 1000.0;
			this->timeLabel_->setText(trUtf8(u8"耗时：") + QString::number(t, 'f', 2) + "s");
		}
	);
}
