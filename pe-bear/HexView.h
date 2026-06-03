#pragma once

#include <stack>
#include <QtGlobal>
#include <QStyledItemDelegate>

#include "QtCompat.h"
#include "REbear.h"
#include "base/PeHandlersManager.h"
#include "PEFileTreeModel.h"

#include "gui_base/ExtTableView.h"
#include "gui_base/ClipboardUtil.h"

#include "HexDumpModel.h"
#include "OffsetHeader.h"


class HexItemDelegate: public QStyledItemDelegate
{
	Q_OBJECT
public:
	HexItemDelegate(QObject* parent);

	virtual void setModelData(QWidget * editor, QAbstractItemModel * model,
		const QModelIndex & index) const override;

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
		const QModelIndex &index) const override;

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

	void setSelectionColor(const QColor& color)
	{
		m_selectionColor = color;
	}

Q_SIGNALS:
	void dataSet(int col, int row) const;

private:
	void selectNextParentItem(const QModelIndex &index) const;

	QColor m_selectionColor;
	QRegularExpressionValidator validator;
};

//---

class HexTableView : public ExtTableView //TreeCpView
{
	Q_OBJECT
public:
	HexTableView(QWidget *parent);
	virtual QSize span(const QModelIndex &index) const { return QSize(0,0); }

	virtual void setModel(HexDumpModel *model);

	void setVHdrVisible(bool isVisible);
	virtual void keyPressEvent(QKeyEvent *event);

public slots:
	void onDataSet(int col, int row);
	void onScrollReset();
	void onModelUpdated() { reset(); }
	void changeSettings(HexViewSettings &settings);

	virtual void copySelected();
	virtual void pasteToSelected();
	virtual void clearSelected();
	virtual void fillSelected();
	virtual void followSelected();
	offset_t getSelectedAddress();

	void updateFollowAction();

	void setPageUp();
	void setPageDown();
	void undoOffset();
	void undoLastModification();
	void updateUndoAction();

public slots:
	void onResetRequested() { reset(); }

protected:
	bool isIndexListContinuous(QModelIndexList &list);

	inline void adjustMinWidth();
	int hexColWidth;
	bool isVHdrVisible;
	void init();
	void initHeader();
	void initHeaderMenu();
	void initMenu();
	void setSelectionColor(const QColor& color);

	QAction *backAction, *undoAction, *followAction;

	OffsetHeader* vHdr;
	QHeaderView *hHdr;
	HexDumpModel *hexModel;
	QScrollBar vScrollbar;
	HexItemDelegate* m_delegate;
};

