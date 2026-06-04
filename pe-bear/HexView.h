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

	void setSelectionColor(const QColor& bgColor, const QColor& textColor)
	{
		m_selectionBgColor = bgColor;
		m_selectionTextColor = textColor;
	}

Q_SIGNALS:
	void dataSet(int col, int row) const;

private:
	QColor m_selectionBgColor, m_selectionTextColor;
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
	virtual void clearSelected() { return fillSelected(0x00); }
	virtual void fillSelectedNOP() { return fillSelected(0x90); }
	virtual void fillSelectedCustom();

	virtual void followSelectedVa() { return followSelected(Executable::VA);  }
	virtual void followSelectedRva() { return followSelected(Executable::RVA); }
	virtual void followSelectedRaw() { return followSelected(Executable::RAW); }

	void updateFollowAction();

	void setPageUp();
	void setPageDown();
	void undoOffset();
	void undoLastModification();
	void updateUndoAction();

public slots:
	void onResetRequested() { reset(); }

protected:
	offset_t getSelectedAddress();
	void followSelected(const Executable::addr_type& type);
	bool isIndexListContinuous(QModelIndexList &list);
	void fillSelected(const char val);

	inline void adjustMinWidth();
	int hexColWidth;
	bool isVHdrVisible;
	void init();
	void initHeader();
	void initHeaderMenu();
	void initMenu();
	void setSelectionColor(const QColor& bgColor, const QColor& textColor);

	QMenu* followAddrSubmenu;
	QAction* followAction[Executable::ADDR_TYPE_COUNT] = { nullptr };
	QAction *backAction, *undoAction;

	OffsetHeader* vHdr;
	QHeaderView *hHdr;
	HexDumpModel *hexModel;
	QScrollBar vScrollbar;
	HexItemDelegate* m_delegate;
};

