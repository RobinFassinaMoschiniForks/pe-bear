#include "PatternSearchWindow.h"

PatternSearchWindow::PatternSearchWindow(QWidget *parent, PeHandler* peHndl)
	: QDialog(parent, Qt::Dialog),
	m_peHndl(peHndl), threadMngr(nullptr)
{
	setModal(true);
	setWindowTitle(tr("Define search"));
	offsetLabel.setText(tr("Starting from the offset:" ));
	secPropertyLayout2.addWidget(&offsetLabel);
	secPropertyLayout2.addWidget(&startOffsetBox);

	startOffsetBox.setRange(0, 0);
	startOffsetBox.setPrefix("0x");
	startOffsetBox.setValue(0);

	// Pattern search:
	QRegularExpressionValidator *validator = new QRegularExpressionValidator(QRegularExpression("([0-9A-Fa-f\\?]{0,2} {0,1})*"), this);
	patternEdit.setValidator(validator);

	const QString signInfo = tr("Hexadecimal with wild characters, i.e.") + "\"55 8B ?? 8B 45 0C\"";
	patternEdit.setToolTip(signInfo);
	patternLabel.setText(tr("Signature to search:"));
	patternEdit.setPlaceholderText(signInfo);
	patternLabel.setBuddy(&patternEdit);

	signatureTabLayout.addWidget(&patternLabel);
	signatureTabLayout.addWidget(&patternEdit);
	signatureTab.setLayout(&signatureTabLayout);

	// String search
	searchedStrLabel.setText(tr("String to search:"));
	searchedStrLabel.setBuddy(&searchedStrEdit);

	encodingCombo.addItem(tr("ANSI"));
	encodingCombo.addItem(tr("UTF-16"));

	stringTabLayout.addWidget(&searchedStrLabel);
	stringTabLayout.addWidget(&searchedStrEdit);
	stringTabLayout.addWidget(&encodingCombo);

	stringTab.setLayout(&stringTabLayout);
	//---
	tabWidget.addTab(&stringTab, tr("Strings"));
	tabWidget.addTab(&signatureTab, tr("Signatures"));

	secPropertyLayout4.addWidget(&progressBar);
	
	topLayout.addLayout(&secPropertyLayout2);
	topLayout.addWidget(&tabWidget);
	topLayout.addLayout(&secPropertyLayout4);
	topLayout.addLayout(&buttonLayout);

	progressBar.setRange(0, 1000);
	progressBar.setVisible(false);
	topLayout.addStretch();
	setLayout(&topLayout);

	searchButton.setText(tr("Search"));
	buttonLayout.addWidget(&searchButton);
	connect(&searchButton, SIGNAL(clicked()), this, SLOT(onSearchClicked()));
	patternEdit.setFocus();
}

QString PatternSearchWindow::stringToSignature(const QString& text)
{
	QByteArray bytes;

	switch (encodingCombo.currentIndex())
	{
	case 0: // ANSI
		bytes = text.toLocal8Bit();
		break;

	case 1: // UTF-16
		bytes = QByteArray(
			reinterpret_cast<const char*>(text.utf16()),
			text.size() * sizeof(ushort)
		);
		break;

	default:
		return QString();
	}

	return bytes.toHex().toUpper();
}

void PatternSearchWindow::onSearchClicked()
{
	if (!this->m_peHndl) return;
	MappedExe* exe = m_peHndl->getPe();
	if (!exe) return;

	offset_t offset = startOffsetBox.value();
	if (this->progressBar.value() == this->progressBar.maximum()) {
		startOffsetBox.setValue(0);
		offset = 0;
	}

	const size_t fullSize = exe->getContentSize();
	if (offset >= fullSize) return;
	
	QString text;
	if (tabWidget.currentWidget() == &signatureTab) {
		text = patternEdit.text();
	}
	else if (tabWidget.currentWidget() == &stringTab) {
		text = searchedStrEdit.text();
		if (text.length()) {
			text = stringToSignature(text);
		}
	}
	if (text.isEmpty()) {
		return;
	}
	
	if (!this->threadMngr) {
		threadMngr = new SignFinderThreadManager(exe);
	}

	if (!threadMngr->loadSignature(tr("Searched"), text)) {
		progressBar.setValue(0);
		QMessageBox::information(this, tr("Info"), tr("Could not parse the signature!"), QMessageBox::Ok);
		return;
	}

	connect(threadMngr, SIGNAL(gotMatches(MatchesCollection* )), 
		this, SLOT(matchesFound(MatchesCollection *)), Qt::UniqueConnection);
	connect(threadMngr, SIGNAL(progressUpdated(int )), 
		this, SLOT(onProgressUpdated(int )), Qt::UniqueConnection);
	connect(threadMngr, SIGNAL(searchStarted(bool )), 
		this, SLOT(onSearchStarted(bool )), Qt::UniqueConnection);
		
	progressBar.setVisible(true);
	progressBar.setValue(0);
	threadMngr->setStartOffset(offset);
	threadMngr->recreateThread();
}

void PatternSearchWindow::onProgressUpdated(int progress)
{
	progressBar.setValue(progress);
}

void PatternSearchWindow::onSearchStarted(bool isStarted)
{
	searchButton.setEnabled(!isStarted);
}

void PatternSearchWindow::matchesFound(MatchesCollection *matches)
{
	if (!threadMngr) return; //should never happen
	
	if (!matches || !matches->packerAtOffset.size()) {
		QMessageBox::information(this, tr("Info"), tr("Not found!"), QMessageBox::Ok);
		threadMngr->stopThread();
		return;
	}
	const QList<MatchedSign> &signAtOffset = matches->packerAtOffset;
	if (!signAtOffset.size()) {
		QMessageBox::information(this, tr("Info"), tr("Not found!"), QMessageBox::Ok);
		threadMngr->stopThread();
		return;
	}
	MatchedSign match = *(signAtOffset.begin());
	const size_t offset = match.offset;
	const size_t signLen = match.len;
	m_peHndl->setDisplayed(false, offset, signLen);
	m_peHndl->setHilighted(offset, signLen);
	startOffsetBox.setValue(offset);
	if (QMessageBox::question(this, tr("Info"), tr("Found at:") + " 0x" + QString::number(offset, 16) + "\n"+ 
		tr("Search next?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
	{
		threadMngr->stopThread();
		return;
	}

	threadMngr->setStartOffset(offset + 1);
	threadMngr->recreateThread();
}
