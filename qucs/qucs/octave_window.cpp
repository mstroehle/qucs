/***************************************************************************
    copyright            : (C) 2010 by Michael Margraf
    email                : michael.margraf@alumni.tu-berlin.de
 ***************************************************************************/

#include "octave_window.h"
#include "main.h"

#include <qsize.h>
#include <qvbox.h>
#include <qcolor.h>
#include <qaccel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qtextedit.h>
#include <qdockwindow.h>

extern QDir QucsWorkDir;  // current project path

OctaveWindow::OctaveWindow(QDockWindow *parent_): QWidget(parent_, 0)
{
  vBox = new QVBoxLayout(this);

  output = new QTextEdit(this);
  output->setReadOnly(true);
  output->setUndoRedoEnabled(false);
  output->setTextFormat(Qt::LogText);
  output->setMaxLogLines(2000);
  output->setWordWrap(QTextEdit::NoWrap);
  output->setPaletteBackgroundColor(QucsSettings.BGColor);
  vBox->addWidget(output, 10);

  input = new QLineEdit(this);
  connect(input, SIGNAL(returnPressed()), SLOT(slotSendCommand()));
  vBox->addWidget(input);

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  parent_->setWidget(this);
  parent_->setResizeEnabled(true);
  parent_->setHorizontallyStretchable(true);

  histIterator = cmdHistory.end();
}

// -----------------------------------------------------------------
OctaveWindow::~OctaveWindow()
{
  if(octProcess.isRunning())
    octProcess.kill();
}

// -----------------------------------------------------------------
QSize OctaveWindow::sizeHint()
{
  QSize Size;
  int w=0, h=0;

  Size = output->sizeHint();
  w = Size.width();
  h = Size.height() + input->sizeHint().height();
  return QSize(w, h);
}

// ------------------------------------------------------------------------
bool OctaveWindow::startOctave()
{
  if(octProcess.isRunning())
    return true;

  QStringList CommandLine;
  CommandLine << "octave" << "--no-history" << "-i";
  octProcess.setArguments(CommandLine);

  disconnect(&octProcess, 0, 0, 0);
  connect(&octProcess, SIGNAL(readyReadStderr()), SLOT(slotDisplayErr()));
  connect(&octProcess, SIGNAL(readyReadStdout()), SLOT(slotDisplayMsg()));
  connect(&octProcess, SIGNAL(processExited()), SLOT(slotOctaveEnded()));

  if(!octProcess.start()) {
    output->setText(tr("ERROR: Cannot start Octave!"));
    return false;
  }

  output->clear();
  octProcess.writeToStdin("cd \"" + QucsWorkDir.absPath() + "\"\n");
  return true;
}

// ------------------------------------------------------------------------
void OctaveWindow::runOctaveScript(const QString& name)
{
  QFileInfo info(name);
  int par = output->paragraphs() - 1;
  int idx = output->paragraphLength(par);
  output->insertAt(info.baseName(true) + "\n", par, idx);

  octProcess.writeToStdin(info.baseName(true) + "\n");
  output->scrollToBottom();
}

// ------------------------------------------------------------------------
void OctaveWindow::slotSendCommand()
{
  int par = output->paragraphs() - 1;
  int idx = output->paragraphLength(par);
  output->insertAt(input->text() + "\n", par, idx);
  output->scrollToBottom();

  octProcess.writeToStdin(input->text() + "\n");
  if(!input->text().stripWhiteSpace().isEmpty())
    cmdHistory.append(input->text());
  histIterator = cmdHistory.end();
  input->clear();
}

// ------------------------------------------------------------------------
void OctaveWindow::keyPressEvent(QKeyEvent *event)
{
  if(event->key() == Qt::Key_Up) {
    if(histIterator == cmdHistory.begin())
      return;
    histIterator--;
    input->setText(*histIterator);
    return;
  }

  if(event->key() == Qt::Key_Down) {
    if(histIterator == cmdHistory.end())
      return;
    histIterator++;
    input->setText(*histIterator);
    return;
  }
}

// ------------------------------------------------------------------------
// Is called when the process sends an output to stdout.
void OctaveWindow::slotDisplayMsg()
{
  int par = output->paragraphs() - 1;
  int idx = output->paragraphLength(par);
  output->insertAt(QString(octProcess.readStdout()), par, idx);
  output->scrollToBottom();
}

// ------------------------------------------------------------------------
// Is called when the process sends an output to stderr.
void OctaveWindow::slotDisplayErr()
{
  if(!isVisible())
    ((QDockWindow*)parent())->show();  // always show an error

  int par = output->paragraphs() - 1;
  int idx = output->paragraphLength(par);
  output->insertAt(QString(octProcess.readStderr()), par, idx);
  output->scrollToBottom();
}

// ------------------------------------------------------------------------
// Is called when the simulation process terminates.
void OctaveWindow::slotOctaveEnded()
{
  output->clear();
}
