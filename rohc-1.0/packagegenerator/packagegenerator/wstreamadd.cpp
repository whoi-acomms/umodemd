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

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdir.h>
#include <qlabel.h>
#include <qslider.h>
#include <kfiledialog.h>
#include <ksimpleconfig.h>
#include <klineedit.h>
#include <knuminput.h>
#include <kpushbutton.h>
#include "wstreamadd.h"

WStreamAdd::WStreamAdd(QWidget *parent)
	: UStreamAdd(parent, "WStreamAdd", TRUE)
{
	loadSettings();
}

WStreamAdd::~WStreamAdd()
{
}

QString WStreamAdd::configName()
{
	return QDir::home().filePath(".packagegegenerator");
}

void WStreamAdd::coverageRandomChange(bool n)
{
	kIntNumInput3->setEnabled(!n);
}

void WStreamAdd::createStream(KListView * view , ProcessParser * pp)
{
	int cov;	
	int port = kIntNumInput1->value();
	int freq = slider1->value();
	QString host = kLineEdit1->text();
	int length = kIntNumInput2->value();
	int rnd_len = checkBox1->isChecked();
	QString filename = kLineEdit2->text();
	bool reloop = checkBox3->isChecked();

	StreamExec::Type type = getType();
	StreamExec * ex = new StreamExec(view, pp, type, port, freq);
	
	switch(type) {
	case StreamExec::UDPClient:
		ex->udpClient(port, host, freq, length, rnd_len);
		break;
	case StreamExec::UDPServer:
		ex->udpServer(port);
		break;
	case StreamExec::LiteClient:
		cov = kIntNumInput3->value();
		cov = checkBox2->isChecked() ? -1 : cov;
		ex->liteClient(host, freq, length, rnd_len, cov);
		break;
	case StreamExec::LiteServer:
		ex->liteServer();
		break;
	case StreamExec::MultiIpClient:
		ex->multiClient(host, freq, filename, reloop);
		break;
	case StreamExec::MultiIpServer:
		ex->multiServer();
		break;
	}
}

void WStreamAdd::fileNameChanged(const QString &)
{
	typeChanged();
}

void WStreamAdd::frequencyChange(int value)
{
	QString t = "";
	if (value < 61) {
		t = QString::number(value) + " packages/minute";
	} else {
		double d = value / 60.0;

		t.setNum(d, 'f', 2);
		t += " packages/second";
	}
	textLabel5->setText(t);
}

StreamExec::Type WStreamAdd::getType()
{
	switch(comboBox1->currentItem()) {
	case 0: return StreamExec::UDPClient;
	case 1: return StreamExec::UDPServer;
	case 2: return StreamExec::LiteClient;
	case 3: return StreamExec::LiteServer;
	case 4: return StreamExec::MultiIpClient;
	case 5: return StreamExec::MultiIpServer;
	}
	qDebug("(%s:%d) Unknown type", __FILE__, __LINE__);

	// default
	return StreamExec::UDPServer;
}

void WStreamAdd::hostChanged(const QString & s)
{
	typeChanged();
}

void WStreamAdd::lengthChanged(int max)
{
	kIntNumInput3->setMaxValue(max);
}

void WStreamAdd::loadSettings()
{
	KConfig * conf = new KSimpleConfig(configName());
	conf->setGroup("main");

	comboBox1->setCurrentItem(conf->readUnsignedNumEntry("type", 1));
	kIntNumInput1->setValue(conf->readUnsignedNumEntry("port", 5000));
	kLineEdit1->setText(conf->readEntry("host", ""));
	slider1->setValue(conf->readUnsignedNumEntry("package", 1));

	kIntNumInput2->setValue(conf->readUnsignedNumEntry("length", 40));
	checkBox1->setChecked(conf->readBoolEntry("len_rnd", false));

	kIntNumInput3->setValue(conf->readUnsignedNumEntry("cov", 8));
	checkBox2->setChecked(conf->readBoolEntry("cov_rnd", false));

	kLineEdit2->setText(conf->readEntry("filename", ""));
	checkBox3->setChecked(conf->readBoolEntry("reloop", false));

	delete conf;

	typeChanged();
}

void WStreamAdd::saveSettings()
{
	KConfig * conf = new KSimpleConfig(configName());
	conf->setGroup("main");

	conf->writeEntry("type", comboBox1->currentItem());
        conf->writeEntry("port", kIntNumInput1->value());
	conf->writeEntry("host", kLineEdit1->text());
	conf->writeEntry("package", slider1->value());
	conf->writeEntry("length", kIntNumInput2->value());
	conf->writeEntry("len_rnd", checkBox1->isChecked());
	conf->writeEntry("cov", kIntNumInput3->value());
	conf->writeEntry("cov_rnd", checkBox2->isChecked());
	conf->writeEntry("filename", kLineEdit2->text());
	conf->writeEntry("reloop", checkBox3->isChecked());

	conf->sync();

	delete conf;
}

void WStreamAdd::selectFirstFile()
{
	QString filename = KFileDialog::getOpenFileName();
	if (filename != QString::null) {
		kLineEdit2->setText(filename);
	}
}

void WStreamAdd::typeChanged()
{
	bool ok = kLineEdit1->text().length() > 0;
	bool port = true;
	bool host = false;
	bool freq = false;
	bool len = false;
	bool cov = false;
	bool file = false;
	
	switch(getType()) {
	case StreamExec::UDPClient:
		host = true;
		freq = true;
		len = true;
		break;
		
	case StreamExec::UDPServer:
		ok = true;
		break;

	case StreamExec::LiteClient:
		port = false;
		host = true;
		freq = true;
		len = true;
		cov = true;
		break;

	case StreamExec::LiteServer:
		ok = true;
		port = false;
		break;

	case StreamExec::MultiIpClient:
		if (ok) ok = kLineEdit2->text().length() > 0;
		port = false;
		host = true;
		freq = true;
		file = true;
		break;

	case StreamExec::MultiIpServer:
		ok = true;
		port = false;
		break;
	}

	kPushButton1->setEnabled(ok);	// OK
	kIntNumInput1->setEnabled(port); // port
	kLineEdit1->setEnabled(host);	// host
	textLabel3->setEnabled(host);
	slider1->setEnabled(freq);	// freq
	textLabel4->setEnabled(freq);
	textLabel5->setEnabled(freq);
	textLabel6->setEnabled(len);	// length
	kIntNumInput2->setEnabled(len);
	checkBox1->setEnabled(len);
	textLabel7->setEnabled(cov);	// coverage
	kIntNumInput3->setEnabled(cov && !checkBox2->isChecked());
	checkBox2->setEnabled(cov);
	textLabel8->setEnabled(file);	// filename
	kLineEdit2->setEnabled(file);
	kPushButton3->setEnabled(file);
	checkBox3->setEnabled(file);
}
