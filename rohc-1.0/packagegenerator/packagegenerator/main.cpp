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

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include "packagegenerator.h"
#include "streamexec.h"
#include "udpprofile.h"
#include "liteprofile.h"
#include "multiipprofile.h"

static const char *description =
	I18N_NOOP("PackageGenerator");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE
	
	

static KCmdLineOptions options[] = {
	{ "udp ", I18N_NOOP("start a udp client and send to <port>"), "5000" },
	{ "lite ", I18N_NOOP("start a udp-lite client with coverage <c> ."), "30" },
	{ "multi ", I18N_NOOP("start a multi-ip client with the flow <f>."), ""},
	{ "host ", I18N_NOOP("connect to host."), "localhost"},
	{ "freq ", I18N_NOOP("set frequency (pkg/min) for udp client."), "60"},
	{ "length ", I18N_NOOP("package length."), "400"},
	{ "random", I18N_NOOP("randomize package length"), 0},
	{ "reloop", I18N_NOOP("reloop multi-ip flow."), 0},
	{ "servudp ", I18N_NOOP("start a udp server."), "5000"},
	{ "servlite", I18N_NOOP("start a udp-lite server."), 0},
	{ "servmulti", I18N_NOOP("start a multi-ip server."), 0},
//	{ "port", I18N_NOOP("use port"), "5000"},
/*
	{ "a", I18N_NOOP("A short binary option."), 0 },
	{ "b ", I18N_NOOP("A short option which takes an argument."), 0 },
	{ "c ", I18N_NOOP("As above but with a default value."), "9600" },
	{ "option1", I18N_NOOP("A long binary option, off by default."), 0 },
	{ "nooption2", I18N_NOOP("A long binary option, on by default."), 0 },
	{ "option3 ", I18N_NOOP("A long option which takes an argument."), 0 },
	{ "option3 ", I18N_NOOP("As above with 9600 as default."), "9600" },
	{ "d", 0, 0 },
	{ "option4", I18N_NOOP("A long option which has a short option as alias."), 0 },
	{ "e", 0, 0 },
	{ "nooption5", I18N_NOOP("Another long option with an alias."), 0 },
	{ "f", 0, 0 },
	{ "option6 ", I18N_NOOP("'--option6 speed' is same a '-f speed'"), 0 },
	{ "!option7 ", I18N_NOOP("All options following this one will be treated as arguments", 0 },
	{ "+file", I18N_NOOP("A required argument 'file'.), 0 },
	{ "+[arg1]", I18N_NOOP("An optional argument 'arg1'."), 0 },
	{ "!+command", I18N_NOOP("A required argument 'command', that can contain multiple words, even starting with '-'.), 0 },
	{ 0, 0, 0 } // End of options. };
*/

	{ 0, 0, 0 }
	// INSERT YOUR COMMANDLINE OPTIONS HERE
};

int main(int argc, char *argv[])
{
	StreamExec::setAppFileName(argv[0]);

	KAboutData aboutData( "packagegenerator", I18N_NOOP("PackageGenerator"),
		VERSION, description, KAboutData::License_GPL,
		"(c) 2003, Martin Juhlin", 0, 0, "marjuh-8@student.luth.se");
	aboutData.addAuthor("Martin Juhlin",0, "marjuh-8@student.luth.se");
	KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

	KApplication a;

	KCmdLineArgs * cmd = KCmdLineArgs::parsedArgs();
	bool udp = cmd->isSet("udp");
	bool len = cmd->isSet("length");
	bool rnd = cmd->isSet("random");
	bool host = cmd->isSet("host");
	bool freq = cmd->isSet("freq");
	bool lite = cmd->isSet("lite");
	bool multi = cmd->isSet("multi");
	bool udpserv = cmd->isSet("servudp");
	bool liteserv = cmd->isSet("servlite");
	bool multiserv = cmd->isSet("servmulti");

	bool udpclient = udp && host && len && freq;
	bool liteclient = lite && host && len && freq;
	bool multiclient = multi && host && freq && !len;
	bool client = udp || lite || multi;
	bool server = udpserv || liteserv || multiserv;
	bool extra = host || freq || len || rnd;
        
	if (udpclient && !server && !lite && !multi) {
		// start a udp client
		printf("Starting udp client\n");
		QString su = cmd->getOption("udp");
		QString sh = cmd->getOption("host");
		QString sf = cmd->getOption("freq");
		QString sl = cmd->getOption("length");

		//fprintf(stderr, "port = %d\n", su.toInt());
		//fprintf(stderr, "host = \"%s\"\n", sh.ascii());
		UdpProfile p;
		p.client(sh.ascii(), su.toInt(), sf.toInt(), sl.toInt(), rnd);

        } else if (udpserv && !client && !liteserv && !multiserv && !extra) {
        	// start a udp server
         	printf("Starting udp server\n");
		QCString su = cmd->getOption("servudp");

		UdpProfile p;
		p.server(su.toInt());
          
        } else if (liteclient && !udp && !multi && !server) {
		printf("Starting udp-lite server\n");
		QString su = cmd->getOption("lite");
		QString sh = cmd->getOption("host");
		QString sf = cmd->getOption("freq");
		QString sl = cmd->getOption("length");

                LiteProfile p;
                p.client(sh.ascii(), su.toInt(), sf.toInt(), sl.toInt(), rnd);
                
	} else if (liteserv && !client && !udpserv && !multiserv && !extra) {
         	printf("Starting udp-lite server\n");
          
                LiteProfile p;
                p.server();
		
        } else if (multiclient && !udp && !lite && !server) {
		printf("Starting multi-ip client\n");
		QString su = cmd->getOption("multi");
		QString sh = cmd->getOption("host");
		QString sf = cmd->getOption("freq");
		bool reloop = cmd->isSet("reloop");

		MultiIpProfile p;
		p.client(su, sh, sf.toInt(), reloop);
		
	} else if (multiserv && !client && !udpserv && !liteserv && !extra) {
		printf("Starting multi-ip server\n");

		MultiIpProfile p;
		p.server();
		
        } else if (!client && !server && !extra) {
        	// No option, start gui

		PackageGenerator *packagegenerator = new PackageGenerator();
		a.setMainWidget(packagegenerator);
		packagegenerator->show();  

		return a.exec();
	} else {
		KCmdLineArgs::usage();
	}
	return 0;
}
