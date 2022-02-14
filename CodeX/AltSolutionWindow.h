#pragma once

#include <QDialog>
#include <Chip/Chip.h>
#include <unordered_map>
namespace Ui { class AltSolutionWindow; };
class GFChip;
class ChipSolver;
class ChipTableModel;
class ChipTableDelegate;
class SolutionTableModel;

class AltSolutionWindow : public QDialog
{
	Q_OBJECT

public:
	AltSolutionWindow(QWidget *parent = Q_NULLPTR);
	~AltSolutionWindow();

	void addSolution(const Solution& s);
	void clearSolution();

	void init();
	bool chipUsed(int no);

protected:
	void closeEvent(QCloseEvent*) override;

protected slots:
	void selectSolution(int index);
	void delSolution();

private:
	Ui::AltSolutionWindow *ui;
	ChipTableModel* chipTableModel_;
	ChipTableDelegate* chipTableDelegate_;
	SolutionTableModel* solutionTableModel_;
	std::vector<Solution> solutions_;
	std::unordered_map<int, int> chipCount_;

	void saveSolutions();
	void connect();
};
