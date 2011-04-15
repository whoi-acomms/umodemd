/*
 * Package generator for generating UDP and TCP flows
 * Copyright (C) 2003  Martin Juhlin <juhlin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef WSTREAMADD_H
#define WSTREAMADD_H

#include <qwidget.h>
#include <ustreamadd.h>

#include "streamexec.h"

class KListView;

/**Dialog for adding a new stream
  *@author Martin Juhlin
  */

class WStreamAdd : public UStreamAdd
{
	Q_OBJECT
public: 
	WStreamAdd(QWidget *parent = 0);
	~WStreamAdd();

	void createStream(KListView *, ProcessParser *);

	void loadSettings();
	
public slots:
	virtual void coverageRandomChange(bool);
	virtual void fileNameChanged(const QString &);
	virtual void frequencyChange(int);
	virtual void hostChanged(const QString &);
	virtual void lengthChanged(int);
	virtual void saveSettings();
	virtual void selectFirstFile();
	virtual void typeChanged();

private:
	QString configName();
	StreamExec::Type getType();
};

#endif
