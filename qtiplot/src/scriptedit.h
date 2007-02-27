/***************************************************************************
    File                 : scriptedit.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, 
                           Tilman Hoener zu Siederdissen,
                           Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Scripting classes
                           
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef SCRIPTEDIT_H
#define SCRIPTEDIT_H

#include "Scripting.h"

#include <QMenu>
#include <QTextEdit>

class QAction;
class QMenu;

/*!\brief Editor widget with support for evaluating expressions and executing code.
 *
 * \section future Future Plans
 * - Display line numbers.
 * - syntax highlighting, indentation, auto-completion etc. (maybe using QScintilla)
 */
class ScriptEdit: public QTextEdit, public scripted
{
  Q_OBJECT
    
  public:
    ScriptEdit(ScriptingEnv *env, QWidget *parent=0, const char *name=0);
    ~ScriptEdit();

    void customEvent(QEvent*);
    int lineNumber(int pos) const;
    
  public slots:
    void execute();
    void executeAll();
    void evaluate();
    void print();
    QString exportASCII(const QString &file=QString::null);
    QString importASCII(const QString &file=QString::null);
    void insertFunction(const QString &);
    void insertFunction(QAction * action);
    void setContext(QObject *context) { myScript->setContext(context); }
    void scriptPrint(const QString&);
    void updateIndentation();

  protected:
    virtual void contextMenuEvent(QContextMenuEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);

  private:
    Script *myScript;
    QAction *actionExecute, *actionExecuteAll, *actionEval, *actionPrint, *actionImport, *actionExport;
    QMenu *functionsMenu;
    QTextCursor printCursor;

  private slots:
    void insertErrorMsg(const QString &message);
};

#endif
