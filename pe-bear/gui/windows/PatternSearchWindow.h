#pragma once
#include <QtGlobal>

#include <bearparser/bearparser.h>

#include "../../QtCompat.h"
#include "../../base/PeHandler.h"
#include "../../base/threads/SignFinderThread.h"
#include "../../gui_base/HexSpinBox.h"

//---
class PatternSearchWindow : public QDialog
{
	Q_OBJECT

public:
	PatternSearchWindow(QWidget *parent, PeHandler* peHndl);
	~PatternSearchWindow()
	{
		if (threadMngr) {
			delete threadMngr;
		}
	}

	int exec()
	{
		if (!m_peHndl) return QDialog::Rejected;

		PEFile* peFile = this->m_peHndl->getPe();
		if (!peFile) return QDialog::Rejected;

		offset_t maxOffset = peFile->getContentSize();
		offset_t offset = m_peHndl->getDisplayedOffset();

		startOffsetBox.setRange(0, maxOffset);
		startOffsetBox.setValue(offset);
		return QDialog::exec();
	}

protected slots:
	void onSearchClicked();

	void matchesFound(MatchesCollection *thread);
	void onProgressUpdated(int progress);
	void onSearchStarted(bool isStarted);

protected:
	QString stringToSignature(const QString& text);

	QVBoxLayout topLayout;
	QHBoxLayout secPropertyLayout2;
	QHBoxLayout secPropertyLayout4;
	QHBoxLayout buttonLayout;

	// search options related:
	QTabWidget tabWidget;

	QWidget signatureTab;
	QWidget stringTab;

	QVBoxLayout signatureTabLayout;
	QVBoxLayout stringTabLayout;

	QLabel patternLabel;
	QLineEdit patternEdit;

	QLabel searchedStrLabel;
	QLineEdit searchedStrEdit;
	QComboBox encodingCombo;

	QLabel offsetLabel;
	HexSpinBox startOffsetBox;
	QProgressBar progressBar;
	
	QPushButton searchButton;
	
	PeHandler* m_peHndl;
	SignFinderThreadManager *threadMngr;
};
