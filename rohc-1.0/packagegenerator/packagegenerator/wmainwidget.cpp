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

#include <qpushbutton.h>
#include "processparser.h"
#include "wstreamadd.h"
#include "wmainwidget.h"

WMainWidget::WMainWidget(QWidget * parent)
	: UMainWidget(parent, "WMainWidget")
{
	pp = new ProcessParser();
}

WMainWidget::~WMainWidget()
{
}


void WMainWidget::addStream()
{
	WStreamAdd * dlg = new WStreamAdd(this);
	if (dlg->exec() == QDialog::Accepted) {
		dlg->createStream(kListView1, pp);
	}
	delete dlg;
}

void WMainWidget::removeStream()
{
	delete m_selectedItem;
	selectedStream(kListView1->selectedItem());
}


void WMainWidget::selectedStream(QListViewItem * item)
{
	m_selectedItem = item;
	pushButton2->setEnabled( (item ? true : false) );
}
